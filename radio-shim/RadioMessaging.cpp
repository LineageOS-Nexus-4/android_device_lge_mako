#include "RadioMessaging.h"
#include "Structs.h"

#include <android-base/logging.h>

namespace android::hardware::radio::shim {

using ::ndk::ScopedAStatus;

ScopedAStatus RadioMessaging::setResponseFunctions(
        const std::shared_ptr<ar::messaging::IRadioMessagingResponse>& response,
        const std::shared_ptr<ar::messaging::IRadioMessagingIndication>& indication) {
    mCtx->messagingResponse = response;
    mCtx->messagingIndication = indication;
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::responseAcknowledgement() {
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::sendSms(int32_t serial,
                                      const ar::messaging::GsmSmsMessage& message) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MSG_SEND_SMS);
    const auto status = mCtx->radio->sendSms(serial, toHidl(message));
    if (!status.isOk()) {
        LOG(ERROR) << "sendSms HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::sendSmsExpectMore(int32_t serial,
                                                const ar::messaging::GsmSmsMessage& message) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MSG_SEND_SMS_EXPECT_MORE);
    const auto status = mCtx->radio->sendSMSExpectMore(serial, toHidl(message));
    if (!status.isOk()) {
        LOG(ERROR) << "sendSMSExpectMore HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::acknowledgeLastIncomingGsmSms(
        int32_t serial, bool success, ar::messaging::SmsAcknowledgeFailCause cause) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MSG_ACK_LAST_INCOMING_GSM_SMS);
    const auto status =
            mCtx->radio->acknowledgeLastIncomingGsmSms(serial, success, static_cast<R10::SmsAcknowledgeFailCause>(cause));
    if (!status.isOk()) {
        LOG(ERROR) << "acknowledgeLastIncomingGsmSms HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::acknowledgeIncomingGsmSmsWithPdu(int32_t serial, bool success,
                                                               const std::string& ackPdu) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MSG_ACK_INCOMING_GSM_SMS_WITH_PDU);
    const auto status = mCtx->radio->acknowledgeIncomingGsmSmsWithPdu(serial, success, ackPdu);
    if (!status.isOk()) {
        LOG(ERROR) << "acknowledgeIncomingGsmSmsWithPdu HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::getSmscAddress(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MSG_GET_SMSC_ADDRESS);
    const auto status = mCtx->radio->getSmscAddress(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getSmscAddress HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::setSmscAddress(int32_t serial, const std::string& smsc) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MSG_SET_SMSC_ADDRESS);
    const auto status = mCtx->radio->setSmscAddress(serial, smsc);
    if (!status.isOk()) {
        LOG(ERROR) << "setSmscAddress HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::writeSmsToSim(int32_t serial,
                                            const ar::messaging::SmsWriteArgs& smsWriteArgs) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MSG_WRITE_SMS_TO_SIM);
    const auto status = mCtx->radio->writeSmsToSim(serial, toHidl(smsWriteArgs));
    if (!status.isOk()) {
        LOG(ERROR) << "writeSmsToSim HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::deleteSmsOnSim(int32_t serial, int32_t index) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MSG_DELETE_SMS_ON_SIM);
    const auto status = mCtx->radio->deleteSmsOnSim(serial, index);
    if (!status.isOk()) {
        LOG(ERROR) << "deleteSmsOnSim HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::getGsmBroadcastConfig(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MSG_GET_GSM_BROADCAST_CONFIG);
    const auto status = mCtx->radio->getGsmBroadcastConfig(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getGsmBroadcastConfig HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::setGsmBroadcastConfig(
        int32_t serial,
        const std::vector<ar::messaging::GsmBroadcastSmsConfigInfo>& configInfo) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MSG_SET_GSM_BROADCAST_CONFIG);
    std::vector<R10::GsmBroadcastSmsConfigInfo> hidl;
    for (const auto& c : configInfo) {
        R10::GsmBroadcastSmsConfigInfo cfg;
        cfg.fromServiceId = c.fromServiceId;
        cfg.toServiceId = c.toServiceId;
        cfg.fromCodeScheme = c.fromCodeScheme;
        cfg.toCodeScheme = c.toCodeScheme;
        cfg.selected = c.selected;
        hidl.push_back(cfg);
    }
    const auto status = mCtx->radio->setGsmBroadcastConfig(serial, hidl);
    if (!status.isOk()) {
        LOG(ERROR) << "setGsmBroadcastConfig HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::setGsmBroadcastActivation(int32_t serial, bool activate) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MSG_SET_GSM_BROADCAST_ACTIVATION);
    const auto status = mCtx->radio->setGsmBroadcastActivation(serial, activate);
    if (!status.isOk()) {
        LOG(ERROR) << "setGsmBroadcastActivation HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::reportSmsMemoryStatus(int32_t serial, bool available) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MSG_REPORT_SMS_MEMORY_STATUS);
    const auto status = mCtx->radio->reportSmsMemoryStatus(serial, available);
    if (!status.isOk()) {
        LOG(ERROR) << "reportSmsMemoryStatus HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// Unsupported (CDMA / IMS SMS).
// ---------------------------------------------------------------------------

ScopedAStatus RadioMessaging::acknowledgeLastIncomingCdmaSms(
        int32_t serial, const ar::messaging::CdmaSmsAck&) {
    if (mCtx->messagingResponse) mCtx->messagingResponse->acknowledgeLastIncomingCdmaSmsResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::deleteSmsOnRuim(int32_t serial, int32_t) {
    if (mCtx->messagingResponse) mCtx->messagingResponse->deleteSmsOnRuimResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::getCdmaBroadcastConfig(int32_t serial) {
    if (mCtx->messagingResponse) mCtx->messagingResponse->getCdmaBroadcastConfigResponse(
            mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::sendCdmaSms(int32_t serial, const ar::messaging::CdmaSmsMessage&) {
    if (mCtx->messagingResponse) mCtx->messagingResponse->sendCdmaSmsResponse(
            mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::sendCdmaSmsExpectMore(int32_t serial,
                                                    const ar::messaging::CdmaSmsMessage&) {
    if (mCtx->messagingResponse) mCtx->messagingResponse->sendCdmaSmsExpectMoreResponse(
            mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::sendImsSms(int32_t serial,
                                         const ar::messaging::ImsSmsMessage&) {
    if (mCtx->messagingResponse) mCtx->messagingResponse->sendImsSmsResponse(
            mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::setCdmaBroadcastActivation(int32_t serial, bool) {
    if (mCtx->messagingResponse) mCtx->messagingResponse->setCdmaBroadcastActivationResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::setCdmaBroadcastConfig(
        int32_t serial, const std::vector<ar::messaging::CdmaBroadcastSmsConfigInfo>&) {
    if (mCtx->messagingResponse) mCtx->messagingResponse->setCdmaBroadcastConfigResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioMessaging::writeSmsToRuim(int32_t serial,
                                             const ar::messaging::CdmaSmsWriteArgs&) {
    if (mCtx->messagingResponse) mCtx->messagingResponse->writeSmsToRuimResponse(
            mCtx->notSupported(serial), 0);
    return ScopedAStatus::ok();
}

}  // namespace android::hardware::radio::shim
