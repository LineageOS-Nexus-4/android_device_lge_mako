#include "CameraDeviceSessionShim.h"

#include <linux/ioctl.h>
#include <system/camera_metadata.h>
#include <android-base/file.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>

#include <cinttypes>
#include <cstring>
#include <algorithm>
#include <map>

namespace aidl::android::hardware::camera::provider::shim {

using ::aidl::android::hardware::camera::device::StreamConfiguration;
using ::aidl::android::hardware::camera::device::HalStream;
using ::aidl::android::hardware::camera::device::CameraMetadata;
using ::aidl::android::hardware::camera::device::ShutterMsg;
using ::aidl::android::hardware::camera::device::RequestTemplate;
using ::aidl::android::hardware::camera::device::CaptureRequest;
using ::aidl::android::hardware::camera::device::BufferCache;
using ::aidl::android::hardware::camera::device::Stream;
using ::aidl::android::hardware::camera::device::CaptureResult;
using ::aidl::android::hardware::common::NativeHandle;
using ::aidl::android::hardware::camera::device::StreamBuffer;
using ::aidl::android::hardware::camera::device::BufferStatus;
using ::aidl::android::hardware::camera::device::ConfigureStreamsRet;
using ::android::GraphicBufferMapper;

namespace {

constexpr int32_t kHALPixelFormatYV12 = 0x32315659;
constexpr int32_t kHALPixelFormatNV21 = 0x11;
constexpr int32_t kHALPixelFormatNV12 = 0x3231564e;
constexpr int32_t kHALPixelFormatBayer = 0x100;

struct ShimCameraMemory {
    camera_memory_t mem;
    void* dataPtr;
    size_t size;
    int fd;
};

native_handle_t* toNativeHandle(const NativeHandle& nh) {
    native_handle_t* h = native_handle_create(nh.fds.size(), nh.ints.size());
    if (h == nullptr) {
        return nullptr;
    }
    int i = 0;
    for (const auto& fd : nh.fds) {
        // The AIDL request owns the fd and closes it when the request is
        // destroyed; duplicate it so our stored handle stays valid.
        h->data[i++] = dup(fd.get());
    }
    for (int32_t v : nh.ints) {
        h->data[i++] = v;
    }
    return h;
}

camera_memory_t* shimGetMemory(int fd, size_t buf_size, unsigned int num_bufs, void* /*user*/) {
    auto* m = new ShimCameraMemory();
    m->fd = fd;
    m->size = buf_size * num_bufs;
    if (fd >= 0) {
        // Real ion/graphics buffer: map it so the camera daemon's output is
        // visible in the callback data.
        m->dataPtr = mmap(nullptr, m->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (m->dataPtr == MAP_FAILED) {
            m->dataPtr = nullptr;
        }
    } else {
        m->dataPtr = malloc(m->size);
    }
    m->mem.data = m->dataPtr;
    m->mem.size = buf_size;
    m->mem.handle = nullptr;
    m->mem.release = [](camera_memory_t* mem) {
        auto* s = reinterpret_cast<ShimCameraMemory*>(mem);
        if (s->fd >= 0) {
            if (s->dataPtr != nullptr) {
                munmap(s->dataPtr, s->size);
            }
        } else {
            free(s->dataPtr);
        }
        delete s;
    };
    return &m->mem;
}


// --- Dummy preview window ---
static int shimWinDequeue(preview_stream_ops_t* w, buffer_handle_t** buf, int* stride) {
    auto* win = reinterpret_cast<ShimPreviewWindow*>(w);
    if (win->handles.empty()) {
        int err = win->allocateBuffers();
        if (err != 0) {
            return err;
        }
    }
    size_t idx = win->next % win->handles.size();
    win->next++;
    *buf = &win->handles[idx];
    *stride = win->strides[idx];
    return 0;
}

static int shimWinEnqueue(preview_stream_ops_t*, buffer_handle_t*) {
    return 0;
}

static int shimWinCancel(preview_stream_ops_t*, buffer_handle_t*) {
    return 0;
}

static int shimWinLock(preview_stream_ops_t*, buffer_handle_t*) {
    return 0;
}

static int shimWinSetBufferCount(preview_stream_ops_t* w, int count) {
    reinterpret_cast<ShimPreviewWindow*>(w)->count = count;
    return 0;
}

static int shimWinSetGeometry(preview_stream_ops_t* w, int width, int height, int format) {
    auto* win = reinterpret_cast<ShimPreviewWindow*>(w);
    win->width = width;
    win->height = height;
    win->format = format;
    return win->allocateBuffers();
}

static int shimWinSetUsage(preview_stream_ops_t*, int) {
    return 0;
}

static int shimWinGetMinUndequeued(const preview_stream_ops_t*, int* count) {
    *count = 0;
    return 0;
}

static int shimWinSetCrop(preview_stream_ops_t*, int, int, int, int) {
    return 0;
}

static int shimWinSetSwapInterval(preview_stream_ops_t*, int) {
    return 0;
}

static int shimWinSetTimestamp(preview_stream_ops_t*, int64_t) {
    return 0;
}

void initShimPreviewWindow(ShimPreviewWindow* win) {
    win->ops.dequeue_buffer = shimWinDequeue;
    win->ops.enqueue_buffer = shimWinEnqueue;
    win->ops.cancel_buffer = shimWinCancel;
    win->ops.lock_buffer = shimWinLock;
    win->ops.set_buffer_count = shimWinSetBufferCount;
    win->ops.set_buffers_geometry = shimWinSetGeometry;
    win->ops.set_usage = shimWinSetUsage;
    win->ops.get_min_undequeued_buffer_count = shimWinGetMinUndequeued;
    win->ops.set_crop = shimWinSetCrop;
    win->ops.set_swap_interval = shimWinSetSwapInterval;
    win->ops.set_timestamp = shimWinSetTimestamp;
}

// Minimal mako ION uapi definitions (kernel 3.4) for cache maintenance.
struct ShimIonFdData {
    void* handle;
    int fd;
};
struct ShimIonHandleData {
    void* handle;
};
struct ShimIonFlushData {
    void* handle;
    int fd;
    void* vaddr;
    unsigned int offset;
    unsigned int length;
};
struct ShimIonCustomData {
    unsigned int cmd;
    unsigned long arg;
};
#define SHIM_ION_IOC_MAGIC 'I'
#define SHIM_ION_IOC_IMPORT _IOWR(SHIM_ION_IOC_MAGIC, 5, struct ShimIonFdData)
#define SHIM_ION_IOC_FREE _IOWR(SHIM_ION_IOC_MAGIC, 1, struct ShimIonHandleData)
#define SHIM_ION_IOC_CUSTOM _IOWR(SHIM_ION_IOC_MAGIC, 6, struct ShimIonCustomData)
#define SHIM_ION_MSM_MAGIC 'M'
#define SHIM_ION_IOC_CLEAN_INV_CACHES _IOWR(SHIM_ION_MSM_MAGIC, 2, struct ShimIonFlushData)

static void shimCleanCache(const native_handle_t* nh, void* vaddr, size_t length) {
    if (nh == nullptr || nh->numFds < 1 || vaddr == nullptr) {
        return;
    }
    int ionFd = open("/dev/ion", O_RDONLY);
    if (ionFd < 0) {
        ALOGE("shim: cache clean: open /dev/ion failed errno=%d", errno);
        return;
    }
    struct ShimIonFdData fdData;
    memset(&fdData, 0, sizeof(fdData));
    fdData.fd = nh->data[0];
    if (ioctl(ionFd, SHIM_ION_IOC_IMPORT, &fdData) == 0 && fdData.handle != nullptr) {
        struct ShimIonFlushData flush;
        memset(&flush, 0, sizeof(flush));
        flush.handle = fdData.handle;
        flush.fd = nh->data[0];
        flush.vaddr = vaddr;
        flush.length = length;
        struct ShimIonCustomData custom;
        custom.cmd = SHIM_ION_IOC_CLEAN_INV_CACHES;
        custom.arg = reinterpret_cast<unsigned long>(&flush);
        int cr = ioctl(ionFd, SHIM_ION_IOC_CUSTOM, &custom);
        if (cr != 0) {
            ALOGE("shim: cache clean failed errno=%d", errno);
        }
        struct ShimIonHandleData hd;
        hd.handle = fdData.handle;
        ioctl(ionFd, SHIM_ION_IOC_FREE, &hd);
    }
    close(ionFd);
}

// Compute the center-crop source window for a destination aspect ratio.
// The camera2 contract scales the sensor crop to each stream size without
// distorting it (the scaler crops the sensor region). The legacy HAL only
// gives us fixed-size frames, so emulate the crop here: sample the largest
// centered source rectangle with the destination's aspect ratio.
static void cropWindow(int srcW, int srcH, int dstW, int dstH, int* cx0, int* cy0,
                       int* cw, int* ch) {
    if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) {
        *cx0 = 0; *cy0 = 0; *cw = srcW; *ch = srcH;
        return;
    }
    const double dstAspect = static_cast<double>(dstW) / dstH;
    const double srcAspect = static_cast<double>(srcW) / srcH;
    if (srcAspect > dstAspect) {
        // Source too wide: crop horizontally.
        *cw = (dstAspect >= 1.0)
                ? srcW
                : static_cast<int>(srcH * dstAspect + 0.5);
        // General: cw = srcH * dstAspect when that fits within srcW.
        if (*cw > srcW) *cw = srcW;
        *ch = static_cast<int>(*cw / dstAspect + 0.5);
        if (*ch > srcH) { *ch = srcH; *cw = static_cast<int>(*ch * dstAspect + 0.5); }
    } else {
        *ch = static_cast<int>(srcW / dstAspect + 0.5);
        if (*ch > srcH) *ch = srcH;
        *cw = static_cast<int>(*ch * dstAspect + 0.5);
        if (*cw > srcW) { *cw = srcW; *ch = static_cast<int>(*cw / dstAspect + 0.5); }
    }
    *cx0 = (srcW - *cw) / 2 & ~1;  // even-aligned for chroma subsampling
    *cy0 = (srcH - *ch) / 2 & ~1;
}

// Center-crop a packed NV21 frame to the destination aspect and scale it to
// RGBA (BT.601 full range), nearest-neighbor.
static void nv21ToRgba(const uint8_t* src, uint8_t* dst, int srcW, int srcH,
                       int dstW, int dstH) {
    if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) {
        return;
    }
    int cx0, cy0, cw, ch;
    cropWindow(srcW, srcH, dstW, dstH, &cx0, &cy0, &cw, &ch);
    const uint8_t* yPlane = src;
    const uint8_t* vuPlane = src + static_cast<size_t>(srcW) * srcH;
    for (int y = 0; y < dstH; y++) {
        int sy = cy0 + (y * ch) / dstH;
        if (sy >= srcH) sy = srcH - 1;
        const uint8_t* yRow = yPlane + static_cast<size_t>(sy) * srcW;
        const uint8_t* vuRow = vuPlane + static_cast<size_t>(sy / 2) * srcW;
        uint8_t* dstRow = dst + static_cast<size_t>(y) * dstW * 4;
        for (int x = 0; x < dstW; x++) {
            int sx = cx0 + (x * cw) / dstW;
            if (sx >= srcW) sx = srcW - 1;
            int yy = yRow[sx];
            int v = vuRow[sx & ~1];
            int u = vuRow[(sx & ~1) + 1];
            int c = yy - 16;
            int d = u - 128;
            int e = v - 128;
            int r = (298 * c + 409 * e + 128) >> 8;
            int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
            int b = (298 * c + 516 * d + 128) >> 8;
            uint8_t* p = dstRow + static_cast<size_t>(x) * 4;
            p[0] = (r < 0) ? 0 : ((r > 255) ? 255 : static_cast<uint8_t>(r));
            p[1] = (g < 0) ? 0 : ((g > 255) ? 255 : static_cast<uint8_t>(g));
            p[2] = (b < 0) ? 0 : ((b > 255) ? 255 : static_cast<uint8_t>(b));
            p[3] = 0xff;
        }
    }
}

// Center-crop a packed NV21 frame to the destination aspect and scale it to
// packed NV12 (Y plane then interleaved UV, U first) — the color format the
// msm8960 OMX video encoder consumes from gralloc encoder buffers.
static void nv21ToNv12(const uint8_t* src, uint8_t* dst, int srcW, int srcH,
                       int dstW, int dstH) {
    if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) {
        return;
    }
    int cx0, cy0, cw, ch;
    cropWindow(srcW, srcH, dstW, dstH, &cx0, &cy0, &cw, &ch);
    const uint8_t* yPlane = src;
    const uint8_t* vuPlane = src + static_cast<size_t>(srcW) * srcH;
    uint8_t* dstY = dst;
    uint8_t* dstUv = dst + static_cast<size_t>(dstW) * dstH;
    for (int y = 0; y < dstH; y++) {
        int sy = cy0 + (y * ch) / dstH;
        if (sy >= srcH) sy = srcH - 1;
        const uint8_t* yRow = yPlane + static_cast<size_t>(sy) * srcW;
        uint8_t* dstRow = dstY + static_cast<size_t>(y) * dstW;
        for (int x = 0; x < dstW; x++) {
            int sx = cx0 + (x * cw) / dstW;
            if (sx >= srcW) sx = srcW - 1;
            dstRow[x] = yRow[sx];
        }
        if (y & 1) continue;
        // Chroma: one UV row per two luma rows.
        int cy = sy & ~1;
        const uint8_t* vRow = vuPlane + static_cast<size_t>(cy / 2) * srcW;
        uint8_t* uvRow = dstUv + static_cast<size_t>(y / 2) * dstW;
        for (int x = 0; x < dstW; x += 2) {
            int sx = cx0 + ((x) * cw) / dstW;
            sx &= ~1;
            if (sx >= srcW - 1) sx = srcW - 2;
            int v = vRow[sx];
            int u = vRow[sx + 1];
            uvRow[x] = static_cast<uint8_t>(u);      // NV12: U first
            uvRow[x + 1] = static_cast<uint8_t>(v);
        }
    }
}

}  // namespace

