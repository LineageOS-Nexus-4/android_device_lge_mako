#include "CameraProviderShim.h"
#include "CameraDeviceShim.h"

#include <aidl/android/hardware/camera/common/TorchModeStatus.h>

#include <hardware/hardware.h>

#include <android-base/logging.h>
#include <log/log.h>

#include <cstring>

namespace aidl::android::hardware::camera::provider::shim {
using ::aidl::android::hardware::camera::device::ICameraDevice;



CameraProviderShim::CameraProviderShim() = default;

CameraProviderShim::~CameraProviderShim() = default;

ndk::ScopedAStatus CameraProviderShim::setCallback(
    const std::shared_ptr<ICameraProviderCallback>& callback) {
    std::lock_guard<std::mutex> lock(mLock);
    mCallback = callback;
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraProviderShim::getVendorTags(
    std::vector<VendorTagSection>* _aidl_return) {
    _aidl_return->clear();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraProviderShim::getCameraIdList(
    std::vector<std::string>* _aidl_return) {
    _aidl_return->clear();
    for (int i = 0; i < kNumCameras; ++i) {
        _aidl_return->push_back("device@1.0/internal/" + std::to_string(i));
    }
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraProviderShim::getCameraDeviceInterface(
    const std::string& cameraDeviceName,
    std::shared_ptr<ICameraDevice>* _aidl_return) {
    const std::string kPrefix = "device@1.0/internal/";
    if (cameraDeviceName.rfind(kPrefix, 0) != 0) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    int cameraId = atoi(cameraDeviceName.c_str() + kPrefix.size());
    if (cameraId < 0 || cameraId >= kNumCameras) {
        return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }
    std::lock_guard<std::mutex> lock(mLock);
    auto it = mDevices.find(cameraDeviceName);
    if (it == mDevices.end()) {
        camera_module_t* module = nullptr;
        const hw_module_t* rawModule = nullptr;
        hw_get_module(CAMERA_HARDWARE_MODULE_ID, &rawModule);
        module = reinterpret_cast<camera_module_t*>(const_cast<hw_module_t*>(rawModule));
        if (module == nullptr) {
            return ndk::ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
        }
        it = mDevices
                 .emplace(cameraDeviceName,
                          ::ndk::SharedRefBase::make<CameraDeviceShim>(cameraId, module))
                 .first;
        it->second->setProvider(this);
    }
    *_aidl_return = it->second;
    return ndk::ScopedAStatus::ok();
}

void CameraProviderShim::notifyTorchModeChanged(const std::string& deviceName,
                                                 int32_t newStatus) {
    std::shared_ptr<ICameraProviderCallback> cb;
    {
        std::lock_guard<std::mutex> lock(mLock);
        cb = mCallback;
    }
    if (cb != nullptr) {
        auto status = cb->torchModeStatusChange(
                deviceName,
                static_cast<::aidl::android::hardware::camera::common::TorchModeStatus>(
                        newStatus));
        if (!status.isOk()) {
            ALOGE("shim: torchModeStatusChange failed: %s", status.getMessage());
        }
    }
}

ndk::ScopedAStatus CameraProviderShim::notifyDeviceStateChange(int64_t /*deviceState*/) {
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraProviderShim::getConcurrentCameraIds(
    std::vector<ConcurrentCameraIdCombination>* _aidl_return) {
    _aidl_return->clear();
    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus CameraProviderShim::isConcurrentStreamCombinationSupported(
    const std::vector<CameraIdAndStreamCombination>& /*configs*/, bool* _aidl_return) {
    *_aidl_return = false;
    return ndk::ScopedAStatus::ok();
}

}  // namespace aidl::android::hardware::camera::provider::shim
