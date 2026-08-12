#pragma once

#include <aidl/android/hardware/radio/config/BnRadioConfig.h>

#include "ShimContext.h"

namespace android::hardware::radio::shim {

// Single-slot synthetic RadioConfig: no HIDL backend. SIM slot state is
// synthesized from the V1.1 getIccCardStatus response (request forwarded
// under the CONFIG_GET_SIM_SLOTS_STATUS kind).
class RadioConfig : public ar::config::BnRadioConfig {
  public:
    explicit RadioConfig(std::shared_ptr<ShimContext> ctx) : mCtx(std::move(ctx)) {}

    ::ndk::ScopedAStatus getHalDeviceCapabilities(int32_t serial) override;
    ::ndk::ScopedAStatus getNumOfLiveModems(int32_t serial) override;
    ::ndk::ScopedAStatus getPhoneCapability(int32_t serial) override;
    ::ndk::ScopedAStatus getSimSlotsStatus(int32_t serial) override;
    ::ndk::ScopedAStatus setNumOfLiveModems(int32_t serial, int8_t numOfLiveModems) override;
    ::ndk::ScopedAStatus setPreferredDataModem(int32_t serial, int8_t modemId) override;
    ::ndk::ScopedAStatus setResponseFunctions(
            const std::shared_ptr<ar::config::IRadioConfigResponse>& radioConfigResponse,
            const std::shared_ptr<ar::config::IRadioConfigIndication>& radioConfigIndication) override;
    ::ndk::ScopedAStatus setSimSlotsMapping(
            int32_t serial, const std::vector<ar::config::SlotPortMapping>& slotMap) override;
    ::ndk::ScopedAStatus getSimultaneousCallingSupport(int32_t serial) override;
    ::ndk::ScopedAStatus getSimTypeInfo(int32_t serial) override;
    ::ndk::ScopedAStatus setSimType(int32_t serial,
                                    const std::vector<ar::config::SimType>& simTypes) override;
    ::ndk::ScopedAStatus rebootModem(int32_t serial) override;

  private:
    std::shared_ptr<ShimContext> mCtx;
};

}  // namespace android::hardware::radio::shim