// --- Dummy preview window ---------------------------------------------------

int ShimPreviewWindow::allocateBuffers() {
    if (gralloc == nullptr) {
        const hw_module_t* mod = nullptr;
        if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &mod) != 0) {
            ALOGE("shim: gralloc module not found for preview window");
            return -ENODEV;
        }
        if (gralloc_open(mod, &mGrallocDev) != 0) {
            ALOGE("shim: gralloc_open failed");
            return -ENODEV;
        }
        gralloc = reinterpret_cast<gralloc_module_t*>(mGrallocDev);
    }
    if (mGrallocDev->alloc == nullptr || mGrallocDev->free == nullptr) {
        return -ENODEV;
    }
    for (auto h : handles) {
        if (h != nullptr) {
            mGrallocDev->free(mGrallocDev, h);
        }
    }
    handles.clear();
    strides.clear();
    next = 0;
    int n = (count > 0) ? count : 4;
    if (width <= 0) width = 640;
    if (height <= 0) height = 480;
    for (int i = 0; i < n; i++) {
        buffer_handle_t h = nullptr;
        int stride = 0;
        int err = mGrallocDev->alloc(mGrallocDev, width, height, format,
                                     GRALLOC_USAGE_HW_CAMERA_WRITE |
                                             GRALLOC_USAGE_SW_READ_OFTEN,
                                     &h, &stride);
        if (err != 0 || h == nullptr) {
            ALOGE("shim: preview window buffer %d alloc failed err=%d", i, err);
            return (err != 0) ? -err : -ENOMEM;
        }
        handles.push_back(h);
        strides.push_back(stride);
    }
    ALOGI("shim: preview window allocated %d buffers %dx%d fmt=%d", n, width, height, format);
    return 0;
}


CameraDeviceSessionShim::CameraDeviceSessionShim(camera_module_t* module, int cameraId,
                                                 std::shared_ptr<ICameraDeviceCallback> callback)
    : mModule(module),
      mCameraId(cameraId),
      mCallback(std::move(callback)),
      mDevice(nullptr),
      mPreviewMemory(nullptr),
      mPreviewData(nullptr),
      mJpegMemory(nullptr),
      mJpegData(nullptr),
      mPreviewStreamId(-1),
      mPreviewWidth(0),
      mPreviewHeight(0),
      mPreviewFormat(0),
      mJpegStreamId(-1),
      mJpegWidth(0),
      mJpegHeight(0),
      mHalPreviewWidth(0),
      mHalPreviewHeight(0),
      mActiveRequestId(-1),
      mNextFrameNumber(1),
      mConfigured(false),
      mClosed(false),
      mPreviewRunning(false) {}

CameraDeviceSessionShim::~CameraDeviceSessionShim() {
    close();
}

bool CameraDeviceSessionShim::openHalDeviceLocked() {
    if (mDevice != nullptr) {
        return true;
    }
    camera_device_t* device = nullptr;
    int orc = mModule->common.methods->open(reinterpret_cast<const hw_module_t*>(mModule),
                                           std::to_string(mCameraId).c_str(),
                                           reinterpret_cast<hw_device_t**>(&device));
    ALOGI("shim: open camera %d rc=%d", mCameraId, orc);
    if (orc != 0 || device == nullptr) {
        return false;
    }
    device->ops->set_callbacks(device, halNotify, halDataCallback, halDataCallbackTimestamp,
                               shimGetMemory, this);
    if (device->ops->enable_msg_type != nullptr) {
        device->ops->enable_msg_type(device, CAMERA_MSG_PREVIEW_FRAME | CAMERA_MSG_SHUTTER |
                                              CAMERA_MSG_COMPRESSED_IMAGE);
    }
    initShimPreviewWindow(&mShimWindow);
    if (device->ops->set_preview_window != nullptr) {
        device->ops->set_preview_window(device, &mShimWindow.ops);
        ALOGI("shim: preview window set");
    }
    if (device->ops->get_parameters != nullptr) {
        char* params = device->ops->get_parameters(device);
        ALOGI("shim: camera %d parameters: %s", mCameraId, params ? params : "(null)");
        if (params != nullptr) {
            free(params);
        }
    }
    mDevice = device;
    return true;
}

void CameraDeviceSessionShim::halNotify(int32_t msgType, int32_t ext1, int32_t ext2, void* user) {
    CameraDeviceSessionShim* self = static_cast<CameraDeviceSessionShim*>(user);
    self->halDeviceNotify(msgType, ext1, ext2);
}

void CameraDeviceSessionShim::halDataCallback(int32_t msgType, const camera_memory_t* data,
                                              unsigned int index,
                                              camera_frame_metadata_t* /*metadata*/,
                                              void* user) {
    CameraDeviceSessionShim* self = static_cast<CameraDeviceSessionShim*>(user);
    self->halDeviceData(msgType, data, index);
}

void CameraDeviceSessionShim::halDataCallbackTimestamp(nsecs_t /*timestamp*/, int32_t msgType,
                                                       const camera_memory_t* data,
                                                       unsigned int index, void* user) {
    CameraDeviceSessionShim* self = static_cast<CameraDeviceSessionShim*>(user);
    self->halDeviceData(msgType, data, index);
}

void CameraDeviceSessionShim::halDeviceNotify(int32_t msgType, int32_t /*ext1*/,
                                              int32_t /*ext2*/) {
    // The HAL's shutter message is not used: we send the shutter notification
    // ourselves with the exact request frame number and a timestamp that
    // matches the result metadata.
    (void)msgType;
}

void CameraDeviceSessionShim::halDeviceData(int32_t msgType, const camera_memory_t* data,
                                            unsigned int index) {
    const void* buf = data->data;
    if (index > 0) {
        buf = static_cast<const uint8_t*>(data->data) + data->size * index;
    }
    if (msgType & CAMERA_MSG_PREVIEW_FRAME) {
        handlePreviewBuffer(buf, data->size);
    } else if (msgType & CAMERA_MSG_VIDEO_FRAME) {
        // The HAL record channel is never started (see configureStreams):
        // late VIDEO_FRAME messages can only be leftovers from a previous
        // session. Drop them without touching the VFE ring.
        ALOGW("shim: dropping stray VIDEO_FRAME callback");
    } else if (msgType & CAMERA_MSG_COMPRESSED_IMAGE) {
        handleJpegBuffer(buf, data->size);
    }
}

bool CameraDeviceSessionShim::setHalParametersLocked(const std::string& params) {
    if (mDevice == nullptr) {
        return false;
    }
    // The legacy HAL treats the parameters string as the COMPLETE set:
    // its setParameters() runs every set* (setPreviewFpsRange,
    // setPreviewSize, setHighFrameRate, ...) against the passed object,
    // so a partial string (e.g. "focus-mode=auto;flash-mode=off;") leaves
    // the other keys NULL. That corrupts the HAL state: "FPS range value
    // not supported" wedges the VFE config and the preview callbacks stop
    // arriving, which stalls the framework's drain at the next camera
    // switch (5s timeout), tears the session down in an error state and
    // leaves the daemon's VFE/camif objects dirty for the next camera
    // (garbage preview -> VFE IOMMU fault -> watchdog reboot).
    // Merge the changed keys into the full current parameter string first.
    char* full = mDevice->ops->get_parameters(mDevice);
    if (full == nullptr) {
        return false;
    }
    std::string merged(full);
    free(full);
    size_t start = 0;
    while (start < params.size()) {
        size_t end = params.find(';', start);
        if (end == std::string::npos) {
            end = params.size();
        }
        std::string kv = params.substr(start, end - start);
        size_t eq = kv.find('=');
        if (eq != std::string::npos && eq > 0) {
            std::string key = kv.substr(0, eq);
            std::string needle = key + "=";
            size_t pos = merged.find(needle);
            if (pos != std::string::npos) {
                size_t semicolon = merged.find(';', pos);
                if (semicolon == std::string::npos) {
                    semicolon = merged.size();
                }
                merged.replace(pos, semicolon - pos, kv);
            } else {
                if (!merged.empty() && merged.back() != ';') {
                    merged += ';';
                }
                merged += kv;
            }
        }
        start = end + 1;
    }
    return mDevice->ops->set_parameters(mDevice, merged.c_str()) == 0;
}

