#include "CameraDeviceSessionShim.h"

#include <system/camera_metadata.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/Rect.h>

#include <cinttypes>
#include <cstring>

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
    if (msgType == CAMERA_MSG_SHUTTER) {
        sendShutterNotify(mNextFrameNumber);
    }
}

void CameraDeviceSessionShim::halDeviceData(int32_t msgType, const camera_memory_t* data,
                                            unsigned int index) {
    const void* buf = data->data;
    if (index > 0) {
        buf = static_cast<const uint8_t*>(data->data) + data->size * index;
    }
    if (msgType & CAMERA_MSG_PREVIEW_FRAME) {
        handlePreviewBuffer(buf, data->size);
    } else if (msgType & CAMERA_MSG_COMPRESSED_IMAGE) {
        handleJpegBuffer(buf, data->size);
    }
}

bool CameraDeviceSessionShim::setHalParametersLocked(const std::string& params) {
    if (mDevice == nullptr) {
        return false;
    }
    return mDevice->ops->set_parameters(mDevice, params.c_str()) == 0;
}

ndk::ScopedAStatus CameraDeviceSessionShim::close() {
    std::lock_guard<std::mutex> lock(mLock);
    if (mClosed) {
        return ndk::ScopedAStatus::ok();
    }
    mClosed = true;
    if (mDevice != nullptr) {
        if (mPreviewRunning) {
            mDevice->ops->stop_preview(mDevice);
        }
        mDevice->ops->release(mDevice);
        mDevice->common.close(&mDevice->common);
        mDevice = nullptr;
    }
    if (mPreviewMemory != nullptr) {
        mPreviewMemory->release(mPreviewMemory);
        mPreviewMemory = nullptr;
    }
    if (mJpegMemory != nullptr) {
        mJpegMemory->release(mJpegMemory);
        mJpegMemory = nullptr;
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
    _aidl_return->clear();
    mPreviewStreamId = -1;
    mJpegStreamId = -1;
    for (const auto& s : requestedConfiguration.streams) {
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
        } else if (streamIsPreview(s)) {
            mPreviewStreamId = s.id;
            mPreviewWidth = s.width;
            mPreviewHeight = s.height;
            mPreviewFormat = kHALPixelFormatNV21;
            hal.maxBuffers = 2;
            hal.producerUsage = static_cast<::aidl::android::hardware::graphics::common::BufferUsage>(48L);
            hal.overrideFormat = ::aidl::android::hardware::graphics::common::PixelFormat::YCBCR_420_888;
        } else {
            hal.supportOffline = false;
        }
        _aidl_return->push_back(hal);
    }
    mConfigured = true;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraDeviceSessionShim::constructDefaultRequestSettings(
    RequestTemplate /*type*/, CameraMetadata* _aidl_return) {
    if (mDevice != nullptr) {
        // The legacy HAL needs the device opened for the defaults to apply,
        // which the cameraserver does before requesting templates.
        if (mDevice->ops->get_parameters != nullptr) {
            mDevice->ops->get_parameters(mDevice);
        }
    }
    camera_metadata_t* m = allocate_camera_metadata(16, 256);
    const int32_t aeMode = ANDROID_CONTROL_AE_MODE_ON;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AE_MODE, &aeMode, 1);
    const int32_t afMode = ANDROID_CONTROL_AF_MODE_AUTO;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AF_MODE, &afMode, 1);
    const int32_t awbMode = ANDROID_CONTROL_AWB_MODE_AUTO;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AWB_MODE, &awbMode, 1);
    const int32_t fpsRange[] = {30000, 30000};
    add_camera_metadata_entry(m, ANDROID_CONTROL_AE_TARGET_FPS_RANGE, fpsRange, 2);
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
    std::lock_guard<std::mutex> lock(mLock);
    if (mClosed) {
        *_aidl_return = -1;
        return ndk::ScopedAStatus::ok();
    }
    for (const auto& request : requests) {
        processRequest(request);
    }
    *_aidl_return = static_cast<int32_t>(requests.size());
    return ndk::ScopedAStatus::ok();
}

