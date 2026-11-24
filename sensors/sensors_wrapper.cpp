/*
 * Copyright (C) 2018 Unlegacy Android Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <log/log.h>
#include <hardware/sensors.h>
#include <string>
#include <dlfcn.h>
#include <errno.h>
#include <inttypes.h>
#include <algorithm>
#include <condition_variable>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <set>
#include <time.h>

// The "real" sensor lib to wrap around
#define SENSOR_LIB "sensors.mako.so"

static void *lib_handle = NULL;
static struct sensors_module_t *lib_sensors_module;
static struct hw_device_t *lib_hw_device;
static sensors_poll_device_1_t dev;

// Copy of the original sensors list with the fixed properties
static const struct sensor_t *global_sensors_list = NULL;
static int global_sensors_count = 0;

// The legacy poll cannot be interrupted by a flush request. Keep its blocking
// call on one owned worker, and put samples and individual flush markers in the
// same bounded queue. In particular, sensor handles are not a bit mask.
struct PollContext {
    static constexpr size_t kQueueCapacity = 1024;
    static constexpr int kPollCapacity = 128;
    hw_device_t* device;
    std::mutex control_lock;
    std::mutex lock;
    std::condition_variable changed;
    std::deque<sensors_event_t> events;
    std::set<int> active;
    bool closing = false;
    int poll_error = 0;
};

static std::mutex context_lock;
static std::shared_ptr<PollContext> poll_context;

static std::shared_ptr<PollContext> get_poll_context() {
    std::lock_guard<std::mutex> guard(context_lock);
    return poll_context;
}

static int64_t clock_time_ns(clockid_t clock) {
    timespec value;
    if (clock_gettime(clock, &value) != 0) return -1;
    return int64_t(value.tv_sec) * 1000000000LL + value.tv_nsec;
}

static void normalize_timestamps(sensors_event_t* events, int count) {
    const int64_t boot_before = clock_time_ns(CLOCK_BOOTTIME);
    const int64_t realtime = clock_time_ns(CLOCK_REALTIME);
    const int64_t boot_after = clock_time_ns(CLOCK_BOOTTIME);
    if (boot_before < 0 || realtime < 0 || boot_after < boot_before) return;
    const int64_t boottime = boot_before + (boot_after - boot_before) / 2;
    const int64_t offset = realtime - boottime;
    // Mako's legacy HAL emits epoch timestamps. Convert only timestamps in
    // that clock domain; a HAL already using boottime must pass through intact.
    if (offset <= 0) return;
    for (int i = 0; i < count; ++i) {
        if (events[i].type != SENSOR_TYPE_META_DATA &&
                events[i].timestamp > boottime + offset / 2) {
            events[i].timestamp -= offset;
        }
    }
}

static void* legacy_poll_worker(void* argument) {
    auto* owner = static_cast<std::shared_ptr<PollContext>*>(argument);
    std::shared_ptr<PollContext> context = *owner;
    delete owner;
    auto* legacy = reinterpret_cast<sensors_poll_device_t*>(context->device);
    sensors_event_t data[PollContext::kPollCapacity];
    for (;;) {
        {
            std::unique_lock<std::mutex> guard(context->lock);
            context->changed.wait(guard, [&] {
                return context->closing ||
                        context->events.size() + PollContext::kPollCapacity <= PollContext::kQueueCapacity;
            });
            if (context->closing) break;
        }
        const int count = legacy->poll(legacy, data, PollContext::kPollCapacity);
        if (count > 0 && count <= PollContext::kPollCapacity) normalize_timestamps(data, count);
        std::unique_lock<std::mutex> guard(context->lock);
        if (context->closing) break;
        if (count <= 0 || count > PollContext::kPollCapacity) {
            context->poll_error = count < 0 ? count : -EIO;
            context->changed.notify_all();
            context->changed.wait(guard, [&] { return context->closing; });
            break;
        }
        context->changed.wait(guard, [&] {
            return context->closing || context->events.size() + count <= PollContext::kQueueCapacity;
        });
        if (context->closing) break;
        context->events.insert(context->events.end(), data, data + count);
        context->changed.notify_all();
    }

    // Bionic cannot cancel a legacy blocking poll. close() wakes our callers
    // immediately, but the worker retains the device and library until that
    // native call returns. Never free its state or dlclose executing code.
    std::lock_guard<std::mutex> control(context->control_lock);
    context->device->close(context->device);
    std::lock_guard<std::mutex> guard(context_lock);
    delete[] global_sensors_list;
    global_sensors_list = NULL;
    global_sensors_count = 0;
    lib_hw_device = NULL;
    lib_sensors_module = NULL;
    dlclose(lib_handle);
    lib_handle = NULL;
    poll_context.reset();
    return NULL;
}

static int open_lib()
{
    if (lib_handle != NULL)
        return 0;

    lib_handle = dlopen(SENSOR_LIB, RTLD_LAZY);
    if (lib_handle == NULL) {
        ALOGW("dlerror(): %s", dlerror());
        return -EINVAL;
    } else {
        const char *sym = HAL_MODULE_INFO_SYM_AS_STR;
        ALOGV("Loaded library %s", SENSOR_LIB);
        ALOGV("Opening symbol \"%s\"", sym);
        // clear old errors
        dlerror();
        lib_sensors_module = (sensors_module_t *) dlsym(lib_handle, sym);
        const char* error;
        if ((error = dlerror()) != NULL) {
            ALOGE("Error calling dlsym: %s", error);
            dlclose(lib_handle);
            lib_handle = NULL;
            return -EINVAL;
        } else {
            ALOGV("Loaded symbols from \"%s\"", sym);
        }
    }

    return 0;
}

static int get_sensor_index_from_handle(int handle)
{
    for (int i = 0; i < global_sensors_count; ++i) {
        if (global_sensors_list[i].handle == handle)
            return i;
    }
    return -1;
}

static int sensors_get_sensors_list(struct sensors_module_t *module __unused,
    struct sensor_t const** sensors_list)
{
    std::lock_guard<std::mutex> guard(context_lock);
    if (global_sensors_list != NULL) {
        *sensors_list = global_sensors_list;
        return global_sensors_count;
    }
    int ret = open_lib();
    if (ret < 0)
        return 0;

    int sensors_count =
        lib_sensors_module->get_sensors_list(lib_sensors_module, sensors_list);
    if (sensors_count <= 0) return 0;

    // sensors_list is full of consts - Manipulate this non-const list,
    // and point the const one to it when we're done.
    sensor_t *mutable_sensor_list = new sensor_t[sensors_count];
    memcpy(mutable_sensor_list, *sensors_list,
        sizeof(struct sensor_t) * sensors_count);
    global_sensors_list = mutable_sensor_list;
    global_sensors_count = sensors_count;
    *sensors_list = global_sensors_list;

    // fix sensor properties
    for (int i = 0; i < sensors_count; ++i) {
        sensor_t *sensor = &mutable_sensor_list[i];
        if (sensor->type == SENSOR_TYPE_PROXIMITY)
            sensor->flags = SENSOR_FLAG_WAKE_UP | SENSOR_FLAG_ON_CHANGE_MODE;
        else if (sensor->type == SENSOR_TYPE_SIGNIFICANT_MOTION)
            sensor->flags = SENSOR_FLAG_WAKE_UP | SENSOR_FLAG_ONE_SHOT_MODE;
        else
            sensor->maxDelay = 1000000; // 1 sec
    }

    return sensors_count;
}

static int sensor_device_close(struct hw_device_t *dev __unused)
{
    auto context = get_poll_context();
    if (context == nullptr) return 0;
    std::lock_guard<std::mutex> control(context->control_lock);
    std::lock_guard<std::mutex> guard(context->lock);
    context->closing = true;
    context->events.clear();
    context->changed.notify_all();
    return 0;
}

static int sensor_device_activate(struct sensors_poll_device_t *dev __unused,
     int handle, int enabled)
{
    auto context = get_poll_context();
    if (context == nullptr) return -ENODEV;
    std::lock_guard<std::mutex> control(context->control_lock);
    {
        std::lock_guard<std::mutex> guard(context->lock);
        if (context->closing) return -ENODEV;
    }
    if (get_sensor_index_from_handle(handle) < 0) return -EINVAL;
    struct sensors_poll_device_t *mod =
        (struct sensors_poll_device_t *) context->device;
    ALOGV("%s: handle=%d enabled=%d", __func__, handle, enabled);
    int result = mod->activate(mod, handle, enabled);
    if (result == 0) {
        std::lock_guard<std::mutex> guard(context->lock);
        if (enabled) context->active.insert(handle);
        else context->active.erase(handle);
    }
    return result;
}

static int sensor_device_batch(struct sensors_poll_device_1*, int, int, int64_t, int64_t);

static int sensor_device_set_delay(struct sensors_poll_device_t *dev __unused,
    int handle, int64_t ns)
{
    return sensor_device_batch(&::dev, handle, 0, ns, 0);
}

static int sensor_device_poll(struct sensors_poll_device_t *dev __unused,
    sensors_event_t* data, int count)
{
    if (data == NULL || count <= 0) return -EINVAL;
    auto context = get_poll_context();
    if (context == nullptr) return -ENODEV;
    std::unique_lock<std::mutex> guard(context->lock);
    context->changed.wait(guard, [&] {
        return context->closing || !context->events.empty() || context->poll_error != 0;
    });
    if (context->closing) return -ENODEV;
    if (context->events.empty()) return context->poll_error;
    int num_events = std::min(static_cast<size_t>(count), context->events.size());
    for (int i = 0; i < num_events; ++i) {
        data[i] = context->events.front();
        context->events.pop_front();
    }
    context->changed.notify_all();
    return num_events;
}

static int sensor_device_batch(struct sensors_poll_device_1 *dev __unused,
    int handle, int flags __unused, int64_t period_ns, int64_t timeout __unused)
{
    auto context = get_poll_context();
    if (context == nullptr) return -ENODEV;
    std::lock_guard<std::mutex> control(context->control_lock);
    {
        std::lock_guard<std::mutex> guard(context->lock);
        if (context->closing) return -ENODEV;
    }
    struct sensors_poll_device_t *mod =
        (struct sensors_poll_device_t *) context->device;

    int sensor_index = get_sensor_index_from_handle(handle);
    if (sensor_index < 0 || period_ns < 0) return -EINVAL;
    const int64_t maxDelay = global_sensors_list[sensor_index].maxDelay;
    const int64_t minDelay = global_sensors_list[sensor_index].minDelay;
    if (minDelay > 0 && period_ns < minDelay * 1000LL) {
        period_ns = minDelay * 1000LL;
    }
    if (maxDelay > 0 && period_ns > maxDelay * 1000LL) {
        period_ns = maxDelay * 1000LL;
    }

    ALOGV("%s: handle=%d type=%d maxDelay=%" PRId64 " minDelay=%" PRId64
        ", delay set to %" PRId64,
        __func__, handle, global_sensors_list[sensor_index].type,
        maxDelay, minDelay, period_ns);

    return mod->setDelay(mod, handle, period_ns);
}

static int sensor_device_flush(struct sensors_poll_device_1 *dev __unused,
    int sensor_handle)
{
    auto context = get_poll_context();
    if (context == nullptr) return -ENODEV;
    std::lock_guard<std::mutex> control(context->control_lock);
    const int index = get_sensor_index_from_handle(sensor_handle);
    if (index < 0 || (global_sensors_list[index].flags & SENSOR_FLAG_MASK_REPORTING_MODE)
            == SENSOR_FLAG_ONE_SHOT_MODE) return -EINVAL;
    std::lock_guard<std::mutex> guard(context->lock);
    if (context->closing) return -ENODEV;
    if (context->active.count(sensor_handle) == 0) return -EINVAL;
    if (context->events.size() >= PollContext::kQueueCapacity) return -ENOMEM;
    sensors_event_t complete = {};
    complete.version = META_DATA_VERSION;
    complete.type = SENSOR_TYPE_META_DATA;
    complete.meta_data.what = META_DATA_FLUSH_COMPLETE;
    complete.meta_data.sensor = sensor_handle;
    context->events.push_back(complete);
    context->changed.notify_all();
    return 0;
}

static int open_sensors(const struct hw_module_t *hw_module, const char *name,
    struct hw_device_t **hw_device_out)
{
    std::lock_guard<std::mutex> guard(context_lock);
    // A closed legacy poll may still be unwinding. Its library must remain
    // loaded, and a second instance must not reuse that legacy global state.
    if (poll_context != nullptr) return -EBUSY;
    int ret = open_lib();
    if (ret < 0)
        return ret;

    ret = lib_sensors_module->common.methods->
        open((hw_module_t *) lib_sensors_module, name, &lib_hw_device);
    if (ret < 0) {
        dlclose(lib_handle);
        lib_handle = NULL;
        return ret;
    }

    memset(&dev, 0, sizeof(sensors_poll_device_1_t));
    dev.common.tag     = HARDWARE_DEVICE_TAG;
    dev.common.version = SENSORS_DEVICE_API_VERSION_1_3;
    dev.common.module  = const_cast<hw_module_t*>(hw_module);
    dev.common.close   = sensor_device_close;
    dev.activate       = sensor_device_activate;
    dev.setDelay       = sensor_device_set_delay;
    dev.poll           = sensor_device_poll;
    dev.batch          = sensor_device_batch;
    dev.flush          = sensor_device_flush;

    auto context = std::make_shared<PollContext>();
    context->device = lib_hw_device;
    poll_context = context;
    auto* owner = new std::shared_ptr<PollContext>(context);
    pthread_t worker;
    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setdetachstate(&attributes, PTHREAD_CREATE_DETACHED);
    ret = pthread_create(&worker, &attributes, legacy_poll_worker, owner);
    pthread_attr_destroy(&attributes);
    if (ret != 0) {
        delete owner;
        poll_context.reset();
        lib_hw_device->close(lib_hw_device);
        lib_hw_device = NULL;
        delete[] global_sensors_list;
        global_sensors_list = NULL;
        global_sensors_count = 0;
        dlclose(lib_handle);
        lib_handle = NULL;
        lib_sensors_module = NULL;
        return -ret;
    }

    *hw_device_out = &dev.common;

    return 0;
}

static struct hw_module_methods_t sensors_module_methods = {
    .open = open_sensors
};

struct sensors_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = SENSORS_MODULE_API_VERSION_0_1,
        .hal_api_version = SENSORS_MODULE_API_VERSION_0_1,
        .id = SENSORS_HARDWARE_MODULE_ID,
        .name = "Mako Sensors Module",
        .author = "Google, Inc",
        .methods = &sensors_module_methods,
        .dso = NULL,
        .reserved = {0},
    },
    .get_sensors_list = sensors_get_sensors_list
};