ndk::ScopedAStatus CameraDeviceSessionShim::close() {
    std::vector<PendingBuffer> drain;
    std::map<int32_t, std::vector<PendingBuffer>> extraDrain;
    camera_device_t* dev = nullptr;
    bool previewRunning = false;
    camera_memory_t* previewMem = nullptr;
    camera_memory_t* jpegMem = nullptr;
    std::shared_ptr<ICameraDeviceCallback> cb;
    {
        std::lock_guard<std::mutex> lock(mLock);
        if (mClosed) {
            return ndk::ScopedAStatus::ok();
        }
        mClosed = true;
        // Stop the watchdog before tearing anything down: it delivers
        // results via mCallback and must not race the close drain.
        mWatchdogExit.store(true);
        if (mWatchdogThread.joinable()) {
            mWatchdogThread.join();
        }
        // Collect pending buffers so the framework's inflight tracking
        // drains and waitUntilDrained does not time out (which closes the
        // app with an error).
        {
            std::lock_guard<std::mutex> qlock(mQueueLock);
            drain.assign(mPendingPreview.begin(), mPendingPreview.end());
            drain.insert(drain.end(), mPendingJpeg.begin(), mPendingJpeg.end());
            for (auto& [sid, q] : mPendingExtra) {
                extraDrain[sid].insert(extraDrain[sid].end(), q.begin(), q.end());
            }
            mPendingPreview.clear();
            mPendingJpeg.clear();
            mPendingExtra.clear();
        }
        dev = mDevice;
        previewRunning = mPreviewRunning;
        mDevice = nullptr;
        previewMem = mPreviewMemory;
        mPreviewMemory = nullptr;
        jpegMem = mJpegMemory;
        mJpegMemory = nullptr;
        cb = mCallback;
    }
    if (dev != nullptr) {
        if (previewRunning) {
            dev->ops->stop_preview(dev);
        }
        dev->ops->release(dev);
        dev->common.close(&dev->common);
    }
    if (previewMem != nullptr) {
        previewMem->release(previewMem);
    }
    if (jpegMem != nullptr) {
        jpegMem->release(jpegMem);
    }
    // Deliver error results after mLock is released (the framework may
    // call back into the shim during processCaptureResult).
    if (cb != nullptr) {
        for (const auto& p : drain) {
            CaptureResult r;
            r.frameNumber = p.frameNumber;
            r.fmqResultSize = 0;
            r.partialResult = 1;
            r.inputBuffer.streamId = -1;
            // Attach minimum metadata so the framework's inflight tracker
            // marks the request complete (see flush() drain above).
            const int64_t ts = systemTime(SYSTEM_TIME_MONOTONIC);
            camera_metadata_t* fm = allocate_camera_metadata(4, 64);
            add_camera_metadata_entry(fm, ANDROID_SENSOR_TIMESTAMP, &ts, 1);
            const int32_t aeState = ANDROID_CONTROL_AE_STATE_INACTIVE;
            add_camera_metadata_entry(fm, ANDROID_CONTROL_AE_STATE, &aeState, 1);
            const int32_t afState = ANDROID_CONTROL_AF_STATE_INACTIVE;
            add_camera_metadata_entry(fm, ANDROID_CONTROL_AF_STATE, &afState, 1);
            size_t metaSize = get_camera_metadata_size(fm);
            r.result.metadata.resize(metaSize);
            memcpy(r.result.metadata.data(), fm, metaSize);
            free_camera_metadata(fm);
            StreamBuffer b{};
            b.streamId = mPreviewStreamId;
            b.bufferId = p.bufferId;
            b.status = BufferStatus::ERROR;
            r.outputBuffers.push_back(std::move(b));
            // The framework completes an inflight request only after its
            // shutter arrives; without this the drain runs the full 5s
            // timeout and the session tears down in an error state.
            sendShutterNotify(p.frameNumber, ts);
            std::vector<CaptureResult> results;
            results.push_back(std::move(r));
            cb->processCaptureResult(results);
        }
        // Extra-stream parts: shutters already went out with the preview
        // parts; return them as ERROR parts so those frames complete too.
        for (auto& [sid, vec] : extraDrain) {
            for (const auto& p : vec) {
                CaptureResult r;
                r.frameNumber = p.frameNumber;
                r.fmqResultSize = 0;
                r.partialResult = 1;
                r.inputBuffer.streamId = -1;
                const int64_t ts = systemTime(SYSTEM_TIME_MONOTONIC);
                camera_metadata_t* fm = allocate_camera_metadata(4, 64);
                add_camera_metadata_entry(fm, ANDROID_SENSOR_TIMESTAMP, &ts, 1);
                size_t metaSize = get_camera_metadata_size(fm);
                r.result.metadata.resize(metaSize);
                memcpy(r.result.metadata.data(), fm, metaSize);
                free_camera_metadata(fm);
                StreamBuffer b{};
                b.streamId = sid;
                b.bufferId = p.bufferId;
                b.status = BufferStatus::ERROR;
                r.outputBuffers.push_back(std::move(b));
                std::vector<CaptureResult> results;
                results.push_back(std::move(r));
                cb->processCaptureResult(results);
            }
        }
    }
    return ndk::ScopedAStatus::ok();
}

static bool streamIsPreview(const Stream& s) {
    return s.format == ::aidl::android::hardware::graphics::common::PixelFormat::IMPLEMENTATION_DEFINED ||
           s.format == ::aidl::android::hardware::graphics::common::PixelFormat::YCBCR_420_888 || s.format == ::aidl::android::hardware::graphics::common::PixelFormat::YCRCB_420_SP ||
           s.format == ::aidl::android::hardware::graphics::common::PixelFormat::YV12;
}

ndk::ScopedAStatus CameraDeviceSessionShim::configureStreams(
    const StreamConfiguration& requestedConfiguration, std::vector<HalStream>* _aidl_return) {
    std::lock_guard<std::mutex> lock(mLock);
    if (!openHalDeviceLocked()) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    // A newly configured session is a fresh start: stop the flush-abort
    // window so its preview results are delivered normally.
    mFlushAbortTime.store(0);
    if (!mWatchdogThread.joinable()) {
        mWatchdogExit.store(false);
        mWatchdogThread = std::thread([this]() { watchdogLoop(); });
    }
    _aidl_return->clear();
    mPreviewStreamId = -1;
    mJpegStreamId = -1;
    // A new configuration drops all pending buffers (their frames are gone).
    {
        std::lock_guard<std::mutex> qlock(mQueueLock);
        mPendingExtra.clear();
        mExtraStreams.clear();
        ++mSessionGeneration;
    }
    // A re-opened session must not inherit pending entries from a previous
    // one: the HAL delivers preview callbacks with index=0 and the shim pops
    // the oldest entry, so a stale {buffer 0, frame 0} from an old session
    // makes Camera3 reject the result ("non-existent frame number 0").
    mPendingPreview.clear();
    mPendingJpeg.clear();
    mLastPreviewFrame.clear();
    for (const auto& s : requestedConfiguration.streams) {
        ALOGI("shim: stream req id=%d fmt=0x%x w=%d h=%d usage=0x%x", s.id,
              static_cast<int>(s.format), s.width, s.height,
              static_cast<int>(s.usage));
        HalStream hal;
        hal.id = s.id;
        hal.overrideDataSpace = ::aidl::android::hardware::graphics::common::Dataspace::UNKNOWN;
        hal.physicalCameraId = "";
        hal.supportOffline = false;
        hal.enableHalBufferManager = false;
        if (s.format == ::aidl::android::hardware::graphics::common::PixelFormat::BLOB) {
            mJpegStreamId = s.id;
            mJpegWidth = s.width;
            mJpegHeight = s.height;
            hal.maxBuffers = 1;
            hal.producerUsage = static_cast<::aidl::android::hardware::graphics::common::BufferUsage>(0L);
            hal.overrideFormat = ::aidl::android::hardware::graphics::common::PixelFormat::BLOB;
            // The legacy HAL's snapshot stream is configured from the
            // "picture-size" parameter, which defaults to a small size;
            // set it to the JPEG stream's size so captures come out at the
            // requested resolution.
            std::string p = "picture-size=" + std::to_string(s.width) + "x" +
                            std::to_string(s.height) + ";";
            setHalParametersLocked(p);
        } else if (streamIsPreview(s) && mPreviewStreamId < 0) {
            mPreviewStreamId = s.id;
            mPreviewWidth = s.width;
            mPreviewHeight = s.height;
            mPreviewFormat = kHALPixelFormatNV21;
            // Keep more buffers in the stream than the app's consumer can
            // hold: when the app stops rendering (camera switch/close) it
            // keeps its acquired preview images, and a 2-buffer stream
            // starves the framework's request thread (getBuffer blocks ->
            // drain times out for the full 5s -> forced error-state
            // teardown corrupts the kernel stream state). With a larger
            // pool the repeating requests keep flowing and complete with
            // ERROR buffers after the flush (see flush()), so the drain
            // finishes promptly and the switch is clean.
            hal.maxBuffers = 4;
            hal.producerUsage = static_cast<::aidl::android::hardware::graphics::common::BufferUsage>(48L);
            // RGBA buffers so the app's GL (skia EGLImage) can render them on
            // the Adreno 320; the shim converts NV21 -> RGBA when filling.
            hal.overrideFormat = ::aidl::android::hardware::graphics::common::PixelFormat::RGBA_8888;
        } else {
            // Secondary non-JPEG stream (MediaRecorder encoder surface,
            // snapshot ImageReader, extra GL target). Filled from the last
            // preview frame on each HAL preview callback; never driven
            // through the legacy HAL's record channel (starting it wedges
            // this daemon).
            const bool encoder = static_cast<int64_t>(s.usage) & 0x10000;
            ExtraStream ex{};
            ex.id = s.id;
            ex.width = s.width;
            ex.height = s.height;
            ex.yuv = s.format != ::aidl::android::hardware::graphics::common::PixelFormat::
                                    IMPLEMENTATION_DEFINED ||
                     encoder;  // YUV requests and encoder surfaces get NV12
            ex.rgba = !ex.yuv;
            mExtraStreams.push_back(ex);
            hal.maxBuffers = 4;
            hal.producerUsage =
                    static_cast<::aidl::android::hardware::graphics::common::BufferUsage>(48L);
            // Allocate a concrete format we can write linearly: flexible YUV
            // (NV12 bytes) for encoder/YUV consumers, RGBA for the rest.
            hal.overrideFormat =
                    ex.yuv ? ::aidl::android::hardware::graphics::common::PixelFormat::YCBCR_420_888
                           : ::aidl::android::hardware::graphics::common::PixelFormat::RGBA_8888;
            ALOGI("shim: extra stream id=%d %dx%d yuv=%d enc=%d", ex.id, ex.width, ex.height,
                  ex.yuv ? 1 : 0, encoder ? 1 : 0);
        }
        _aidl_return->push_back(hal);
    }
    mConfigured = true;
    return ndk::ScopedAStatus::ok();
}