void CameraDeviceSessionShim::processRequest(const CaptureRequest& request) {
    if (!openHalDeviceLocked()) {
        return;
    }
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
            const char* flashMode = "off";
            switch (entry.data.u8[0]) {
                case ANDROID_FLASH_MODE_SINGLE:
                    flashMode = "on";
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
            params += "jpeg-rotation=" + std::to_string(entry.data.i32[0]);
            params += ";";
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
        if (buf.streamId == mPreviewStreamId) {
            mPendingPreview.push_back({buf.bufferId, static_cast<uint32_t>(request.frameNumber)});
        } else if (buf.streamId == mJpegStreamId) {
            mPendingJpeg.push_back({buf.bufferId, static_cast<uint32_t>(request.frameNumber)});
        }
    }

    // Start preview on the first preview request.
    if (mPreviewStreamId >= 0 && hasPreviewBuffer && !mPreviewRunning) {
        if (mPreviewWidth > 0 && mPreviewHeight > 0) {
            std::string p = "preview-size=" + std::to_string(mPreviewWidth) + "x" +
                            std::to_string(mPreviewHeight) + ";";
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
    memcpy(vaddr, data, size);
    munmap(vaddr, bufSize);
    return true;
}

void CameraDeviceSessionShim::handlePreviewBuffer(const void* data, size_t size) {
    if (mCallback == nullptr || mPendingPreview.empty()) {
        return;
    }
    PendingBuffer pending = mPendingPreview.front();
    mPendingPreview.pop_front();
    ALOGI("shim: preview frame %d into buffer %d", pending.frameNumber, pending.bufferId);
    fillOutputBuffer(pending.bufferId, mPreviewStreamId, data, size);

    CaptureResult result;
    result.frameNumber = pending.frameNumber;
    result.fmqResultSize = 0;
    result.partialResult = 1;
    result.inputBuffer.streamId = -1;
    camera_metadata_t* m = allocate_camera_metadata(8, 128);
    const int64_t timestamp = systemTime(SYSTEM_TIME_MONOTONIC);
    add_camera_metadata_entry(m, ANDROID_SENSOR_TIMESTAMP, &timestamp, 1);
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
    out.status = BufferStatus::OK;
    result.outputBuffers.push_back(std::move(out));
    std::vector<CaptureResult> results;
    results.push_back(std::move(result));
    mCallback->processCaptureResult(results);
}

void CameraDeviceSessionShim::sendShutterNotify(uint32_t frameNumber) {
    if (mCallback == nullptr) {
        return;
    }
    NotifyMsg msg{};
    ShutterMsg shutter;
    shutter.frameNumber = frameNumber;
    shutter.timestamp = systemTime(SYSTEM_TIME_MONOTONIC);
    msg.set<NotifyMsg::shutter>(shutter);
    mCallback->notify({msg});
}

void CameraDeviceSessionShim::handleJpegBuffer(const void* data, size_t size) {
    if (mCallback == nullptr || mPendingJpeg.empty()) {
        return;
    }
    PendingBuffer pendingJpeg = mPendingJpeg.front();
    mPendingJpeg.pop_front();
    ALOGI("shim: jpeg %d bytes into buffer %d", static_cast<int>(size), pendingJpeg.bufferId);
    fillOutputBuffer(pendingJpeg.bufferId, mJpegStreamId, data, size);
    sendShutterNotify(mActiveRequestId);
    CaptureResult result;
    result.frameNumber = pendingJpeg.frameNumber;
    result.fmqResultSize = 0;
    result.partialResult = 1;
    result.inputBuffer.streamId = -1;
    camera_metadata_t* m = allocate_camera_metadata(8, 128);
    const int64_t timestamp = systemTime(SYSTEM_TIME_MONOTONIC);
    add_camera_metadata_entry(m, ANDROID_SENSOR_TIMESTAMP, &timestamp, 1);
    const int32_t aeState = ANDROID_CONTROL_AE_STATE_CONVERGED;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AE_STATE, &aeState, 1);
    const int32_t afState = ANDROID_CONTROL_AF_STATE_FOCUSED_LOCKED;
    add_camera_metadata_entry(m, ANDROID_CONTROL_AF_STATE, &afState, 1);
    size_t metaSize = get_camera_metadata_size(m);
    result.result.metadata.resize(metaSize);
    memcpy(result.result.metadata.data(), m, metaSize);
    free_camera_metadata(m);
    StreamBuffer out{};
    out.streamId = mJpegStreamId;
    out.bufferId = pendingJpeg.bufferId;
    out.status = BufferStatus::OK;
    result.outputBuffers.push_back(std::move(out));
    std::vector<CaptureResult> results;
    results.push_back(std::move(result));
    mCallback->processCaptureResult(results);
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

ndk::ScopedAStatus CameraDeviceSessionShim::configureStreamsV2(
    const StreamConfiguration& requestedConfiguration,
    ::aidl::android::hardware::camera::device::ConfigureStreamsRet* _aidl_return) {
    std::vector<HalStream> halStreams;
    auto status = configureStreams(requestedConfiguration, &halStreams);
    if (!status.isOk()) {
        return status;
    }
    _aidl_return->halStreams = halStreams;
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::camera::provider::shim
