#include "IndicationHandler.h"
#include "Structs.h"

#include <aidl/android/hardware/radio/network/SuppSvcNotification.h>
#include <aidl/android/hardware/radio/sim/SimRefreshResult.h>
#include <aidl/android/hardware/radio/voice/CdmaSignalInfoRecord.h>

#include <android-base/logging.h>

namespace android::hardware::radio::shim {

namespace ar = ::aidl::android::hardware::radio;

void IndicationHandler::callStatus(const char* method, const ::ndk::ScopedAStatus& status) {
    if (!status.isOk()) {
        LOG(WARNING) << "AIDL indication " << method << " failed: " << status.getDescription();
    }
}

void IndicationHandler::drop(const char* method, R10::RadioIndicationType) {
    LOG(VERBOSE) << method << ": dropped (unsupported on mako)";
}

Return<void> IndicationHandler::radioStateChanged(R10::RadioIndicationType type,
                                                  R10::RadioState radioState) {
    mCtx->onRadioStateChanged(static_cast<ar::RadioIndicationType>(type),
                              static_cast<ar::modem::RadioState>(radioState));
    return Void();
}

Return<void> IndicationHandler::callStateChanged(R10::RadioIndicationType type) {
    if (mCtx->voiceIndication) {
        callStatus(__func__, mCtx->voiceIndication->callStateChanged(
                                     static_cast<ar::RadioIndicationType>(type)));
    }
    return Void();
}

Return<void> IndicationHandler::networkStateChanged(R10::RadioIndicationType type) {
    if (mCtx->networkIndication) {
        callStatus(__func__, mCtx->networkIndication->networkStateChanged(
                                     static_cast<ar::RadioIndicationType>(type)));
    }
    return Void();
}

Return<void> IndicationHandler::newSms(R10::RadioIndicationType type,
                                       const hidl_vec<uint8_t>& pdu) {
    if (mCtx->messagingIndication) {
        std::vector<uint8_t> out(pdu.begin(), pdu.end());
        callStatus(__func__, mCtx->messagingIndication->newSms(
                                     static_cast<ar::RadioIndicationType>(type), out));
    }
    return Void();
}

Return<void> IndicationHandler::newSmsStatusReport(R10::RadioIndicationType type,
                                                   const hidl_vec<uint8_t>& pdu) {
    if (mCtx->messagingIndication) {
        std::vector<uint8_t> out(pdu.begin(), pdu.end());
        callStatus(__func__, mCtx->messagingIndication->newSmsStatusReport(
                                     static_cast<ar::RadioIndicationType>(type), out));
    }
    return Void();
}

Return<void> IndicationHandler::newSmsOnSim(R10::RadioIndicationType type,
                                            int32_t recordNumber) {
    if (mCtx->messagingIndication) {
        callStatus(__func__, mCtx->messagingIndication->newSmsOnSim(
                                     static_cast<ar::RadioIndicationType>(type), recordNumber));
    }
    return Void();
}

Return<void> IndicationHandler::onUssd(R10::RadioIndicationType type,
                                       R10::UssdModeType modeType, const hidl_string& msg) {
    if (mCtx->voiceIndication) {
        callStatus(__func__, mCtx->voiceIndication->onUssd(
                                     static_cast<ar::RadioIndicationType>(type),
                                     static_cast<ar::voice::UssdModeType>(modeType), msg));
    }
    return Void();
}

Return<void> IndicationHandler::nitzTimeReceived(R10::RadioIndicationType type,
                                                 const hidl_string& nitzTime,
                                                 uint64_t receivedTime) {
    if (mCtx->networkIndication) {
        callStatus(__func__, mCtx->networkIndication->nitzTimeReceived(
                                     static_cast<ar::RadioIndicationType>(type), nitzTime,
                                     receivedTime, 0));
    }
    return Void();
}

Return<void> IndicationHandler::currentSignalStrength(R10::RadioIndicationType type,
                                                      const R10::SignalStrength& signalStrength) {
    if (mCtx->networkIndication) {
        callStatus(__func__, mCtx->networkIndication->currentSignalStrength(
                                     static_cast<ar::RadioIndicationType>(type),
                                     toAidl(signalStrength)));
    }
    return Void();
}

Return<void> IndicationHandler::dataCallListChanged(
        R10::RadioIndicationType type, const hidl_vec<R10::SetupDataCallResult>& dcList) {
    if (mCtx->dataIndication) {
        std::vector<ar::data::SetupDataCallResult> out;
        for (const auto& r : dcList) out.push_back(toAidl(r));
        callStatus(__func__, mCtx->dataIndication->dataCallListChanged(
                                     static_cast<ar::RadioIndicationType>(type), out));
    }
    return Void();
}

Return<void> IndicationHandler::suppSvcNotify(R10::RadioIndicationType type,
                                              const R10::SuppSvcNotification& suppSvc) {
    if (mCtx->networkIndication) {
        ar::network::SuppSvcNotification out;
        out.isMT = suppSvc.isMT;
        out.code = suppSvc.code;
        out.index = suppSvc.index;
        out.type = suppSvc.type;
        out.number = suppSvc.number;
        callStatus(__func__, mCtx->networkIndication->suppSvcNotify(
                                     static_cast<ar::RadioIndicationType>(type), out));
    }
    return Void();
}

Return<void> IndicationHandler::stkSessionEnd(R10::RadioIndicationType type) {
    if (mCtx->simIndication) {
        callStatus(__func__, mCtx->simIndication->stkSessionEnd(
                                     static_cast<ar::RadioIndicationType>(type)));
    }
    return Void();
}

Return<void> IndicationHandler::stkProactiveCommand(R10::RadioIndicationType type,
                                                    const hidl_string& cmd) {
    if (mCtx->simIndication) {
        callStatus(__func__, mCtx->simIndication->stkProactiveCommand(
                                     static_cast<ar::RadioIndicationType>(type), cmd));
    }
    return Void();
}

Return<void> IndicationHandler::stkEventNotify(R10::RadioIndicationType type,
                                               const hidl_string& cmd) {
    if (mCtx->simIndication) {
        callStatus(__func__, mCtx->simIndication->stkEventNotify(
                                     static_cast<ar::RadioIndicationType>(type), cmd));
    }
    return Void();
}

Return<void> IndicationHandler::stkCallSetup(R10::RadioIndicationType type, int64_t timeout) {
    if (mCtx->voiceIndication) {
        callStatus(__func__, mCtx->voiceIndication->stkCallSetup(
                                     static_cast<ar::RadioIndicationType>(type), timeout));
    }
    return Void();
}

Return<void> IndicationHandler::simSmsStorageFull(R10::RadioIndicationType type) {
    if (mCtx->messagingIndication) {
        callStatus(__func__, mCtx->messagingIndication->simSmsStorageFull(
                                     static_cast<ar::RadioIndicationType>(type)));
    }
    return Void();
}

Return<void> IndicationHandler::simRefresh(R10::RadioIndicationType type,
                                           const R10::SimRefreshResult& refreshResult) {
    if (mCtx->simIndication) {
        ar::sim::SimRefreshResult out;
        out.type = static_cast<int32_t>(refreshResult.type);
        out.efId = refreshResult.efId;
        out.aid = refreshResult.aid;
        callStatus(__func__, mCtx->simIndication->simRefresh(
                                     static_cast<ar::RadioIndicationType>(type), out));
    }
    return Void();
}

Return<void> IndicationHandler::callRing(R10::RadioIndicationType type, bool isGsm,
                                         const R10::CdmaSignalInfoRecord& record) {
    if (mCtx->voiceIndication) {
        ar::voice::CdmaSignalInfoRecord out;
        out.isPresent = record.isPresent;
        out.signalType = record.signalType;
        out.alertPitch = record.alertPitch;
        out.signal = record.signal;
        callStatus(__func__, mCtx->voiceIndication->callRing(
                                     static_cast<ar::RadioIndicationType>(type), isGsm, out));
    }
    return Void();
}

Return<void> IndicationHandler::simStatusChanged(R10::RadioIndicationType type) {
    if (mCtx->simIndication) {
        callStatus(__func__, mCtx->simIndication->simStatusChanged(
                                     static_cast<ar::RadioIndicationType>(type)));
    }
    return Void();
}

Return<void> IndicationHandler::restrictedStateChanged(R10::RadioIndicationType type,
                                                       R10::PhoneRestrictedState state) {
    if (mCtx->networkIndication) {
        callStatus(__func__, mCtx->networkIndication->restrictedStateChanged(
                                     static_cast<ar::RadioIndicationType>(type),
                                     static_cast<ar::network::PhoneRestrictedState>(state)));
    }
    return Void();
}

Return<void> IndicationHandler::indicateRingbackTone(R10::RadioIndicationType type, bool start) {
    if (mCtx->voiceIndication) {
        callStatus(__func__, mCtx->voiceIndication->indicateRingbackTone(
                                     static_cast<ar::RadioIndicationType>(type), start));
    }
    return Void();
}

Return<void> IndicationHandler::resendIncallMute(R10::RadioIndicationType type) {
    if (mCtx->voiceIndication) {
        callStatus(__func__, mCtx->voiceIndication->resendIncallMute(
                                     static_cast<ar::RadioIndicationType>(type)));
    }
    return Void();
}

Return<void> IndicationHandler::exitEmergencyCallbackMode(R10::RadioIndicationType type) {
    if (mCtx->voiceIndication) {
        callStatus(__func__, mCtx->voiceIndication->exitEmergencyCallbackMode(
                                     static_cast<ar::RadioIndicationType>(type)));
    }
    return Void();
}

Return<void> IndicationHandler::rilConnected(R10::RadioIndicationType type) {
    if (const auto indication = mCtx->getModemIndication()) {
        callStatus(__func__, indication->rilConnected(
                                     static_cast<ar::RadioIndicationType>(type)));
    }
    return Void();
}

Return<void> IndicationHandler::voiceRadioTechChanged(R10::RadioIndicationType type,
                                                      R10::RadioTechnology rat) {
    if (mCtx->networkIndication) {
        callStatus(__func__, mCtx->networkIndication->voiceRadioTechChanged(
                                     static_cast<ar::RadioIndicationType>(type),
                                     static_cast<ar::RadioTechnology>(rat)));
    }
    return Void();
}

Return<void> IndicationHandler::imsNetworkStateChanged(R10::RadioIndicationType type) {
    if (mCtx->networkIndication) {
        callStatus(__func__, mCtx->networkIndication->imsNetworkStateChanged(
                                     static_cast<ar::RadioIndicationType>(type)));
    }
    return Void();
}

Return<void> IndicationHandler::subscriptionStatusChanged(R10::RadioIndicationType type,
                                                          bool activate) {
    if (mCtx->simIndication) {
        callStatus(__func__, mCtx->simIndication->subscriptionStatusChanged(
                                     static_cast<ar::RadioIndicationType>(type), activate));
    }
    return Void();
}

Return<void> IndicationHandler::srvccStateNotify(R10::RadioIndicationType type,
                                                 R10::SrvccState state) {
    if (mCtx->voiceIndication) {
        callStatus(__func__, mCtx->voiceIndication->srvccStateNotify(
                                     static_cast<ar::RadioIndicationType>(type),
                                     static_cast<ar::voice::SrvccState>(state)));
    }
    return Void();
}

Return<void> IndicationHandler::hardwareConfigChanged(
        R10::RadioIndicationType type, const hidl_vec<R10::HardwareConfig>& configs) {
    if (const auto indication = mCtx->getModemIndication()) {
        std::vector<ar::modem::HardwareConfig> out;
        for (const auto& hw : configs) {
            ar::modem::HardwareConfig cfg;
            cfg.type = static_cast<int32_t>(hw.type);
            cfg.uuid = hw.uuid;
            cfg.state = static_cast<int32_t>(hw.state);
            if (hw.modem.size() != 0) {
                ar::modem::HardwareConfigModem modem;
                modem.rilModel = hw.modem[0].rilModel;
                modem.rat = static_cast<ar::RadioTechnology>(hw.modem[0].rat);
                modem.maxVoiceCalls = hw.modem[0].maxVoice;
                modem.maxDataCalls = hw.modem[0].maxData;
                modem.maxStandby = hw.modem[0].maxStandby;
                cfg.modem.push_back(modem);
            }
            ar::modem::HardwareConfigSim sim;
            sim.modemUuid = hw.uuid;
            cfg.sim.push_back(sim);
            out.push_back(cfg);
        }
        callStatus(__func__, indication->hardwareConfigChanged(
                                     static_cast<ar::RadioIndicationType>(type), out));
    }
    return Void();
}

Return<void> IndicationHandler::radioCapabilityIndication(R10::RadioIndicationType type,
                                                          const R10::RadioCapability& rc) {
    if (const auto indication = mCtx->getModemIndication()) {
        callStatus(__func__, indication->radioCapabilityIndication(
                                     static_cast<ar::RadioIndicationType>(type), toAidl(rc)));
    }
    return Void();
}

Return<void> IndicationHandler::pcoData(R10::RadioIndicationType type,
                                        const R10::PcoDataInfo& pco) {
    if (mCtx->dataIndication) {
        ar::data::PcoDataInfo out;
        out.cid = pco.cid;
        out.bearerProto = "";
        out.pcoId = 0;
        out.contents.assign(pco.contents.begin(), pco.contents.end());
        callStatus(__func__, mCtx->dataIndication->pcoData(
                                     static_cast<ar::RadioIndicationType>(type), out));
    }
    return Void();
}

Return<void> IndicationHandler::modemReset(R10::RadioIndicationType type,
                                           const hidl_string& reason) {
    if (const auto indication = mCtx->getModemIndication()) {
        callStatus(__func__, indication->modemReset(
                                     static_cast<ar::RadioIndicationType>(type), reason));
    }
    return Void();
}

// ---------------------------------------------------------------------------
// Unsupported / CDMA-only indications.
// ---------------------------------------------------------------------------

Return<void> IndicationHandler::cdmaNewSms(R10::RadioIndicationType type,
                                           const R10::CdmaSmsMessage&) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::newBroadcastSms(R10::RadioIndicationType type,
                                                const hidl_vec<uint8_t>&) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::cdmaRuimSmsStorageFull(R10::RadioIndicationType type) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::enterEmergencyCallbackMode(R10::RadioIndicationType type) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::cdmaCallWaiting(R10::RadioIndicationType type,
                                                const R10::CdmaCallWaiting&) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::cdmaOtaProvisionStatus(R10::RadioIndicationType type,
                                                       R10::CdmaOtaProvisionStatus) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::cdmaInfoRec(R10::RadioIndicationType type,
                                            const R10::CdmaInformationRecords&) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::cdmaSubscriptionSourceChanged(R10::RadioIndicationType type,
                                                              R10::CdmaSubscriptionSource) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::cdmaPrlChanged(R10::RadioIndicationType type, int32_t) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::cellInfoList(R10::RadioIndicationType type,
                                             const hidl_vec<R10::CellInfo>&) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::onSupplementaryServiceIndication(
        R10::RadioIndicationType type, const R10::StkCcUnsolSsResult&) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::stkCallControlAlphaNotify(R10::RadioIndicationType type,
                                                          const hidl_string&) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::lceData(R10::RadioIndicationType type, const R10::LceDataInfo&) {
    drop(__func__, type);
    return Void();
}

// ---------------------------------------------------------------------------
// V1.1 indications.
// ---------------------------------------------------------------------------

Return<void> IndicationHandler::carrierInfoForImsiEncryption(R10::RadioIndicationType type) {
    if (mCtx->simIndication) {
        callStatus(__func__, mCtx->simIndication->carrierInfoForImsiEncryption(
                                     static_cast<ar::RadioIndicationType>(type)));
    }
    return Void();
}

Return<void> IndicationHandler::networkScanResult(R10::RadioIndicationType type,
                                                  const R11::NetworkScanResult&) {
    drop(__func__, type);
    return Void();
}

Return<void> IndicationHandler::keepaliveStatus(R10::RadioIndicationType type,
                                                const R11::KeepaliveStatus&) {
    drop(__func__, type);
    return Void();
}

}  // namespace android::hardware::radio::shim