ndk::ScopedAStatus CameraDeviceSessionShim::configureStreamsV2(
    const StreamConfiguration& requestedConfiguration, ConfigureStreamsRet* _aidl_return) {
    std::vector<HalStream> halStreams;
    ndk::ScopedAStatus status = configureStreams(requestedConfiguration, &halStreams);
    if (status.isOk()) {
        _aidl_return->halStreams = std::move(halStreams);
    }
    return status;
}
ndk::ScopedAStatus CameraDeviceSessionShim::constructDefaultRequestSettings(
    RequestTemplate type, CameraMetadata* _aidl_return) {
    if (mDevice != nullptr) {
        // The legacy HAL needs the device opened for the defaults to apply,
        // which the cameraserver does before requesting templates.
        if (mDevice->ops->get_parameters != nullptr) {
            mDevice->ops->get_parameters(mDevice);
        }
    }
    camera_metadata_t* m = allocate_camera_metadata(32, 512);
    // Video templates drive the record path: continuous-video AF keeps the
    // focus hunting behavior apps expect while recording, and the capture
    // intent tells the framework this session targets the encoder.
    const bool videoTemplate = (type == RequestTemplate::VIDEO_RECORD ||
                                type == RequestTemplate::VIDEO_SNAPSHOT);
    const int32_t captureIntent = videoTemplate
            ? (type == RequestTemplate::VIDEO_RECORD
                       ? ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_RECORD
                       : ANDROID_CONTROL_CAPTURE_INTENT_VIDEO_SNAPSHOT)
            : ANDROID_CONTROL_CAPTURE_INTENT_PREVIEW;
    add_camera_metadata_entry(m, ANDROID_CONTROL_CAPTURE_INTENT, &captureIntent, 1);
    const int32_t controlMode = ANDROID_CONTROL_MODE_AUTO;
    add_camera_metadata_entry(m, ANDROID_CONTROL_MODE, &controlMode, 1);
    const int32_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AE_MODE, &aeMode, 1);
    const int32_t afMode = videoTemplate ? ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO
                                         : ANDROID_CONTROL_AF_MODE_AUTO;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AF_MODE, &afMode, 1);
    const int32_t awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AWB_MODE, &awbMode, 1);
    const int32_t fpsRange[] = {30000, 30000};
    add_camera_metadata_entry(m, ANDROID_CONTROL_AE_TARGET_FPS_RANGE, fpsRange, 2);
    const int32_t effectMode = ANDROID_CONTROL_EFFECT_MODE_OFF;
    add_camera_metadata_entry(m, ANDROID_CONTROL_EFFECT_MODE, &effectMode, 1);
    const int32_t antibandingMode = ANDROID_CONTROL_AE_ANTIBANDING_MODE_AUTO;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AE_ANTIBANDING_MODE, &antibandingMode, 1);
    const int32_t sceneMode = ANDROID_CONTROL_SCENE_MODE_DISABLED;
    add_camera_metadata_entry(m, ANDROID_CONTROL_SCENE_MODE, &sceneMode, 1);
    const int32_t expComp = 0;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &expComp, 1);
    const uint8_t aeLock = ANDROID_CONTROL_AE_LOCK_OFF;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AE_LOCK, &aeLock, 1);
    const uint8_t awbLock = ANDROID_CONTROL_AWB_LOCK_OFF;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AWB_LOCK, &awbLock, 1);
    const int32_t flashMode = ANDROID_FLASH_MODE_OFF;
    add_camera_metadata_entry(m, ANDROID_FLASH_MODE, &flashMode, 1);
    const uint8_t metadataMode = ANDROID_REQUEST_METADATA_MODE_FULL;
    add_camera_metadata_entry(m, ANDROID_REQUEST_METADATA_MODE, &metadataMode, 1);
    const uint8_t afTrigger = ANDROID_CONTROL_AF_TRIGGER_IDLE;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AF_TRIGGER, &afTrigger, 1);
    const float focusDistance = 0.0f;
    add_camera_metadata_entry(m, ANDROID_LENS_FOCUS_DISTANCE, &focusDistance, 1);
    const uint8_t faceDetect = ANDROID_STATISTICS_FACE_DETECT_MODE_OFF;
    add_camera_metadata_entry(m, ANDROID_STATISTICS_FACE_DETECT_MODE, &faceDetect, 1);
    size_t size = get_camera_metadata_size(m);
    _aidl_return->metadata.resize(size);
    memcpy(_aidl_return->metadata.data(), m, size);
    free_camera_metadata(m);
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceSessionShim::flush() {
    std::lock_guard<std::mutex> lock(mLock);
    if (mDevice != nullptr && mPreviewRunning) {
        mDevice->ops->cancel_picture(mDevice);
    }
    // The app stops consuming preview buffers while closing/switching
    // cameras (its renderer thread quits before CameraDevice.close()).
    // The framework's repeating loop keeps resubmitting requests whose
    // OK-status results then block on the full producer queue, stalling
    // the shim's callback thread, the HAL frame dispatch and finally the
    // framework's drain for the full 5s timeout. The forced error-state
    // teardown then leaves the kernel stream state dirty for the next
    // camera (QBUF EINVAL, dead preview). Deliver the preview buffers as
    // ERROR for a short window after a flush: the framework returns them
    // to the stream without touching the consumer, so nothing blocks and
    // the drain completes promptly. The window expires so a session that
    // resumes normally keeps receiving real frames.
    mFlushAbortTime.store(systemTime(SYSTEM_TIME_MONOTONIC));
    ALOGI("shim: DBG flush() called");
    // Return every pending buffer as an error result so the framework's
    // inflight tracking drains; otherwise waitUntilDrained times out when
    // the app closes the session right after a shot.
    std::vector<PendingBuffer> drain;
    {
        std::lock_guard<std::mutex> qlock(mQueueLock);
        drain.assign(mPendingPreview.begin(), mPendingPreview.end());
        drain.insert(drain.end(), mPendingJpeg.begin(), mPendingJpeg.end());
        mPendingPreview.clear();
        mPendingJpeg.clear();
    }
    for (const auto& p : drain) {
        CaptureResult r;
        r.frameNumber = p.frameNumber;
        r.fmqResultSize = 0;
        r.partialResult = 1;
        r.inputBuffer.streamId = -1;
        // A result without metadata never completes the request in the
        // framework's inflight tracker, so waitUntilDrained keeps waiting
        // until it times out (-110) and the app dies. Attach the minimum
        // metadata blob.
        const int64_t ts = systemTime(SYSTEM_TIME_MONOTONIC);
        camera_metadata_t* fm = allocate_camera_metadata(4, 64);
        add_camera_metadata_entry(fm, ANDROID_SENSOR_TIMESTAMP, &ts, 1);
        const int32_t aeState = ANDROID_CONTROL_AE_STATE_INACTIVE;
        add_camera_metadata_entry(fm, ANDROID_CONTROL_AE_STATE, &aeState, 1);
        const int32_t afState = ANDROID_CONTROL_AF_STATE_INACTIVE;
        add_camera_metadata_entry(fm, ANDROID_CONTROL_AF_STATE, &afState, 1);
        size_t metaSize = get_camera_metadata_size(fm);
        r.result.metadata.resize(metaSize);
        memcpy(r.result.metadata.data(), fm, metaSize);
        free_camera_metadata(fm);
        StreamBuffer b{};
        b.streamId = mPreviewStreamId;
        b.bufferId = p.bufferId;
        b.status = BufferStatus::ERROR;
        r.outputBuffers.push_back(std::move(b));
        // The framework only completes an inflight request after its shutter
        // notification arrives ("flushInflightRequests: Frame N | Timestamp:
        // 0" == held forever). Without this the drain always runs the full
        // 5s timeout and tears the session down in an error state.
        sendShutterNotify(p.frameNumber, ts);
        std::vector<CaptureResult> results;
        results.push_back(std::move(r));
        mCallback->processCaptureResult(results);
    }
    // Extra-stream buffers: their frames' shutters went out with the preview
    // part (or in the drain above); return them as ERROR parts so those
    // frames complete too.
    std::map<int32_t, std::vector<PendingBuffer>> extraDrain;
    {
        std::lock_guard<std::mutex> qlock(mQueueLock);
        for (auto& [sid, q] : mPendingExtra) {
            extraDrain[sid].assign(q.begin(), q.end());
        }
        mPendingExtra.clear();
    }
    for (auto& [sid, vec] : extraDrain) {
        for (const auto& p : vec) {
            CaptureResult r;
            r.frameNumber = p.frameNumber;
            r.fmqResultSize = 0;
            r.partialResult = 1;
            r.inputBuffer.streamId = -1;
            const int64_t ts = systemTime(SYSTEM_TIME_MONOTONIC);
            camera_metadata_t* fm = allocate_camera_metadata(4, 64);
            add_camera_metadata_entry(fm, ANDROID_SENSOR_TIMESTAMP, &ts, 1);
            size_t metaSize = get_camera_metadata_size(fm);
            r.result.metadata.resize(metaSize);
            memcpy(r.result.metadata.data(), fm, metaSize);
            free_camera_metadata(fm);
            StreamBuffer b{};
            b.streamId = sid;
            b.bufferId = p.bufferId;
            b.status = BufferStatus::ERROR;
            r.outputBuffers.push_back(std::move(b));
            std::vector<CaptureResult> results;
            results.push_back(std::move(r));
            mCallback->processCaptureResult(results);
        }
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceSessionShim::getCaptureRequestMetadataQueue(
    ::aidl::android::hardware::common::fmq::MQDescriptor<
        int8_t, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>*
        _aidl_return) {
    *_aidl_return = ::aidl::android::hardware::common::fmq::MQDescriptor<
            int8_t, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceSessionShim::getCaptureResultMetadataQueue(
    ::aidl::android::hardware::common::fmq::MQDescriptor<
        int8_t, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>*
        _aidl_return) {
    *_aidl_return = ::aidl::android::hardware::common::fmq::MQDescriptor<
            int8_t, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceSessionShim::isReconfigurationRequired(
    const CameraMetadata& /*oldSessionParams*/, const CameraMetadata& /*newSessionParams*/,
    bool* _aidl_return) {
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceSessionShim::processCaptureRequest(
    const std::vector<CaptureRequest>& requests,
    const std::vector<BufferCache>& /*cachesToRemove*/, int32_t* _aidl_return) {
    std::unique_lock<std::mutex> lock(mLock);
    if (mClosed) {
        *_aidl_return = -1;
        return ndk::ScopedAStatus::ok();
    }
    for (const auto& request : requests) {
        processRequest(lock, request);
    }
    *_aidl_return = static_cast<int32_t>(requests.size());
    return ndk::ScopedAStatus::ok();
}

void CameraDeviceSessionShim::processRequest(std::unique_lock<std::mutex>& lock,
                                             const CaptureRequest& request) {
    if (!openHalDeviceLocked()) {
        return;
    }
    ALOGI("shim: DBG req frame=%d bufs=%zu settings=%zu", request.frameNumber,
          request.outputBuffers.size(), request.settings.metadata.size());
    mActiveRequestId = request.frameNumber;
    const uint32_t frameNumber = mNextFrameNumber++;

    // Parse request settings into HAL 1.0 parameters.
    std::string params;
    if (!request.settings.metadata.empty()) {
        camera_metadata_t* m = reinterpret_cast<camera_metadata_t*>(
                const_cast<uint8_t*>(request.settings.metadata.data()));
        camera_metadata_ro_entry_t entry;
        if (find_camera_metadata_ro_entry(m, ANDROID_CONTROL_AF_MODE, &entry) == 0 &&
            entry.count > 0) {
            const char* focusMode = "auto";
            switch (entry.data.u8[0]) {
                case ANDROID_CONTROL_AF_MODE_OFF:
                    focusMode = "infinity";
                    break;
                case ANDROID_CONTROL_AF_MODE_MACRO:
                    focusMode = "macro";
                    break;
                case ANDROID_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
                    focusMode = "continuous-video";
                    break;
                case ANDROID_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
                    focusMode = "continuous-picture";
                    break;
                default:
                    focusMode = "auto";
                    break;
            }
            params += "focus-mode=";
            params += focusMode;
            params += ";";
        }
        if (find_camera_metadata_ro_entry(m, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION,
                                          &entry) == 0 &&
            entry.count > 0) {
            params += "exposure-compensation=" + std::to_string(entry.data.i32[0] / 100);
            params += ";";
        }
        if (find_camera_metadata_ro_entry(m, ANDROID_FLASH_MODE, &entry) == 0 &&
            entry.count > 0) {
            ALOGI("shim: request FLASH_MODE=%u", entry.data.u8[0]);
            const char* flashMode = "off";
            switch (entry.data.u8[0]) {
                case ANDROID_FLASH_MODE_SINGLE:
                    // The HAL's flash condition only handles the ZSL path
                    // (getFlashCondition returns false in non-ZSL mode), so
                    // "flash-mode=on" never actuates the lm3559. Drive the
                    // spotlight sysfs directly and let the AE re-settle with
                    // the LED on (see take_picture below).
                    mFlashRequested.store(true);
                    flashMode = "off";
                    break;
                case ANDROID_FLASH_MODE_TORCH:
                    flashMode = "torch";
                    break;
                default:
                    flashMode = "off";
                    break;
            }
            params += "flash-mode=";
            params += flashMode;
            params += ";";
        }
        if (find_camera_metadata_ro_entry(m, ANDROID_JPEG_ORIENTATION, &entry) == 0 &&
            entry.count > 0) {
            // The legacy HAL feeds the JPEG encoder from mRotation, which is
            // only set by the camera1 "rotation" parameter; "jpeg-rotation"
            // alone is ignored, leaving every capture in sensor orientation.
            params += "rotation=" + std::to_string(entry.data.i32[0]);
            params += ";";
            params += "jpeg-rotation=" + std::to_string(entry.data.i32[0]);
            params += ";";
        }
        if (find_camera_metadata_ro_entry(m, ANDROID_CONTROL_AE_MODE, &entry) == 0 &&
            entry.count > 0) {
            ALOGI("shim: request AE_MODE=%u", entry.data.u8[0]);
            // Camera2 apps set flash via AE_MODE (ON_ALWAYS_FLASH), not the
            // deprecated FLASH_MODE tag. Map it so the legacy HAL commands
            // the lm3559 during still capture.
            const char* aeFlashMode = nullptr;
            switch (entry.data.u8[0]) {
                case ANDROID_CONTROL_AE_MODE_ON_ALWAYS_FLASH:
                    mFlashRequested.store(true);
                    aeFlashMode = "off";
                    break;
                case ANDROID_CONTROL_AE_MODE_ON_AUTO_FLASH:
                    aeFlashMode = "auto";
                    break;
                default:
                    break;
            }
            if (aeFlashMode != nullptr) {
                params += "flash-mode=";
                params += aeFlashMode;
                params += ";";
            }
        }
    }
    if (!params.empty()) {
        setHalParametersLocked(params);
    }

    // Apply AF trigger if requested.
    if (!request.settings.metadata.empty()) {
        camera_metadata_t* m = reinterpret_cast<camera_metadata_t*>(
                const_cast<uint8_t*>(request.settings.metadata.data()));
        camera_metadata_ro_entry_t entry;
        if (find_camera_metadata_ro_entry(m, ANDROID_CONTROL_AF_TRIGGER, &entry) == 0 &&
            entry.count > 0 && entry.data.u8[0] == ANDROID_CONTROL_AF_TRIGGER_START) {
            if (mDevice->ops->auto_focus != nullptr) {
                mDevice->ops->auto_focus(mDevice);
            }
        }
    }

    // Detect a capture request FIRST and set the flag before pushing any
    // buffers: a preview callback racing the buffer push would otherwise
    // deliver the capture frame's preview shutter early (out-of-order vs
    // the JPEG shutter) and duplicate its metadata.
    for (const auto& buf : request.outputBuffers) {
        if (buf.streamId == mJpegStreamId) {
            std::lock_guard<std::mutex> qlock(mQueueLock);
            mCaptureInFlight = true;
            mCaptureInFlightSince = systemTime(SYSTEM_TIME_MONOTONIC);
            break;
        }
    }

    bool hasPreviewBuffer = false;
    for (const auto& buf : request.outputBuffers) {
        if (buf.streamId == mPreviewStreamId) {
            hasPreviewBuffer = true;
        }
        if (!buf.buffer.fds.empty()) {
            native_handle_t* h = toNativeHandle(buf.buffer);
            if (h != nullptr) {
                mBufferHandles[buf.bufferId] = h;
                ALOGI("shim: stored buffer %lld fds=%d ints=%d (fd0=%d)",
                      static_cast<long long>(buf.bufferId),
                      static_cast<int>(buf.buffer.fds.size()),
                      static_cast<int>(buf.buffer.ints.size()),
                      (buf.buffer.fds.size() > 0) ? buf.buffer.fds[0].get() : -2);
            }
        }
        {
            std::lock_guard<std::mutex> qlock(mQueueLock);
            if (buf.streamId == mPreviewStreamId) {
                mPendingPreview.push_back({buf.bufferId,
                                           static_cast<uint32_t>(request.frameNumber),
                                           systemTime(SYSTEM_TIME_MONOTONIC)});
            } else if (buf.streamId == mJpegStreamId) {
                mPendingJpeg.push_back({buf.bufferId,
                                        static_cast<uint32_t>(request.frameNumber),
                                        systemTime(SYSTEM_TIME_MONOTONIC)});
            } else if (mPendingExtra.count(buf.streamId) > 0 ||
                       [&]() {
                           for (const auto& ex : mExtraStreams) {
                               if (ex.id == buf.streamId) return true;
                           }
                           return false;
                       }()) {
                mPendingExtra[buf.streamId].push_back({buf.bufferId,
                                                       static_cast<uint32_t>(
                                                               request.frameNumber),
                                                       systemTime(SYSTEM_TIME_MONOTONIC)});
            }
        }
    }

    // Start preview on the first preview request.
    if (mPreviewStreamId >= 0 && hasPreviewBuffer && !mPreviewRunning) {
        // A detached post-shot stop_preview may still be in flight; starting
        // preview now would be torn down the moment the stop lands (VFE
        // killed mid-frame -> no more frames -> the framework's repeating
        // request stalls forever because it only resubmits after the
        // previous result completes). Wait for the stop to finish first.
        // IMPORTANT: release mLock while waiting -- the detached stop thread
        // takes mLock for its session-generation check, so holding mLock
        // across this wait deadlocks both threads (the stop can never clear
        // mStopPending and every later camera request hangs).
        std::unique_lock<std::mutex> qlock(mQueueLock);
        if (mStopPending) {
            lock.unlock();
            mStopCv.wait(qlock, [this]() { return !mStopPending; });
            qlock.unlock();
            lock.lock();
            // close() may have run while we waited; the request's buffers
            // were already pushed and close() drained them.
            if (mClosed || mDevice == nullptr || mPreviewRunning) {
                return;
            }
        } else {
            qlock.unlock();
        }
        if (chooseHalPreviewSizeLocked()) {
            std::string p = "preview-size=" + std::to_string(mHalPreviewWidth) + "x" +
                            std::to_string(mHalPreviewHeight) + ";";
            setHalParametersLocked(p);
        }
        if (mDevice->ops->start_preview != nullptr) {
            mDevice->ops->start_preview(mDevice);
            mPreviewRunning = true;
        }
    }

    // Handle still capture.
    for (const auto& buf : request.outputBuffers) {
        if (buf.streamId == mJpegStreamId) {
            // A detached post-shot stop_preview may still be in flight; it
            // tears the VFE down mid-frame if it lands during the new
            // capture (kernel IOMMU fault -> watchdog reboot). Wait for it
            // before take_picture. Release mLock while waiting (see the
            // preview-restart wait above -- the stop thread needs mLock).
            std::unique_lock<std::mutex> qlock(mQueueLock);
            if (mStopPending) {
                lock.unlock();
                mStopCv.wait(qlock, [this]() { return !mStopPending; });
                qlock.unlock();
                lock.lock();
                if (mClosed || mDevice == nullptr) {
                    return;
                }
            } else {
                qlock.unlock();
            }
            if (mJpegWidth > 0 && mJpegHeight > 0) {
                std::string p = "picture-size=" + std::to_string(mJpegWidth) +
                                "x" + std::to_string(mJpegHeight) + ";";
                setHalParametersLocked(p);
            }
            if (mFlashRequested.load()) {
                ::android::base::WriteStringToFile(
                        "255", "/sys/class/leds/spotlight/brightness");
                // Let the preview AE loop re-settle with the LED on, so the
                // snapshot exposure is metered for the flash-lit scene.
                usleep(500000);
            }
            if (mDevice->ops->take_picture != nullptr) {
                mDevice->ops->take_picture(mDevice);
            }
        }
    }
}

bool CameraDeviceSessionShim::fillOutputBuffer(int64_t bufferId, int32_t streamId,
                                               const void* data, size_t size) {
    auto it = mBufferHandles.find(bufferId);
    if (it == mBufferHandles.end() || it->second == nullptr || data == nullptr) {
        return false;
    }
    if (mGralloc == nullptr) {
        const hw_module_t* mod = nullptr;
        if (hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &mod) != 0) {
            ALOGE("shim: gralloc module not found");
            return false;
        }
        mGralloc = reinterpret_cast<const gralloc_module_t*>(mod);
    }
    if (mGralloc->lock == nullptr || mGralloc->unlock == nullptr) {
        return false;
    }
    int width = (streamId == mJpegStreamId) ? mJpegWidth : mPreviewWidth;
    int height = (streamId == mJpegStreamId) ? mJpegHeight : mPreviewHeight;
    for (const auto& ex : mExtraStreams) {
        if (ex.id == streamId) {
            width = ex.width;
            height = ex.height;
        }
    }
    // The handle's 'base' field carries the allocating process's mapping
    // address; clear it so gralloc performs a real map in our process.
    native_handle_t* nh = it->second;
    if (nh->numFds == 2 && nh->numInts == 12 && nh->data[2] == 0x676d736d /* 'gmsm' */) {
        nh->data[nh->numFds + 4] = 0;  // private_handle_t::base
    }
    void* vaddr = nullptr;
    // The gralloc lock path fails on this buffer (the legacy ion cache ops
    // reject the cross-process handle), so map the graphics fd directly.
    size_t bufSize = 0;
    if (nh->numInts >= 3) {
        bufSize = static_cast<size_t>(nh->data[nh->numFds + 2]);
    }
    ALOGI("shim: buf %lld size=%zu ints=%d: %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x %08x",
          static_cast<long long>(bufferId), bufSize, nh->numInts,
          (nh->numInts > 0) ? static_cast<unsigned>(nh->data[nh->numFds + 0]) : 0u,
          (nh->numInts > 1) ? static_cast<unsigned>(nh->data[nh->numFds + 1]) : 0u,
          (nh->numInts > 2) ? static_cast<unsigned>(nh->data[nh->numFds + 2]) : 0u,
          (nh->numInts > 3) ? static_cast<unsigned>(nh->data[nh->numFds + 3]) : 0u,
          (nh->numInts > 4) ? static_cast<unsigned>(nh->data[nh->numFds + 4]) : 0u,
          (nh->numInts > 5) ? static_cast<unsigned>(nh->data[nh->numFds + 5]) : 0u,
          (nh->numInts > 6) ? static_cast<unsigned>(nh->data[nh->numFds + 6]) : 0u,
          (nh->numInts > 7) ? static_cast<unsigned>(nh->data[nh->numFds + 7]) : 0u,
          (nh->numInts > 8) ? static_cast<unsigned>(nh->data[nh->numFds + 8]) : 0u,
          (nh->numInts > 9) ? static_cast<unsigned>(nh->data[nh->numFds + 9]) : 0u,
          (nh->numInts > 10) ? static_cast<unsigned>(nh->data[nh->numFds + 10]) : 0u,
          (nh->numInts > 11) ? static_cast<unsigned>(nh->data[nh->numFds + 11]) : 0u);
    if (nh->numFds > 0) {
        vaddr = mmap(nullptr, bufSize, PROT_READ | PROT_WRITE, MAP_SHARED,
                     nh->data[0], 0);
    }
    if (vaddr == nullptr || vaddr == MAP_FAILED) {
        ALOGE("shim: mmap failed for buffer %lld size=%zu errno=%d",
              static_cast<long long>(bufferId), bufSize, errno);
        if (vaddr != nullptr && vaddr != MAP_FAILED) {
            munmap(vaddr, bufSize);
        }
        return false;
    }
    if (streamId == mPreviewStreamId) {
        // The HAL's preview callback delivers the camera daemon's NV21 frame
        // (written into the display window buffers). Derive the real frame
        // geometry from the private handles: src = the window buffer's dims,
        // dst = the app buffer's dims. The converter center-crops to the
        // destination aspect (camera2 scaler semantics) — a plain stretch
        // distorts the viewfinder whenever stream and HAL aspects differ.
        int srcW = mHalPreviewWidth;
        int srcH = mHalPreviewHeight;
        if (srcW <= 0) srcW = 640;
        if (srcH <= 0) srcH = 480;
        if (!mShimWindow.handles.empty() && mShimWindow.handles[0] != nullptr) {
            const native_handle_t* wh = mShimWindow.handles[0];
            if (wh->numInts >= 11) {
                srcW = wh->data[wh->numFds + 9];
                srcH = wh->data[wh->numFds + 10];
            }
        }
        int dstW = mPreviewWidth;
        int dstH = mPreviewHeight;
        if (nh->numInts >= 11) {
            dstW = nh->data[nh->numFds + 9];
            dstH = nh->data[nh->numFds + 10];
        }
        ALOGI("shim: convert %dx%d -> %dx%d", srcW, srcH, dstW, dstH);
        nv21ToRgba(static_cast<const uint8_t*>(data), static_cast<uint8_t*>(vaddr),
                   srcW, srcH, dstW, dstH);
    } else if (streamId == mJpegStreamId) {
        memcpy(vaddr, data, size);
        // Append the camera3 JPEG blob footer at the end of the buffer so the
        // app's ImageReader can read the JPEG size (AIDL CameraBlob format:
        // {jpeg_blob_id = 0x00FF, jpeg_size}).
        if (nh->numInts >= 10) {
            uint32_t blobWidth = static_cast<uint32_t>(nh->data[nh->numFds + 9]);
            if (blobWidth >= 8) {
                uint8_t* footer = static_cast<uint8_t*>(vaddr) + blobWidth - 8;
                uint32_t id = 0x00FF;  // CAMERA3_JPEG_BLOB_ID (AIDL CameraBlobId::JPEG)
                uint32_t jsz = static_cast<uint32_t>(size);
                memcpy(footer, &id, 4);
                memcpy(footer + 4, &jsz, 4);
            }
        }
    } else {
        // Extra stream (or unknown): convert from the last preview frame.
        // Encoder/YUV consumers get NV12, image/GL consumers get RGBA.
        const ExtraStream* ex = nullptr;
        for (const auto& e : mExtraStreams) {
            if (e.id == streamId) {
                ex = &e;
            }
        }
        int srcW = mHalPreviewWidth;
        int srcH = mHalPreviewHeight;
        if (srcW <= 0) srcW = 640;
        if (srcH <= 0) srcH = 480;
        if (!mShimWindow.handles.empty() && mShimWindow.handles[0] != nullptr) {
            const native_handle_t* wh = mShimWindow.handles[0];
            if (wh->numInts >= 11) {
                srcW = wh->data[wh->numFds + 9];
                srcH = wh->data[wh->numFds + 10];
            }
        }
        if (ex == nullptr) {
            memcpy(vaddr, data, std::min(size, bufSize));
        } else {
            const uint8_t* src = static_cast<const uint8_t*>(data);
            size_t need = static_cast<size_t>(srcW) * srcH * 3 / 2;
            if (mLastPreviewFrame.size() >= need) {
                src = mLastPreviewFrame.data();
            }
            int dstW = width;
            int dstH = height;
            if (nh->numInts >= 11) {
                dstW = nh->data[nh->numFds + 9];
                dstH = nh->data[nh->numFds + 10];
            }
            ALOGI("shim: extra convert id=%d %dx%d -> %dx%d yuv=%d", ex->id, srcW, srcH,
                  dstW, dstH, ex->yuv ? 1 : 0);
            if (ex->yuv) {
                nv21ToNv12(src, static_cast<uint8_t*>(vaddr), srcW, srcH, dstW, dstH);
            } else {
                nv21ToRgba(src, static_cast<uint8_t*>(vaddr), srcW, srcH, dstW, dstH);
            }
        }
    }
    // Make the CPU writes visible to the consumer (the framework does not
    // perform cache maintenance for buffers we map directly).
    shimCleanCache(nh, vaddr, bufSize);
    const uint8_t* chk = static_cast<const uint8_t*>(vaddr);
    ALOGI("shim: filled b=%lld first=[%02x %02x %02x %02x]", static_cast<long long>(bufferId),
          chk[0], chk[1], chk[2], chk[3]);
    munmap(vaddr, bufSize);
    return true;
}

void CameraDeviceSessionShim::handlePreviewBuffer(const void* data, size_t size) {
    if (mCallback == nullptr) {
        return;
    }
    PendingBuffer pending{};
    {
        std::lock_guard<std::mutex> qlock(mQueueLock);
        if (mPendingPreview.empty()) {
            ALOGI("shim: DBG preview cb dropped (queue empty) size=%zu", size);
            return;
        }
        ALOGI("shim: DBG preview cb queued size=%zu front=%u/%u q=%zu",
              size, mPendingPreview.front().frameNumber,
              mPendingPreview.front().bufferId, mPendingPreview.size());
        mLastPreviewFrame.assign(static_cast<const uint8_t*>(data),
                                 static_cast<const uint8_t*>(data) + size);
        if (mCaptureInFlight) {
            // Late frames from the stopped preview stream arrive while the
            // JPEG is encoding. Delivering them here puts the shutter of a
            // later frame before the capture frame's JPEG shutter and kills
            // the Camera3 session (out-of-order). The JPEG handler flushes
            // the pending queue with postview data instead.
            // If the JPEG never arrives (failed/aborted capture), clear the
            // flag after a few seconds so the preview resumes instead of
            // staying frozen forever.
            if (systemTime(SYSTEM_TIME_MONOTONIC) - mCaptureInFlightSince >
                5000000000LL) {
                mCaptureInFlight = false;
            } else {
                return;
            }
        }
        pending = mPendingPreview.front();
        mPendingPreview.pop_front();
    }
    const uint8_t* dbg = static_cast<const uint8_t*>(data);
    ALOGI("shim: preview frame %d into buffer %d cb_size=%zu win=%dx%d pv=%dx%d first=[%d %d %d %d %d %d]",
          pending.frameNumber, pending.bufferId, size, mShimWindow.width, mShimWindow.height,
          mPreviewWidth, mPreviewHeight,
          (size > 6) ? dbg[0] : -1, (size > 6) ? dbg[1] : -1, (size > 6) ? dbg[2] : -1,
          (size > 6) ? dbg[3] : -1, (size > 6) ? dbg[4] : -1, (size > 6) ? dbg[5] : -1);
    fillOutputBuffer(pending.bufferId, mPreviewStreamId, data, size);
    // The framework only returns filled output buffers to the app's consumer
    // after the shutter notification for the frame arrives, and requires the
    // shutter timestamp to match the result's sensor timestamp.
    const int64_t frameTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);
    sendShutterNotify(pending.frameNumber, frameTimestamp);

    CaptureResult result;
    result.frameNumber = pending.frameNumber;
    result.fmqResultSize = 0;
    result.partialResult = 1;
    result.inputBuffer.streamId = -1;
    camera_metadata_t* m = allocate_camera_metadata(8, 128);
    add_camera_metadata_entry(m, ANDROID_SENSOR_TIMESTAMP, &frameTimestamp, 1);
    const int32_t aeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AE_STATE, &aeState, 1);
    const int32_t afState = ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AF_STATE, &afState, 1);
    const int32_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AE_MODE, &aeMode, 1);
    const int32_t afMode = ANDROID_CONTROL_AF_MODE_AUTO;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AF_MODE, &afMode, 1);
    const int32_t awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AWB_MODE, &awbMode, 1);
    const int32_t flashMode = ANDROID_FLASH_MODE_OFF;
    add_camera_metadata_entry(m, ANDROID_FLASH_MODE, &flashMode, 1);
    size_t metaSize = get_camera_metadata_size(m);
    result.result.metadata.resize(metaSize);
    memcpy(result.result.metadata.data(), m, metaSize);
    free_camera_metadata(m);
    StreamBuffer out{};
    out.streamId = mPreviewStreamId;
    out.bufferId = pending.bufferId;
    out.status = flushAbortedDelivery() ? BufferStatus::ERROR : BufferStatus::OK;
    result.outputBuffers.push_back(std::move(out));
    // Drain one pending buffer per extra stream (encoder surface,
    // ImageReaders, ...) into this same frame's result: one request carries
    // all its buffers and the framework completes the frame only when every
    // part has arrived.
    {
        std::lock_guard<std::mutex> qlock(mQueueLock);
        for (auto& [sid, q] : mPendingExtra) {
            if (q.empty()) {
                continue;
            }
            PendingBuffer ep = q.front();
            q.pop_front();
            fillOutputBuffer(ep.bufferId, sid, data, size);
            StreamBuffer eb{};
            eb.streamId = sid;
            eb.bufferId = ep.bufferId;
            eb.status = flushAbortedDelivery() ? BufferStatus::ERROR : BufferStatus::OK;
            result.outputBuffers.push_back(std::move(eb));
        }
    }
    std::vector<CaptureResult> results;
    results.push_back(std::move(result));
    mCallback->processCaptureResult(results);
}

