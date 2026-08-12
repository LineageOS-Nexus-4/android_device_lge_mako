#include "RadioConfig.h"

#include <android-base/logging.h>

namespace android::hardware::radio::shim {

using ::ndk::ScopedAStatus;

ScopedAStatus RadioConfig::setResponseFunctions(
        const std::shared_ptr<ar::config::IRadioConfigResponse>& response,
        const std::shared_ptr<ar::config::IRadioConfigIndication>& indication) {
    mCtx->configResponse = response;
    mCtx->configIndication = indication;
    return ScopedAStatus::ok();
}

ScopedAStatus RadioConfig::getPhoneCapability(int32_t serial) {
    if (!mCtx->configResponse) return ScopedAStatus::ok();
    ar::config::PhoneCapability cap;
    cap.maxActiveData = 1;
    cap.maxActiveInternetData = 1;
    cap.isInternetLingeringSupported = false;
    cap.logicalModemIds = {0};
    cap.maxActiveVoice = 1;
    mCtx->configResponse->getPhoneCapabilityResponse(mCtx->noError(serial), cap);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioConfig::getSimSlotsStatus(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    // Forward getIccCardStatus to the HIDL radio; the response handler
    // synthesizes the SimSlotStatus from the CardStatus.
    mCtx->registerRequest(serial, RequestKind::CONFIG_GET_SIM_SLOTS_STATUS);
    const auto status = mCtx->radio->getIccCardStatus(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getIccCardStatus (config) HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioConfig::setSimSlotsMapping(
        int32_t serial, const std::vector<ar::config::SlotPortMapping>&) {
    if (!mCtx->configResponse) return ScopedAStatus::ok();
    mCtx->configResponse->setSimSlotsMappingResponse(mCtx->noError(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioConfig::setPreferredDataModem(int32_t serial, int8_t) {
    if (!mCtx->configResponse) return ScopedAStatus::ok();
    mCtx->configResponse->setPreferredDataModemResponse(mCtx->noError(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioConfig::getNumOfLiveModems(int32_t serial) {
    if (!mCtx->configResponse) return ScopedAStatus::ok();
    mCtx->configResponse->getNumOfLiveModemsResponse(mCtx->noError(serial), 1);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioConfig::setNumOfLiveModems(int32_t serial, int8_t) {
    if (!mCtx->configResponse) return ScopedAStatus::ok();
    mCtx->configResponse->setNumOfLiveModemsResponse(mCtx->noError(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioConfig::getHalDeviceCapabilities(int32_t serial) {
    if (!mCtx->configResponse) return ScopedAStatus::ok();
    mCtx->configResponse->getHalDeviceCapabilitiesResponse(mCtx->noError(serial), false);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioConfig::rebootModem(int32_t serial) {
    if (!mCtx->configResponse) return ScopedAStatus::ok();
    mCtx->configResponse->rebootModemResponse(mCtx->noError(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioConfig::getSimultaneousCallingSupport(int32_t serial) {
    if (!mCtx->configResponse) return ScopedAStatus::ok();
    mCtx->configResponse->getSimultaneousCallingSupportResponse(mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioConfig::getSimTypeInfo(int32_t serial) {
    if (!mCtx->configResponse) return ScopedAStatus::ok();
    mCtx->configResponse->getSimTypeInfoResponse(mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioConfig::setSimType(int32_t serial,
                                      const std::vector<ar::config::SimType>&) {
    if (!mCtx->configResponse) return ScopedAStatus::ok();
    mCtx->configResponse->setSimTypeResponse(mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

}  // namespace android::hardware::radio::shim
