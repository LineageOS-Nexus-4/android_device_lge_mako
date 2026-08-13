// mako camera provider shim service: registers the AIDL camera provider
// (virtual/0) that wraps the legacy camera HAL 1.0.
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <aidl/android/hardware/camera/provider/BnCameraProvider.h>

#include <string>

#include "CameraProviderShim.h"

using ::aidl::android::hardware::camera::provider::shim::CameraProviderShim;

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(4);

    auto provider = ndk::SharedRefBase::make<CameraProviderShim>();

    const std::string instance =
            std::string() + ::aidl::android::hardware::camera::provider::BnCameraProvider::descriptor +
            "/virtual/0";

    binder_status_t status =
            AServiceManager_addService(provider->asBinder().get(), instance.c_str());
    if (status != STATUS_OK) {
        return 1;
    }

    ABinderProcess_startThreadPool();
    ABinderProcess_joinThreadPool();
    return 0;
}