void CameraDeviceSessionShim::sendShutterNotify(uint32_t frameNumber, int64_t timestamp) {
    if (mCallback == nullptr) {
        return;
    }
    NotifyMsg msg{};
    ShutterMsg shutter;
    shutter.frameNumber = frameNumber;
    // The framework requires the shutter timestamp to match the result
    // metadata's android.sensor.timestamp for the same frame.
    shutter.timestamp = timestamp;
    msg.set<NotifyMsg::shutter>(shutter);
    mCallback->notify({msg});
}

// True right after a flush(): preview buffers are returned to the
// framework as ERROR so a stalled consumer cannot block the callback
// thread (see flush()).
bool CameraDeviceSessionShim::flushAbortedDelivery() {
    const int64_t t = mFlushAbortTime.load();
    if (t == 0) {
        return false;
    }
    // The window must cover the framework's full drain (up to 5s): the
    // app holds its acquired preview images for the whole close, so an
    // OK-status delivery can block on the full producer queue at any
    // point during it. configureStreams() (the next session) resets the
    // window so a resumed session receives normal frames again.
    if (systemTime(SYSTEM_TIME_MONOTONIC) - t > 10000000000LL) {
        mFlushAbortTime.store(0);
        return false;
    }
    return true;
}

// Completes a pending entry as an ERROR result; withShutter controls whether
// the frame's shutter notification is sent (required for the framework to
// retire the inflight request; record parts rely on their preview part's
// shutter and must NOT send a second one).
void CameraDeviceSessionShim::deliverErrorResult(const PendingBuffer& p,
                                                 int32_t streamId, bool withShutter) {
    if (mCallback == nullptr) {
        return;
    }
    const int64_t ts = systemTime(SYSTEM_TIME_MONOTONIC);
    CaptureResult r;
    r.frameNumber = p.frameNumber;
    r.fmqResultSize = 0;
    r.partialResult = 1;
    r.inputBuffer.streamId = -1;
    camera_metadata_t* fm = allocate_camera_metadata(4, 64);
    add_camera_metadata_entry(fm, ANDROID_SENSOR_TIMESTAMP, &ts, 1);
    const int32_t aeState = ANDROID_CONTROL_AE_STATE_INACTIVE;
    add_camera_metadata_entry(fm, ANDROID_CONTROL_AE_STATE, &aeState, 1);
    const int32_t afState = ANDROID_CONTROL_AF_STATE_INACTIVE;
    add_camera_metadata_entry(fm, ANDROID_CONTROL_AF_STATE, &afState, 1);
    size_t metaSize = get_camera_metadata_size(fm);
    r.result.metadata.resize(metaSize);
    memcpy(r.result.metadata.data(), fm, metaSize);
    free_camera_metadata(fm);
    StreamBuffer b{};
    b.streamId = streamId;
    b.bufferId = p.bufferId;
    b.status = BufferStatus::ERROR;
    r.outputBuffers.push_back(std::move(b));
    if (withShutter) {
        sendShutterNotify(p.frameNumber, ts);
    }
    std::vector<CaptureResult> results;
    results.push_back(std::move(r));
    mCallback->processCaptureResult(results);
}

