#pragma once

#include <aidl/android/hardware/radio/modem/BnRadioModem.h>

#include "ShimContext.h"

namespace android::hardware::radio::shim {

class RadioModem : public ar::modem::BnRadioModem {
  public:
    explicit RadioModem(std::shared_ptr<ShimContext> ctx) : mCtx(std::move(ctx)) {}

    ::ndk::ScopedAStatus enableModem(int32_t serial, bool on) override;
    ::ndk::ScopedAStatus getBasebandVersion(int32_t serial) override;
    ::ndk::ScopedAStatus getDeviceIdentity(int32_t serial) override;
    ::ndk::ScopedAStatus getHardwareConfig(int32_t serial) override;
    ::ndk::ScopedAStatus getModemActivityInfo(int32_t serial) override;
    ::ndk::ScopedAStatus getModemStackStatus(int32_t serial) override;
    ::ndk::ScopedAStatus getRadioCapability(int32_t serial) override;
    ::ndk::ScopedAStatus nvReadItem(int32_t serial, ar::modem::NvItem itemId) override;
    ::ndk::ScopedAStatus nvResetConfig(int32_t serial, ar::modem::ResetNvType resetType) override;
    ::ndk::ScopedAStatus nvWriteCdmaPrl(int32_t serial,
                                        const std::vector<uint8_t>& prl) override;
    ::ndk::ScopedAStatus nvWriteItem(int32_t serial,
                                     const ar::modem::NvWriteItem& item) override;
    ::ndk::ScopedAStatus requestShutdown(int32_t serial) override;
    ::ndk::ScopedAStatus responseAcknowledgement() override;
    ::ndk::ScopedAStatus sendDeviceState(int32_t serial, ar::modem::DeviceStateType deviceStateType,
                                         bool state) override;
    ::ndk::ScopedAStatus setRadioCapability(int32_t serial,
                                            const ar::modem::RadioCapability& rc) override;
    ::ndk::ScopedAStatus setRadioPower(int32_t serial, bool powerOn, bool forEmergencyCall,
                                       bool preferredForEmergencyCall) override;
    ::ndk::ScopedAStatus setResponseFunctions(
            const std::shared_ptr<ar::modem::IRadioModemResponse>& radioModemResponse,
            const std::shared_ptr<ar::modem::IRadioModemIndication>& radioModemIndication) override;
    ::ndk::ScopedAStatus getImei(int32_t serial) override;

  private:
    std::shared_ptr<ShimContext> mCtx;
};

}  // namespace android::hardware::radio::shim
