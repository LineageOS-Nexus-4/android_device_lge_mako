// Camera provider AIDL shim for mako: exposes the legacy camera HAL 1.0
// (camera.mako.so) as an AIDL camera provider with AIDL camera devices,
// because the AOSP 17 cameraserver only supports HIDL device HAL major 3
// and AIDL camera devices (major 1).
#pragma once

#include <aidl/android/hardware/camera/provider/BnCameraProvider.h>
#include <aidl/android/hardware/camera/provider/ICameraProviderCallback.h>

#include <hardware/camera.h>

#include <mutex>
#include <unordered_map>

namespace aidl::android::hardware::camera::provider::shim {




using ::aidl::android::hardware::camera::provider::BnCameraProvider;
using ::aidl::android::hardware::camera::provider::ICameraProviderCallback;
using ::aidl::android::hardware::camera::provider::ConcurrentCameraIdCombination;
using ::aidl::android::hardware::camera::provider::CameraIdAndStreamCombination;
using ::aidl::android::hardware::camera::provider::BnCameraProvider;
using ::aidl::android::hardware::camera::provider::ICameraProviderCallback;
using ::aidl::android::hardware::camera::provider::ConcurrentCameraIdCombination;
using ::aidl::android::hardware::camera::provider::CameraIdAndStreamCombination;
using ::aidl::android::hardware::camera::common::VendorTagSection;

class CameraDeviceShim;

class CameraProviderShim : public BnCameraProvider {
  public:
    CameraProviderShim();
    ~CameraProviderShim() override;

    // ICameraProvider
    ndk::ScopedAStatus setCallback(
        const std::shared_ptr<ICameraProviderCallback>& callback) override;
    ndk::ScopedAStatus getVendorTags(std::vector<VendorTagSection>* _aidl_return) override;
    ndk::ScopedAStatus getCameraIdList(std::vector<std::string>* _aidl_return) override;
    ndk::ScopedAStatus getCameraDeviceInterface(
        const std::string& cameraDeviceName,
        std::shared_ptr<::aidl::android::hardware::camera::device::ICameraDevice>* _aidl_return) override;
    ndk::ScopedAStatus notifyDeviceStateChange(int64_t deviceState) override;
    ndk::ScopedAStatus getConcurrentCameraIds(
        std::vector<ConcurrentCameraIdCombination>* _aidl_return) override;
    ndk::ScopedAStatus isConcurrentStreamCombinationSupported(
        const std::vector<CameraIdAndStreamCombination>& configs,
        bool* _aidl_return) override;

  private:
  public:
    void notifyTorchModeChanged(const std::string& deviceName, int32_t newStatus);

  private:
    static const int kNumCameras = 2;
    std::mutex mLock;
    std::shared_ptr<ICameraProviderCallback> mCallback;
    std::unordered_map<std::string, std::shared_ptr<CameraDeviceShim>> mDevices;
};

}  // namespace aidl::android::hardware::camera::provider::shim