// Watchdog thread body: completes pending entries older than 1s as error
// results. Preview entries carry their frame's only shutter (sent here);
// extra-stream entries never send shutters (their preview part does). Skipped
// while a JPEG capture is in flight — the capture flow owns those frames'
// delivery order (out-of-order shutters kill the session).
void CameraDeviceSessionShim::watchdogLoop() {
    while (!mWatchdogExit.load()) {
        usleep(300000);
        if (mWatchdogExit.load()) {
            break;
        }
        std::vector<PendingBuffer> stalePreview;
        std::map<int32_t, std::vector<PendingBuffer>> staleExtra;
        {
            std::lock_guard<std::mutex> qlock(mQueueLock);
            const int64_t now = systemTime(SYSTEM_TIME_MONOTONIC);
            while (!mPendingPreview.empty() &&
                   mPendingPreview.front().enqueuedAt != 0 &&
                   now - mPendingPreview.front().enqueuedAt > 1000000000LL) {
                stalePreview.push_back(mPendingPreview.front());
                mPendingPreview.pop_front();
            }
            for (auto& [sid, q] : mPendingExtra) {
                while (!q.empty() && q.front().enqueuedAt != 0 &&
                       now - q.front().enqueuedAt > 1000000000LL) {
                    staleExtra[sid].push_back(q.front());
                    q.pop_front();
                }
            }
        }
        const bool captureInFlight = mCaptureInFlight;
        for (const auto& p : stalePreview) {
            if (captureInFlight) {
                break;  // capture flow owns ordering; leave them queued
            }
            ALOGW("shim: watchdog completing stale preview frame %d", p.frameNumber);
            deliverErrorResult(p, mPreviewStreamId, /*withShutter=*/true);
        }
        for (auto& [sid, vec] : staleExtra) {
            for (const auto& p : vec) {
                ALOGW("shim: watchdog completing stale extra frame %d stream %d",
                      p.frameNumber, sid);
                deliverErrorResult(p, sid, /*withShutter=*/false);
            }
        }
    }
}

