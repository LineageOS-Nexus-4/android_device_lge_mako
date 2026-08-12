#pragma once

#include <aidl/android/hardware/radio/messaging/BnRadioMessaging.h>

#include "ShimContext.h"

namespace android::hardware::radio::shim {

class RadioMessaging : public ar::messaging::BnRadioMessaging {
  public:
    explicit RadioMessaging(std::shared_ptr<ShimContext> ctx) : mCtx(std::move(ctx)) {}

    ::ndk::ScopedAStatus acknowledgeIncomingGsmSmsWithPdu(int32_t serial, bool success,
                                                          const std::string& ackPdu) override;
    ::ndk::ScopedAStatus acknowledgeLastIncomingCdmaSms(
            int32_t serial, const ar::messaging::CdmaSmsAck& smsAck) override;
    ::ndk::ScopedAStatus acknowledgeLastIncomingGsmSms(
            int32_t serial, bool success, ar::messaging::SmsAcknowledgeFailCause cause) override;
    ::ndk::ScopedAStatus deleteSmsOnRuim(int32_t serial, int32_t index) override;
    ::ndk::ScopedAStatus deleteSmsOnSim(int32_t serial, int32_t index) override;
    ::ndk::ScopedAStatus getCdmaBroadcastConfig(int32_t serial) override;
    ::ndk::ScopedAStatus getGsmBroadcastConfig(int32_t serial) override;
    ::ndk::ScopedAStatus getSmscAddress(int32_t serial) override;
    ::ndk::ScopedAStatus reportSmsMemoryStatus(int32_t serial, bool available) override;
    ::ndk::ScopedAStatus responseAcknowledgement() override;
    ::ndk::ScopedAStatus sendCdmaSms(int32_t serial,
                                     const ar::messaging::CdmaSmsMessage& sms) override;
    ::ndk::ScopedAStatus sendCdmaSmsExpectMore(int32_t serial,
                                               const ar::messaging::CdmaSmsMessage& sms) override;
    ::ndk::ScopedAStatus sendImsSms(int32_t serial,
                                    const ar::messaging::ImsSmsMessage& message) override;
    ::ndk::ScopedAStatus sendSms(int32_t serial,
                                 const ar::messaging::GsmSmsMessage& message) override;
    ::ndk::ScopedAStatus sendSmsExpectMore(int32_t serial,
                                           const ar::messaging::GsmSmsMessage& message) override;
    ::ndk::ScopedAStatus setCdmaBroadcastActivation(int32_t serial, bool activate) override;
    ::ndk::ScopedAStatus setCdmaBroadcastConfig(
            int32_t serial,
            const std::vector<ar::messaging::CdmaBroadcastSmsConfigInfo>& configInfo) override;
    ::ndk::ScopedAStatus setGsmBroadcastActivation(int32_t serial, bool activate) override;
    ::ndk::ScopedAStatus setGsmBroadcastConfig(
            int32_t serial,
            const std::vector<ar::messaging::GsmBroadcastSmsConfigInfo>& configInfo) override;
    ::ndk::ScopedAStatus setResponseFunctions(
            const std::shared_ptr<ar::messaging::IRadioMessagingResponse>& radioMessagingResponse,
            const std::shared_ptr<ar::messaging::IRadioMessagingIndication>&
                    radioMessagingIndication) override;
    ::ndk::ScopedAStatus setSmscAddress(int32_t serial, const std::string& smsc) override;
    ::ndk::ScopedAStatus writeSmsToRuim(int32_t serial,
                                        const ar::messaging::CdmaSmsWriteArgs& cdmaSms) override;
    ::ndk::ScopedAStatus writeSmsToSim(int32_t serial,
                                       const ar::messaging::SmsWriteArgs& smsWriteArgs) override;

  private:
    std::shared_ptr<ShimContext> mCtx;
};

}  // namespace android::hardware::radio::shim
