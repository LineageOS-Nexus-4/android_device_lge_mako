// Camera device session shim for mako: drives the legacy HAL 1.0
// camera_device_t preview/still/record operations behind the AIDL
// ICameraDeviceSession interface.
#pragma once

#include <aidl/android/hardware/camera/device/BnCameraDeviceSession.h>
#include <aidl/android/hardware/camera/device/ICameraDeviceCallback.h>

#include <cutils/native_handle.h>
#include <hardware/camera.h>
#include <hardware/gralloc.h>
#include <ui/GraphicBufferMapper.h>

#include <condition_variable>
#include <deque>
#include <map>
#include <mutex>
#include <thread>

namespace aidl::android::hardware::camera::provider::shim {



using ::aidl::android::hardware::camera::device::StreamConfiguration;
using ::aidl::android::hardware::camera::device::HalStream;
using ::aidl::android::hardware::camera::device::BnCameraDeviceSession;
using ::aidl::android::hardware::camera::device::ICameraDeviceCallback;
using ::aidl::android::hardware::camera::device::CameraMetadata;
using ::aidl::android::hardware::camera::device::RequestTemplate;
using ::aidl::android::hardware::camera::device::CaptureRequest;
using ::aidl::android::hardware::camera::device::BufferCache;
using ::aidl::android::hardware::camera::device::CaptureResult;
using ::aidl::android::hardware::camera::device::NotifyMsg;

// Minimal pass-through preview window: the legacy mako HAL only drives its
// preview stream through a preview_stream_ops_t window (dequeue/lock buffers,
// camera daemon writes frames into them). We supply our own gralloc buffers.
struct ShimPreviewWindow {
    preview_stream_ops_t ops{};
    alloc_device_t* mGrallocDev = nullptr;
    gralloc_module_t* gralloc = nullptr;
    std::vector<buffer_handle_t> handles;
    std::vector<int> strides;
    size_t next = 0;
    int width = 0;
    int height = 0;
    int format = 0x11;
    int count = 0;
    int allocateBuffers();
};

class CameraDeviceSessionShim : public BnCameraDeviceSession {
  public:
    CameraDeviceSessionShim(camera_module_t* module, int cameraId,
                            std::shared_ptr<ICameraDeviceCallback> callback);
    ~CameraDeviceSessionShim() override;

    // ICameraDeviceSession
    ndk::ScopedAStatus close() override;
    ndk::ScopedAStatus configureStreams(
        const StreamConfiguration& requestedConfiguration,
        std::vector<HalStream>* _aidl_return) override;
    ndk::ScopedAStatus constructDefaultRequestSettings(
        RequestTemplate type, CameraMetadata* _aidl_return) override;
    ndk::ScopedAStatus flush() override;
    ndk::ScopedAStatus getCaptureRequestMetadataQueue(
        ::aidl::android::hardware::common::fmq::MQDescriptor<
            int8_t, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>*
            _aidl_return) override;
    ndk::ScopedAStatus getCaptureResultMetadataQueue(
        ::aidl::android::hardware::common::fmq::MQDescriptor<
            int8_t, ::aidl::android::hardware::common::fmq::SynchronizedReadWrite>*
            _aidl_return) override;
    ndk::ScopedAStatus isReconfigurationRequired(
        const CameraMetadata& oldSessionParams,
        const CameraMetadata& newSessionParams, bool* _aidl_return) override;
    ndk::ScopedAStatus processCaptureRequest(
        const std::vector<CaptureRequest>& requests,
        const std::vector<BufferCache>& cachesToRemove,
        int32_t* _aidl_return) override;
    ndk::ScopedAStatus signalStreamFlush(const std::vector<int32_t>& streamIds,
                                         int32_t streamConfigCounter) override;
    ndk::ScopedAStatus switchToOffline(
        const std::vector<int32_t>& streamsToKeep,
        ::aidl::android::hardware::camera::device::CameraOfflineSessionInfo*
            out_offlineSessionInfo,
        std::shared_ptr<::aidl::android::hardware::camera::device::ICameraOfflineSession>*
            _aidl_return) override;
    ndk::ScopedAStatus repeatingRequestEnd(
        int32_t frameNumber, const std::vector<int32_t>& streamIds) override;
    ndk::ScopedAStatus configureStreamsV2(
        const StreamConfiguration& requestedConfiguration,
        ::aidl::android::hardware::camera::device::ConfigureStreamsRet* _aidl_return) override;

  private:
    // HAL 1.0 callbacks
    static void halNotify(int32_t msgType, int32_t ext1, int32_t ext2, void* user);
    static void halDataCallback(int32_t msgType, const camera_memory_t* data,
                                unsigned int index, camera_frame_metadata_t* metadata,
                                void* user);
    static void halDataCallbackTimestamp(nsecs_t timestamp, int32_t msgType,
                                         const camera_memory_t* data, unsigned int index,
                                         void* user);

    void halDeviceNotify(int32_t msgType, int32_t ext1, int32_t ext2);
    void halDeviceData(int32_t msgType, const camera_memory_t* data, unsigned int index);
    bool openHalDeviceLocked();
    bool setHalParametersLocked(const std::string& params);
    void handlePreviewBuffer(const void* data, size_t size);
    void handleJpegBuffer(const void* data, size_t size);
    void sendShutterNotify(uint32_t frameNumber, int64_t timestamp);
    bool fillOutputBuffer(int64_t bufferId, int32_t streamId, const void* data, size_t size);
    void processRequest(const CaptureRequest& request);

    struct PendingBuffer {
        int64_t bufferId;
        uint32_t frameNumber;
    };

    camera_module_t* mModule;
    const int mCameraId;
    std::shared_ptr<ICameraDeviceCallback> mCallback;
    std::mutex mLock;
    std::condition_variable mCv;

    camera_device_t* mDevice;
    camera_memory_t* mPreviewMemory;
    void* mPreviewData;
    camera_memory_t* mJpegMemory;
    void* mJpegData;

    // stream state
    int mPreviewStreamId;
    int mPreviewWidth;
    int mPreviewHeight;
    int mPreviewFormat;
    int mJpegStreamId;
    int mJpegWidth;
    int mJpegHeight;
    int mActiveRequestId;
    uint32_t mNextFrameNumber;
    bool mConfigured;
    bool mClosed;
    bool mPreviewRunning;

    ShimPreviewWindow mShimWindow;
    std::map<int64_t, native_handle_t*> mBufferHandles;
    std::deque<PendingBuffer> mPendingPreview;
    std::deque<PendingBuffer> mPendingJpeg;
    const gralloc_module_t* mGralloc = nullptr;
};

}  // namespace aidl::android::hardware::camera::provider::shim