// Completes one in-flight preview request whose preview callback never
// arrived (the HAL stops the preview stream for snapshot): fills its buffer
// with the last real frame as postview, sends its shutter notification and
// a minimal capture result. Must be called with shutters in strictly
// increasing frame order (Camera3 errors the device otherwise).
void CameraDeviceSessionShim::deliverPreviewResult(const PendingBuffer& pv) {
    if (mCallback == nullptr) {
        return;
    }
    if (!mLastPreviewFrame.empty()) {
        fillOutputBuffer(pv.bufferId, mPreviewStreamId, mLastPreviewFrame.data(),
                         mLastPreviewFrame.size());
    }
    const int64_t ts = systemTime(SYSTEM_TIME_MONOTONIC);
    sendShutterNotify(pv.frameNumber, ts);
    CaptureResult fr;
    fr.frameNumber = pv.frameNumber;
    fr.fmqResultSize = 0;
    fr.partialResult = 1;
    fr.inputBuffer.streamId = -1;
    camera_metadata_t* fm = allocate_camera_metadata(8, 128);
    add_camera_metadata_entry(fm, ANDROID_SENSOR_TIMESTAMP, &ts, 1);
    const int32_t aeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
    add_camera_metadata_entry(fm, ANDROID_CONTROL_AE_STATE, &aeState, 1);
    const int32_t afState = ANDROID_CONTROL_AF_STATE_PASSIVE_SCAN;
    add_camera_metadata_entry(fm, ANDROID_CONTROL_AF_STATE, &afState, 1);
    size_t fmetaSize = get_camera_metadata_size(fm);
    fr.result.metadata.resize(fmetaSize);
    memcpy(fr.result.metadata.data(), fm, fmetaSize);
    free_camera_metadata(fm);
    StreamBuffer fbuf{};
    fbuf.streamId = mPreviewStreamId;
    fbuf.bufferId = pv.bufferId;
    fbuf.status = flushAbortedDelivery() ? BufferStatus::ERROR : BufferStatus::OK;
    fr.outputBuffers.push_back(std::move(fbuf));
    std::vector<CaptureResult> fresults;
    fresults.push_back(std::move(fr));
    mCallback->processCaptureResult(fresults);
}

