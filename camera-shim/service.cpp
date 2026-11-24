// mako camera provider shim service: registers the AIDL camera provider
// (internal/0) that wraps the legacy camera HAL 1.0.
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <android/api-level.h>
#include <aidl/android/hardware/camera/provider/BnCameraProvider.h>
#include <dlfcn.h>
#include <log/log.h>

#include <string>

#include "CameraProviderShim.h"

using ::aidl::android::hardware::camera::provider::shim::CameraProviderShim;

int main() {
    // The legacy JPEG blob can join a worker that has already been reaped.
    // Bionic returned ESRCH through API 25; API 26+ aborts the entire provider.
    // Keep that compatibility confined to this old HAL's process.
    void* dlAndroid = dlopen("libdl_android.so", RTLD_NOW | RTLD_LOCAL);
    auto setTargetSdk = dlAndroid ? reinterpret_cast<void (*)(int)>(
            dlsym(dlAndroid, "android_set_application_target_sdk_version")) : nullptr;
    if (setTargetSdk == nullptr) {
        ALOGE("Cannot enable legacy camera libc compatibility: %s", dlerror());
        return 1;
    }
    setTargetSdk(25);
    dlclose(dlAndroid);
    ALOGI("Legacy camera libc target SDK: %d", android_get_application_target_sdk_version());
    ABinderProcess_setThreadPoolMaxThreadCount(4);

    auto provider = ndk::SharedRefBase::make<CameraProviderShim>();

    const std::string instance =
            std::string() + ::aidl::android::hardware::camera::provider::BnCameraProvider::descriptor +
            "/internal/0";

    binder_status_t status =
            AServiceManager_addService(provider->asBinder().get(), instance.c_str());
    if (status != STATUS_OK) {
        return 1;
    }

    ABinderProcess_startThreadPool();
    ABinderProcess_joinThreadPool();
    return 0;
}
