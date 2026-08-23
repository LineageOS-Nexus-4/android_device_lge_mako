// Camera device AIDL shim for mako: wraps one legacy HAL 1.0 camera
// (camera_device_t) behind the AIDL ICameraDevice interface.
#pragma once

#include <aidl/android/hardware/camera/device/BnCameraDevice.h>
#include <aidl/android/hardware/camera/device/ICameraDeviceCallback.h>
#include <aidl/android/hardware/camera/device/BnCameraDeviceSession.h>

#include <hardware/camera.h>

#include <mutex>

namespace aidl::android::hardware::camera::provider::shim {




using ::aidl::android::hardware::camera::device::BnCameraDevice;
using ::aidl::android::hardware::camera::device::ICameraDeviceCallback;
using ::aidl::android::hardware::camera::device::ICameraDeviceSession;
using ::aidl::android::hardware::camera::device::CameraMetadata;
using ::aidl::android::hardware::camera::common::CameraResourceCost;
using ::aidl::android::hardware::camera::device::StreamConfiguration;
using ::aidl::android::hardware::camera::device::RequestTemplate;

class CameraDeviceSessionShim;
class CameraProviderShim;

class CameraDeviceShim : public BnCameraDevice {
  public:
    CameraDeviceShim(int cameraId, camera_module_t* module);
    void setProvider(CameraProviderShim* provider);
    ~CameraDeviceShim() override;

    // ICameraDevice
    ndk::ScopedAStatus getCameraCharacteristics(CameraMetadata* _aidl_return) override;
    ndk::ScopedAStatus getPhysicalCameraCharacteristics(
        const std::string& physicalCameraId, CameraMetadata* _aidl_return) override;
    ndk::ScopedAStatus getResourceCost(CameraResourceCost* _aidl_return) override;
    ndk::ScopedAStatus isStreamCombinationSupported(
        const StreamConfiguration& streams, bool* _aidl_return) override;
    ndk::ScopedAStatus open(
        const std::shared_ptr<ICameraDeviceCallback>& callback,
        std::shared_ptr<ICameraDeviceSession>* _aidl_return) override;
    ndk::ScopedAStatus openInjectionSession(
        const std::shared_ptr<ICameraDeviceCallback>& callback,
        std::shared_ptr<::aidl::android::hardware::camera::device::ICameraInjectionSession>*
            _aidl_return) override;
    ndk::ScopedAStatus setTorchMode(bool on) override;
    ndk::ScopedAStatus turnOnTorchWithStrengthLevel(int32_t torchStrength) override;
    ndk::ScopedAStatus getTorchStrengthLevel(int32_t* _aidl_return) override;
    ndk::ScopedAStatus constructDefaultRequestSettings(
        RequestTemplate type, CameraMetadata* _aidl_return) override;
    ndk::ScopedAStatus isStreamCombinationWithSettingsSupported(
        const StreamConfiguration& streams, bool* _aidl_return) override;
    ndk::ScopedAStatus getSessionCharacteristics(
        const StreamConfiguration& sessionConfig, CameraMetadata* _aidl_return) override;

  private:
    friend class CameraDeviceSessionShim;
    camera_metadata_t* buildStaticCharacteristicsLocked();

    const int mCameraId;
    camera_module_t* mModule;
    std::mutex mLock;
    camera_metadata_t* mStaticMetadata;
    bool mTorchOn;
    CameraProviderShim* mProvider = nullptr;
};

}  // namespace aidl::android::hardware::camera::provider::shim