void CameraDeviceSessionShim::handleJpegBuffer(const void* data, size_t size) {
    if (mCallback == nullptr) {
        return;
    }
    if (mFlashRequested.load()) {
        mFlashRequested.store(false);
        ::android::base::WriteStringToFile(
                "0", "/sys/class/leds/spotlight/brightness");
    }
    PendingBuffer pendingJpeg{};
    PendingBuffer pendingPostview{};
    bool hasPostview = false;
    std::vector<PendingBuffer> preFlush;
    std::vector<PendingBuffer> postFlush;
    {
        std::lock_guard<std::mutex> qlock(mQueueLock);
        if (mPendingJpeg.empty()) {
            return;
        }
        pendingJpeg = mPendingJpeg.front();
        mPendingJpeg.pop_front();
        // The capture frame's own preview buffer never receives a preview
        // callback (the legacy HAL stops the preview stream for snapshot).
        // Take it out of the queue and return it in the JPEG result, filled
        // with the last real preview frame as postview, so Camera3
        // completes the frame and resumes the repeating preview requests.
        for (auto it = mPendingPreview.begin(); it != mPendingPreview.end(); ++it) {
            if (it->frameNumber == pendingJpeg.frameNumber) {
                pendingPostview = *it;
                hasPostview = true;
                mPendingPreview.erase(it);
                break;
            }
        }
        // Frames strictly OLDER than the capture frame: their preview
        // callbacks never arrive (the legacy HAL stops the preview stream
        // for snapshot), so their shutters would otherwise go out only now
        // -- AFTER the capture frame's shutter. Camera3 rejects any shutter
        // older than the last one accepted ("Shutter notification
        // out-of-order" -> CAMERA_HAL_CALLBACK_ERROR -> device error -> the
        // app dies on the capture-approval screen). Deliver them first so
        // the framework sees shutters in strictly increasing frame order.
        while (!mPendingPreview.empty() &&
               mPendingPreview.front().frameNumber < pendingJpeg.frameNumber) {
            preFlush.push_back(mPendingPreview.front());
            mPendingPreview.pop_front();
        }
        // Repeating requests submitted after the capture are still queued;
        // complete them in order, strictly before any next pending JPEG.
        const uint32_t nextJpegFrame = mPendingJpeg.empty()
                ? UINT32_MAX : mPendingJpeg.front().frameNumber;
        while (!mPendingPreview.empty() &&
               mPendingPreview.front().frameNumber < nextJpegFrame) {
            postFlush.push_back(mPendingPreview.front());
            mPendingPreview.pop_front();
        }
        if (mPendingJpeg.empty()) {
            mCaptureInFlight = false;
        }
    }
    const uint8_t* jd = static_cast<const uint8_t*>(data);
    ALOGI("shim: jpeg %d bytes into buffer %d head=[%02x %02x %02x %02x %02x %02x]",
          static_cast<int>(size), pendingJpeg.bufferId,
          (size > 0) ? jd[0] : 0, (size > 1) ? jd[1] : 0, (size > 2) ? jd[2] : 0,
          (size > 3) ? jd[3] : 0, (size > 4) ? jd[4] : 0, (size > 5) ? jd[5] : 0);
    // The HAL's JPEG stream may carry a leading byte before the SOI marker;
    // locate the FF D8 start so the app's ImageReader sees a valid header.
    const uint8_t* jpegData = static_cast<const uint8_t*>(data);
    size_t jpegSize = size;
    for (size_t i = 0; i + 1 < size && i < 16; i++) {
        if (jpegData[i] == 0xff && jpegData[i + 1] == 0xd8) {
            jpegData += i;
            jpegSize -= i;
            break;
        }
    }
    auto jit = mBufferHandles.find(pendingJpeg.bufferId);
    if (jit != mBufferHandles.end() && jit->second != nullptr) {
        const native_handle_t* jh = jit->second;
        ALOGI("shim: jpeg buffer %lld fds=%d ints=%d w=%d h=%d sz=%d",
              static_cast<long long>(pendingJpeg.bufferId), jh->numFds, jh->numInts,
              (jh->numInts >= 10) ? jh->data[jh->numFds + 9] : -1,
              (jh->numInts >= 11) ? jh->data[jh->numFds + 10] : -1,
              (jh->numInts >= 3) ? jh->data[jh->numFds + 2] : -1);
    }
    // Deliver the older in-flight preview frames BEFORE the capture frame's
    // shutter (see above; the framework requires strictly increasing frame
    // order and errors the device otherwise).
    for (const auto& pv : preFlush) {
        deliverPreviewResult(pv);
    }
    fillOutputBuffer(pendingJpeg.bufferId, mJpegStreamId, jpegData, jpegSize);
    const int64_t jpegTimestamp = systemTime(SYSTEM_TIME_MONOTONIC);
    sendShutterNotify(pendingJpeg.frameNumber, jpegTimestamp);
    CaptureResult result;
    result.frameNumber = pendingJpeg.frameNumber;
    result.fmqResultSize = 0;
    result.partialResult = 1;
    result.inputBuffer.streamId = -1;
    camera_metadata_t* m = allocate_camera_metadata(16, 256);
    add_camera_metadata_entry(m, ANDROID_SENSOR_TIMESTAMP, &jpegTimestamp, 1);
    const int32_t aeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AE_STATE, &aeState, 1);
    const int32_t afState = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AF_STATE, &afState, 1);
    // EXIF fields the app reads from the JPEG result (ExifUtil): a missing
    // FLASH_MODE makes flashMode.intValue() NPE and crashes Camera2 after
    // every capture. Fill the full set.
    const int64_t exposureTimeNs = 16666667;   // ~1/60s
    add_camera_metadata_entry(m, ANDROID_SENSOR_EXPOSURE_TIME, &exposureTimeNs, 1);
    const int32_t sensitivity = 100;
    add_camera_metadata_entry(m, ANDROID_SENSOR_SENSITIVITY, &sensitivity, 1);
    const float aperture = 2.4f;
    add_camera_metadata_entry(m, ANDROID_LENS_APERTURE, &aperture, 1);
    const float focalLength = 4.6f;
    add_camera_metadata_entry(m, ANDROID_LENS_FOCAL_LENGTH, &focalLength, 1);
    const int32_t flashMode = ANDROID_FLASH_MODE_OFF;
    add_camera_metadata_entry(m, ANDROID_FLASH_MODE, &flashMode, 1);
    const int32_t awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AWB_MODE, &awbMode, 1);
    size_t metaSize = get_camera_metadata_size(m);
    result.result.metadata.resize(metaSize);
    memcpy(result.result.metadata.data(), m, metaSize);
    free_camera_metadata(m);
    StreamBuffer out{};
    out.streamId = mJpegStreamId;
    out.bufferId = pendingJpeg.bufferId;
    out.status = BufferStatus::OK;
    result.outputBuffers.push_back(std::move(out));
    if (hasPostview) {
        if (!mLastPreviewFrame.empty()) {
            fillOutputBuffer(pendingPostview.bufferId, mPreviewStreamId,
                             mLastPreviewFrame.data(), mLastPreviewFrame.size());
        }
        StreamBuffer pv{};
        pv.streamId = mPreviewStreamId;
        pv.bufferId = pendingPostview.bufferId;
        pv.status = BufferStatus::OK;
        result.outputBuffers.push_back(std::move(pv));
    }
    std::vector<CaptureResult> results;
    results.push_back(std::move(result));
    mCallback->processCaptureResult(results);

    // Complete the repeating requests queued after the capture frame, in
    // frame order, strictly before any next pending JPEG (rapid consecutive
    // shots) so the next JPEG handler never re-delivers a shutter.
    for (const auto& pv : postFlush) {
        deliverPreviewResult(pv);
    }
    // The legacy HAL stops the preview stream internally for snapshot
    // (pausePreviewForSnapshot) and remains in QCAMERA_HAL_TAKE_PICTURE;
    // a second take_picture in that state is a no-op and the session
    // errors. Drive it back to STOPPED via stop_preview() from a detached
    // thread: this callback still runs under the HAL's JPEG callback lock
    // and stop_preview -> cancelPictureInternal waits on that lock
    // (deadlock if called inline). Reset the running flag so the next
    // preview request restarts the stream.
    mPreviewRunning = false;
    camera_device_t* dev = mDevice;
    const uint32_t generation = mSessionGeneration;
    if (dev != nullptr && dev->ops != nullptr && dev->ops->stop_preview != nullptr) {
        {
            std::lock_guard<std::mutex> qlock(mQueueLock);
            mStopPending = true;
        }
        std::thread([this, dev, generation]() {
            usleep(50000);
            // The app may close and reopen the camera between the JPEG
            // callback and this stop; stopping a freshly-opened session
            // tears down the live preview. Only stop our own generation.
            {
                std::lock_guard<std::mutex> lock(mLock);
                if (mClosed || mSessionGeneration != generation) {
                    std::lock_guard<std::mutex> qlock(mQueueLock);
                    mStopPending = false;
                    mStopCv.notify_all();
                    return;
                }
            }
            dev->ops->stop_preview(dev);
            // The framework only auto-resubmits the repeating preview
            // request after the previous request's result completes, and
            // preview callbacks are the only source of those results. If
            // preview requests are already queued (submitted while the
            // capture was in flight), nothing would ever restart the
            // stream and the session would stall with the viewfinder
            // frozen. Restart preview ourselves so the queued requests
            // complete and the repeating loop keeps running.
            bool restartPreview = false;
            {
                std::lock_guard<std::mutex> qlock(mQueueLock);
                restartPreview = !mPendingPreview.empty();
            }
            if (restartPreview) {
                if (dev->ops->start_preview != nullptr) {
                    dev->ops->start_preview(dev);
                    mPreviewRunning = true;
                }
            }
            std::lock_guard<std::mutex> qlock(mQueueLock);
            mStopPending = false;
            mStopCv.notify_all();
        }).detach();
    }
}

std::string CameraDeviceSessionShim::readHalParameterLocked(const std::string& key) {
    if (mDevice == nullptr || mDevice->ops->get_parameters == nullptr) {
        return "";
    }
    char* full = mDevice->ops->get_parameters(mDevice);
    if (full == nullptr) {
        return "";
    }
    std::string merged(full);
    free(full);
    const std::string needle = key + "=";
    size_t pos = merged.find(needle);
    if (pos == std::string::npos) {
        return "";
    }
    size_t end = merged.find(';', pos);
    if (end == std::string::npos) {
        end = merged.size();
    }
    return merged.substr(pos + needle.size(), end - pos - needle.size());
}

// Pick the HAL preview size whose aspect ratio best matches the requested
// preview stream, choosing the candidate whose AREA is closest to the
// stream's (bigger is NOT better: 1280x960 preview runs at ~2fps on this
// daemon while 640x480 does 30fps; 1280x720 produces NO frames at all).
// The center-crop converter handles the resolution difference; matching the
// aspect avoids distortion. Hard cap at 640x480: the only size verified to
// stream reliably on this daemon.
bool CameraDeviceSessionShim::chooseHalPreviewSizeLocked() {
    if (mPreviewWidth <= 0 || mPreviewHeight <= 0) {
        return false;
    }
    const std::string values = readHalParameterLocked("preview-size-values");
    if (values.empty()) {
        return false;
    }
    const double wantAspect = static_cast<double>(mPreviewWidth) / mPreviewHeight;
    const long capArea = 640L * 480L;  // known-good ceiling for this VFE
    // Target area for the closest-area score: the preview stream size, so
    // snapshots come out near viewfinder scale while respecting the cap.
    const long wantArea =
            static_cast<long>(mPreviewWidth) * static_cast<long>(mPreviewHeight);
    long bestW = 0, bestH = 0;
    double bestErr = 1e9;
    size_t start = 0;
    while (start < values.size()) {
        size_t end = values.find(',', start);
        if (end == std::string::npos) {
            end = values.size();
        }
        std::string cand = values.substr(start, end - start);
        int w = 0, h = 0;
        if (sscanf(cand.c_str(), "%dx%d", &w, &h) == 2 && w > 0 && h > 0) {
            const double aspect = static_cast<double>(w) / h;
            const double aspectErr = aspect > wantAspect ? aspect / wantAspect
                                                         : wantAspect / aspect;
            // Accept up to ~34% aspect deviation as a match: with the hard
            // area cap only 4:3 candidates survive anyway, and a cropped
            // 4:3 viewfinder beats no frames at all.
            if (aspectErr <= 1.34) {
                const double areaErr =
                        (static_cast<double>(w) * h) / wantArea;
                const double score = aspectErr * 10.0 +
                        (areaErr > 1.0 ? areaErr : 1.0 / areaErr);
                if (score < bestErr &&
                        static_cast<long>(w) * static_cast<long>(h) <= capArea) {
                    bestErr = score;
                    bestW = w;
                    bestH = h;
                }
            }
        }
        start = end + 1;
    }
    if (bestW <= 0) {
        return false;
    }
    if (bestW == mHalPreviewWidth && bestH == mHalPreviewHeight) {
        return false;  // unchanged; skip the parameter write
    }
    mHalPreviewWidth = bestW;
    mHalPreviewHeight = bestH;
    ALOGI("shim: hal preview size %dx%d for stream %dx%d", bestW, bestH, mPreviewWidth,
          mPreviewHeight);
    return true;
}

ndk::ScopedAStatus CameraDeviceSessionShim::signalStreamFlush(
    const std::vector<int32_t>& /*streamIds*/, int32_t /*streamConfigCounter*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceSessionShim::switchToOffline(
    const std::vector<int32_t>& /*streamsToKeep*/,
    ::aidl::android::hardware::camera::device::CameraOfflineSessionInfo*
        /*out_offlineSessionInfo*/,
    std::shared_ptr<::aidl::android::hardware::camera::device::ICameraOfflineSession>*
        _aidl_return) {
    _aidl_return->reset();
    return ndk::ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

ndk::ScopedAStatus CameraDeviceSessionShim::repeatingRequestEnd(
    int32_t /*frameNumber*/, const std::vector<int32_t>& /*streamIds*/) {
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::camera::provider::shim
