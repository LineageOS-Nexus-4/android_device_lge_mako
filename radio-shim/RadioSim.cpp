#include "RadioSim.h"
#include "Structs.h"

#include <android-base/logging.h>

namespace android::hardware::radio::shim {

using ::ndk::ScopedAStatus;

ScopedAStatus RadioSim::setResponseFunctions(
        const std::shared_ptr<ar::sim::IRadioSimResponse>& response,
        const std::shared_ptr<ar::sim::IRadioSimIndication>& indication) {
    mCtx->simResponse = response;
    mCtx->simIndication = indication;
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::responseAcknowledgement() {
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::getIccCardStatus(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_GET_ICC_CARD_STATUS);
    const auto status = mCtx->radio->getIccCardStatus(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getIccCardStatus HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::iccIoForApp(int32_t serial, const ar::sim::IccIo& iccIo) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_ICC_IO_FOR_APP);
    const auto status = mCtx->radio->iccIOForApp(serial, toHidl(iccIo));
    if (!status.isOk()) {
        LOG(ERROR) << "iccIOForApp HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::getImsiForApp(int32_t serial, const std::string& aid) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_GET_IMSI_FOR_APP);
    const auto status = mCtx->radio->getImsiForApp(serial, aid);
    if (!status.isOk()) {
        LOG(ERROR) << "getImsiForApp HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::supplyIccPinForApp(int32_t serial, const std::string& pin,
                                           const std::string& aid) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_SUPPLY_ICC_PIN_FOR_APP);
    const auto status = mCtx->radio->supplyIccPinForApp(serial, pin, aid);
    if (!status.isOk()) {
        LOG(ERROR) << "supplyIccPinForApp HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::supplyIccPukForApp(int32_t serial, const std::string& puk,
                                           const std::string& pin, const std::string& aid) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_SUPPLY_ICC_PUK_FOR_APP);
    const auto status = mCtx->radio->supplyIccPukForApp(serial, puk, pin, aid);
    if (!status.isOk()) {
        LOG(ERROR) << "supplyIccPukForApp HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::supplyIccPin2ForApp(int32_t serial, const std::string& pin2,
                                            const std::string& aid) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_SUPPLY_ICC_PIN2_FOR_APP);
    const auto status = mCtx->radio->supplyIccPin2ForApp(serial, pin2, aid);
    if (!status.isOk()) {
        LOG(ERROR) << "supplyIccPin2ForApp HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::supplyIccPuk2ForApp(int32_t serial, const std::string& puk2,
                                            const std::string& pin2, const std::string& aid) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_SUPPLY_ICC_PUK2_FOR_APP);
    const auto status = mCtx->radio->supplyIccPuk2ForApp(serial, puk2, pin2, aid);
    if (!status.isOk()) {
        LOG(ERROR) << "supplyIccPuk2ForApp HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::changeIccPinForApp(int32_t serial, const std::string& oldPin,
                                           const std::string& newPin, const std::string& aid) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_CHANGE_ICC_PIN_FOR_APP);
    const auto status = mCtx->radio->changeIccPinForApp(serial, oldPin, newPin, aid);
    if (!status.isOk()) {
        LOG(ERROR) << "changeIccPinForApp HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::changeIccPin2ForApp(int32_t serial, const std::string& oldPin2,
                                            const std::string& newPin2, const std::string& aid) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_CHANGE_ICC_PIN2_FOR_APP);
    const auto status = mCtx->radio->changeIccPin2ForApp(serial, oldPin2, newPin2, aid);
    if (!status.isOk()) {
        LOG(ERROR) << "changeIccPin2ForApp HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::getFacilityLockForApp(int32_t serial, const std::string& facility,
                                              const std::string& password, int32_t serviceClass,
                                              const std::string& aid) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_GET_FACILITY_LOCK_FOR_APP);
    const auto status = mCtx->radio->getFacilityLockForApp(serial, facility, password,
                                                           serviceClass, aid);
    if (!status.isOk()) {
        LOG(ERROR) << "getFacilityLockForApp HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::setFacilityLockForApp(int32_t serial, const std::string& facility,
                                              bool lockState, const std::string& password,
                                              int32_t serviceClass,
                                              const std::string& aid) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_SET_FACILITY_LOCK_FOR_APP);
    const auto status = mCtx->radio->setFacilityLockForApp(
            serial, facility, lockState, password, serviceClass, aid);
    if (!status.isOk()) {
        LOG(ERROR) << "setFacilityLockForApp HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::setSimCardPower(int32_t serial, ar::sim::CardPowerState powerUp) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_SET_SIM_CARD_POWER);
    const auto status = mCtx->radio->setSimCardPower_1_1(
            serial, static_cast<R11::CardPowerState>(powerUp));
    if (!status.isOk()) {
        LOG(ERROR) << "setSimCardPower_1_1 HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::reportStkServiceIsRunning(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_REPORT_STK_SERVICE_IS_RUNNING);
    const auto status = mCtx->radio->reportStkServiceIsRunning(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "reportStkServiceIsRunning HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::sendEnvelope(int32_t serial, const std::string& contents) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_SEND_ENVELOPE);
    const auto status = mCtx->radio->sendEnvelope(serial, contents);
    if (!status.isOk()) {
        LOG(ERROR) << "sendEnvelope HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::sendEnvelopeWithStatus(int32_t serial, const std::string& contents) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_SEND_ENVELOPE_WITH_STATUS);
    const auto status = mCtx->radio->sendEnvelopeWithStatus(serial, contents);
    if (!status.isOk()) {
        LOG(ERROR) << "sendEnvelopeWithStatus HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::sendTerminalResponseToSim(int32_t serial, const std::string& contents) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_SEND_TERMINAL_RESPONSE_TO_SIM);
    const auto status = mCtx->radio->sendTerminalResponseToSim(serial, contents);
    if (!status.isOk()) {
        LOG(ERROR) << "sendTerminalResponseToSim HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::requestIccSimAuthentication(int32_t serial, int32_t authContext,
                                                    const std::string& authData,
                                                    const std::string& aid) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_REQUEST_ICC_SIM_AUTHENTICATION);
    const auto status =
            mCtx->radio->requestIccSimAuthentication(serial, authContext, authData, aid);
    if (!status.isOk()) {
        LOG(ERROR) << "requestIccSimAuthentication HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::iccTransmitApduBasicChannel(int32_t serial,
                                                    const ar::sim::SimApdu& message) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_ICC_TRANSMIT_APDU_BASIC_CHANNEL);
    R10::SimApdu apdu;
    apdu.sessionId = message.sessionId;
    apdu.cla = message.cla;
    apdu.instruction = message.instruction;
    apdu.p1 = message.p1;
    apdu.p2 = message.p2;
    apdu.p3 = message.p3;
    apdu.data = message.data;
    const auto status = mCtx->radio->iccTransmitApduBasicChannel(serial, apdu);
    if (!status.isOk()) {
        LOG(ERROR) << "iccTransmitApduBasicChannel HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::iccOpenLogicalChannel(int32_t serial, const std::string& aid,
                                              int32_t p2) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_ICC_OPEN_LOGICAL_CHANNEL);
    const auto status = mCtx->radio->iccOpenLogicalChannel(serial, aid, p2);
    if (!status.isOk()) {
        LOG(ERROR) << "iccOpenLogicalChannel HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::iccCloseLogicalChannel(int32_t serial, int32_t channelId) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_ICC_CLOSE_LOGICAL_CHANNEL);
    const auto status = mCtx->radio->iccCloseLogicalChannel(serial, channelId);
    if (!status.isOk()) {
        LOG(ERROR) << "iccCloseLogicalChannel HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::iccTransmitApduLogicalChannel(int32_t serial,
                                                      const ar::sim::SimApdu& message) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_ICC_TRANSMIT_APDU_LOGICAL_CHANNEL);
    R10::SimApdu apdu;
    apdu.sessionId = message.sessionId;
    apdu.cla = message.cla;
    apdu.instruction = message.instruction;
    apdu.p1 = message.p1;
    apdu.p2 = message.p2;
    apdu.p3 = message.p3;
    apdu.data = message.data;
    const auto status = mCtx->radio->iccTransmitApduLogicalChannel(serial, apdu);
    if (!status.isOk()) {
        LOG(ERROR) << "iccTransmitApduLogicalChannel HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::setCarrierInfoForImsiEncryption(
        int32_t serial, const ar::sim::ImsiEncryptionInfo& imsiEncryptionInfo) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::SIM_SET_CARRIER_INFO_FOR_IMSI_ENCRYPTION);
    R11::ImsiEncryptionInfo info;
    info.mcc = imsiEncryptionInfo.mcc;
    info.mnc = imsiEncryptionInfo.mnc;
    info.carrierKey = imsiEncryptionInfo.carrierKey;
    info.keyIdentifier = imsiEncryptionInfo.keyIdentifier;
    info.expirationTime = imsiEncryptionInfo.expirationTime;
    const auto status = mCtx->radio->setCarrierInfoForImsiEncryption(serial, info);
    if (!status.isOk()) {
        LOG(ERROR) << "setCarrierInfoForImsiEncryption HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// Unsupported: respond REQUEST_NOT_SUPPORTED.
// ---------------------------------------------------------------------------

ScopedAStatus RadioSim::areUiccApplicationsEnabled(int32_t serial) {
    if (mCtx->simResponse) mCtx->simResponse->areUiccApplicationsEnabledResponse(
            mCtx->notSupported(serial), false);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::enableUiccApplications(int32_t serial, bool) {
    if (mCtx->simResponse) mCtx->simResponse->enableUiccApplicationsResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::getAllowedCarriers(int32_t serial) {
    if (mCtx->simResponse) mCtx->simResponse->getAllowedCarriersResponse(
            mCtx->notSupported(serial), {}, ar::sim::SimLockMultiSimPolicy::NO_MULTISIM_POLICY);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::setAllowedCarriers(int32_t serial,
                                           const ar::sim::CarrierRestrictions&,
                                           ar::sim::SimLockMultiSimPolicy) {
    if (mCtx->simResponse) mCtx->simResponse->setAllowedCarriersResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::getCdmaSubscription(int32_t serial) {
    if (mCtx->simResponse) mCtx->simResponse->getCdmaSubscriptionResponse(
            mCtx->notSupported(serial), "", "", "", "", "");
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::getCdmaSubscriptionSource(int32_t serial) {
    if (mCtx->simResponse) mCtx->simResponse->getCdmaSubscriptionSourceResponse(
            mCtx->notSupported(serial), ar::sim::CdmaSubscriptionSource::RUIM_SIM);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::setCdmaSubscriptionSource(int32_t serial,
                                                  ar::sim::CdmaSubscriptionSource) {
    if (mCtx->simResponse) mCtx->simResponse->setCdmaSubscriptionSourceResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::getSimPhonebookCapacity(int32_t serial) {
    if (mCtx->simResponse) mCtx->simResponse->getSimPhonebookCapacityResponse(
            mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::getSimPhonebookRecords(int32_t serial) {
    if (mCtx->simResponse) mCtx->simResponse->getSimPhonebookRecordsResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::updateSimPhonebookRecords(int32_t serial,
                                                  const ar::sim::PhonebookRecordInfo&) {
    if (mCtx->simResponse) mCtx->simResponse->updateSimPhonebookRecordsResponse(
            mCtx->notSupported(serial), 0);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::setUiccSubscription(int32_t serial, const ar::sim::SelectUiccSub&) {
    if (mCtx->simResponse) mCtx->simResponse->setUiccSubscriptionResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::supplySimDepersonalization(int32_t serial, ar::sim::PersoSubstate,
                                                   const std::string&) {
    if (mCtx->simResponse) mCtx->simResponse->supplySimDepersonalizationResponse(
            mCtx->notSupported(serial), ar::sim::PersoSubstate::UNKNOWN, 0);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioSim::iccCloseLogicalChannelWithSessionInfo(
        int32_t serial, const ar::sim::SessionInfo&) {
    if (mCtx->simResponse) mCtx->simResponse->iccCloseLogicalChannelWithSessionInfoResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

}  // namespace android::hardware::radio::shim
