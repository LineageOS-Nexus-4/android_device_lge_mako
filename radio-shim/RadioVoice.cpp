#include "RadioVoice.h"

#include <android-base/logging.h>

namespace android::hardware::radio::shim {

using ::ndk::ScopedAStatus;

ScopedAStatus RadioVoice::setResponseFunctions(
        const std::shared_ptr<ar::voice::IRadioVoiceResponse>& response,
        const std::shared_ptr<ar::voice::IRadioVoiceIndication>& indication) {
    mCtx->voiceResponse = response;
    mCtx->voiceIndication = indication;
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::responseAcknowledgement() {
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::getCurrentCalls(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_GET_CURRENT_CALLS);
    const auto status = mCtx->radio->getCurrentCalls(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getCurrentCalls HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::dial(int32_t serial, const ar::voice::Dial& dialInfo) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_DIAL);
    R10::Dial hidl;
    hidl.address = dialInfo.address;
    hidl.clir = static_cast<R10::Clir>(dialInfo.clir);
    std::vector<R10::UusInfo> uusInfos;
    for (const auto& u : dialInfo.uusInfo) {
        R10::UusInfo uus;
        uus.uusType = static_cast<R10::UusType>(u.uusType);
        uus.uusDcs = static_cast<R10::UusDcs>(u.uusDcs);
        uus.uusData = u.uusData;
        uusInfos.push_back(uus);
    }
    hidl.uusInfo = uusInfos;
    const auto status = mCtx->radio->dial(serial, hidl);
    if (!status.isOk()) {
        LOG(ERROR) << "dial HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::acceptCall(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_ACCEPT_CALL);
    const auto status = mCtx->radio->acceptCall(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "acceptCall HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::rejectCall(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_REJECT_CALL);
    const auto status = mCtx->radio->rejectCall(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "rejectCall HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::hangup(int32_t serial, int32_t gsmIndex) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_HANGUP);
    const auto status = mCtx->radio->hangup(serial, gsmIndex);
    if (!status.isOk()) {
        LOG(ERROR) << "hangup HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::hangupWaitingOrBackground(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_HANGUP_WAITING_OR_BACKGROUND);
    const auto status = mCtx->radio->hangupWaitingOrBackground(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "hangupWaitingOrBackground HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::hangupForegroundResumeBackground(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_HANGUP_FOREGROUND_RESUME_BACKGROUND);
    const auto status = mCtx->radio->hangupForegroundResumeBackground(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "hangupForegroundResumeBackground HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::switchWaitingOrHoldingAndActive(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE);
    const auto status = mCtx->radio->switchWaitingOrHoldingAndActive(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "switchWaitingOrHoldingAndActive HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::conference(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_CONFERENCE);
    const auto status = mCtx->radio->conference(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "conference HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::separateConnection(int32_t serial, int32_t gsmIndex) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_SEPARATE_CONNECTION);
    const auto status = mCtx->radio->separateConnection(serial, gsmIndex);
    if (!status.isOk()) {
        LOG(ERROR) << "separateConnection HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::getLastCallFailCause(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_GET_LAST_CALL_FAIL_CAUSE);
    const auto status = mCtx->radio->getLastCallFailCause(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getLastCallFailCause HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::sendDtmf(int32_t serial, const std::string& s) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_SEND_DTMF);
    const auto status = mCtx->radio->sendDtmf(serial, s);
    if (!status.isOk()) {
        LOG(ERROR) << "sendDtmf HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::startDtmf(int32_t serial, const std::string& s) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_START_DTMF);
    const auto status = mCtx->radio->startDtmf(serial, s);
    if (!status.isOk()) {
        LOG(ERROR) << "startDtmf HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::stopDtmf(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_STOP_DTMF);
    const auto status = mCtx->radio->stopDtmf(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "stopDtmf HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::sendUssd(int32_t serial, const std::string& ussd) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_SEND_USSD);
    const auto status = mCtx->radio->sendUssd(serial, ussd);
    if (!status.isOk()) {
        LOG(ERROR) << "sendUssd HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::cancelPendingUssd(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_CANCEL_PENDING_USSD);
    const auto status = mCtx->radio->cancelPendingUssd(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "cancelPendingUssd HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::getClir(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_GET_CLIR);
    const auto status = mCtx->radio->getClir(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getClir HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::setClir(int32_t serial, int32_t status) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_SET_CLIR);
    const auto r = mCtx->radio->setClir(serial, status);
    if (!r.isOk()) {
        LOG(ERROR) << "setClir HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::getClip(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_GET_CLIP);
    const auto status = mCtx->radio->getClip(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getClip HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::getCallForwardStatus(int32_t serial,
                                               const ar::voice::CallForwardInfo& callInfo) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_GET_CALL_FORWARD_STATUS);
    R10::CallForwardInfo hidl;
    hidl.status = static_cast<R10::CallForwardInfoStatus>(callInfo.status);
    hidl.reason = callInfo.reason;
    hidl.serviceClass = callInfo.serviceClass;
    hidl.toa = callInfo.toa;
    hidl.number = callInfo.number;
    hidl.timeSeconds = callInfo.timeSeconds;
    const auto status = mCtx->radio->getCallForwardStatus(serial, hidl);
    if (!status.isOk()) {
        LOG(ERROR) << "getCallForwardStatus HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::setCallForward(int32_t serial,
                                         const ar::voice::CallForwardInfo& callInfo) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_SET_CALL_FORWARD);
    R10::CallForwardInfo hidl;
    hidl.status = static_cast<R10::CallForwardInfoStatus>(callInfo.status);
    hidl.reason = callInfo.reason;
    hidl.serviceClass = callInfo.serviceClass;
    hidl.toa = callInfo.toa;
    hidl.number = callInfo.number;
    hidl.timeSeconds = callInfo.timeSeconds;
    const auto status = mCtx->radio->setCallForward(serial, hidl);
    if (!status.isOk()) {
        LOG(ERROR) << "setCallForward HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::getCallWaiting(int32_t serial, int32_t serviceClass) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_GET_CALL_WAITING);
    const auto status = mCtx->radio->getCallWaiting(serial, serviceClass);
    if (!status.isOk()) {
        LOG(ERROR) << "getCallWaiting HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::setCallWaiting(int32_t serial, bool enable, int32_t serviceClass) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_SET_CALL_WAITING);
    const auto status = mCtx->radio->setCallWaiting(serial, enable, serviceClass);
    if (!status.isOk()) {
        LOG(ERROR) << "setCallWaiting HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::getMute(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_GET_MUTE);
    const auto status = mCtx->radio->getMute(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getMute HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::setMute(int32_t serial, bool enable) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_SET_MUTE);
    const auto status = mCtx->radio->setMute(serial, enable);
    if (!status.isOk()) {
        LOG(ERROR) << "setMute HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::explicitCallTransfer(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_EXPLICIT_CALL_TRANSFER);
    const auto status = mCtx->radio->explicitCallTransfer(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "explicitCallTransfer HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::handleStkCallSetupRequestFromSim(int32_t serial, bool accept) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::VOICE_HANDLE_STK_CALL_SETUP_REQUEST_FROM_SIM);
    const auto status = mCtx->radio->handleStkCallSetupRequestFromSim(serial, accept);
    if (!status.isOk()) {
        LOG(ERROR) << "handleStkCallSetupRequestFromSim HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// Unsupported: respond REQUEST_NOT_SUPPORTED.
// ---------------------------------------------------------------------------

ScopedAStatus RadioVoice::emergencyDial(int32_t serial, const ar::voice::Dial&, int32_t,
                                        const std::vector<std::string>&,
                                        ar::voice::EmergencyCallRouting, bool, bool) {
    if (mCtx->voiceResponse) mCtx->voiceResponse->emergencyDialResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::exitEmergencyCallbackMode(int32_t serial) {
    if (mCtx->voiceResponse) mCtx->voiceResponse->exitEmergencyCallbackModeResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::getPreferredVoicePrivacy(int32_t serial) {
    if (mCtx->voiceResponse) mCtx->voiceResponse->getPreferredVoicePrivacyResponse(
            mCtx->notSupported(serial), false);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::setPreferredVoicePrivacy(int32_t serial, bool) {
    if (mCtx->voiceResponse) mCtx->voiceResponse->setPreferredVoicePrivacyResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::getTtyMode(int32_t serial) {
    if (mCtx->voiceResponse) mCtx->voiceResponse->getTtyModeResponse(
            mCtx->notSupported(serial), ar::voice::TtyMode::OFF);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::setTtyMode(int32_t serial, ar::voice::TtyMode) {
    if (mCtx->voiceResponse) mCtx->voiceResponse->setTtyModeResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::isVoNrEnabled(int32_t serial) {
    if (mCtx->voiceResponse) mCtx->voiceResponse->isVoNrEnabledResponse(
            mCtx->notSupported(serial), false);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::setVoNrEnabled(int32_t serial, bool) {
    if (mCtx->voiceResponse) mCtx->voiceResponse->setVoNrEnabledResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::sendBurstDtmf(int32_t serial, const std::string&, int32_t, int32_t) {
    if (mCtx->voiceResponse) mCtx->voiceResponse->sendBurstDtmfResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioVoice::sendCdmaFeatureCode(int32_t serial, const std::string&) {
    if (mCtx->voiceResponse) mCtx->voiceResponse->sendCdmaFeatureCodeResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

}  // namespace android::hardware::radio::shim
