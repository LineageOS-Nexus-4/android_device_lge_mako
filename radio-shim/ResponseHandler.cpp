#include "ResponseHandler.h"
#include "Structs.h"

#include <aidl/android/hardware/radio/config/SimPortInfo.h>
#include <aidl/android/hardware/radio/config/SimSlotStatus.h>
#include <aidl/android/hardware/radio/messaging/GsmBroadcastSmsConfigInfo.h>
#include <aidl/android/hardware/radio/modem/HardwareConfig.h>
#include <aidl/android/hardware/radio/modem/HardwareConfigModem.h>
#include <aidl/android/hardware/radio/modem/HardwareConfigSim.h>
#include <aidl/android/hardware/radio/modem/ImeiInfo.h>
#include <aidl/android/hardware/radio/network/OperatorInfo.h>
#include <aidl/android/hardware/radio/voice/CallForwardInfo.h>
#include <aidl/android/hardware/radio/voice/LastCallFailCauseInfo.h>
#include <aidl/android/hardware/radio/voice/UusInfo.h>

#include <android-base/logging.h>

#include <vector>

namespace android::hardware::radio::shim {

namespace ar = ::aidl::android::hardware::radio;

void ResponseHandler::callStatus(const char* method, const ::ndk::ScopedAStatus& status) {
    if (!status.isOk()) {
        LOG(WARNING) << "AIDL callback " << method << " failed: " << status.getDescription();
    }
}

static int toByte(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return 0;
}

static std::string bcdToString(const std::string& hex) {
    std::string out;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        const int b = (toByte(hex[i]) << 4) | toByte(hex[i + 1]);
        int d = b & 0xF;
        if (d > 9) break;
        out += static_cast<char>('0' + d);
        d = (b >> 4) & 0xF;
        if (d > 9) break;
        out += static_cast<char>('0' + d);
    }
    return out;
}

void ResponseHandler::drop(const char* method, const R10::RadioResponseInfo& info,
                           RequestKind kind) {
    LOG(WARNING) << method << ": serial " << info.serial << " kind " << static_cast<int>(kind)
                 << " dropped (duplicate or unexpected response)";
}

Return<void> ResponseHandler::acknowledgeRequest(int32_t) {
    return Void();
}

// ---------------------------------------------------------------------------
// modem
// ---------------------------------------------------------------------------

Return<void> ResponseHandler::setRadioPowerResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MODEM_SET_RADIO_POWER) return drop(__func__, info, kind), Void();
    if (mCtx->modemResponse) {
        callStatus(__func__, mCtx->modemResponse->setRadioPowerResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::getDeviceIdentityResponse(const R10::RadioResponseInfo& info,
                                                        const hidl_string& imei,
                                                        const hidl_string& imeisv,
                                                        const hidl_string& svn,
                                                        const hidl_string& spn) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind == RequestKind::MODEM_GET_IMEI) {
        if (mCtx->modemResponse) {
            ar::modem::ImeiInfo imeiInfo;
            imeiInfo.type = ar::modem::ImeiInfo::ImeiType::PRIMARY;
            imeiInfo.imei = imei;
            imeiInfo.svn = imeisv;
            callStatus(__func__,
                       mCtx->modemResponse->getImeiResponse(toAidl(info), imeiInfo));
        }
    } else if (kind == RequestKind::MODEM_GET_DEVICE_IDENTITY) {
        if (mCtx->modemResponse) {
            callStatus(__func__, mCtx->modemResponse->getDeviceIdentityResponse(
                                         toAidl(info), imei, imeisv, svn, spn));
        }
    } else {
        drop(__func__, info, kind);
    }
    return Void();
}

Return<void> ResponseHandler::getBasebandVersionResponse(const R10::RadioResponseInfo& info,
                                                         const hidl_string& version) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MODEM_GET_BASEBAND_VERSION) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->modemResponse) {
        callStatus(__func__, mCtx->modemResponse->getBasebandVersionResponse(toAidl(info), version));
    }
    return Void();
}

Return<void> ResponseHandler::getRadioCapabilityResponse(const R10::RadioResponseInfo& info,
                                                         const R10::RadioCapability& rc) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MODEM_GET_RADIO_CAPABILITY) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->modemResponse) {
        callStatus(__func__,
                   mCtx->modemResponse->getRadioCapabilityResponse(toAidl(info), toAidl(rc)));
    }
    return Void();
}

Return<void> ResponseHandler::setRadioCapabilityResponse(const R10::RadioResponseInfo& info,
                                                         const R10::RadioCapability& rc) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MODEM_SET_RADIO_CAPABILITY) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->modemResponse) {
        callStatus(__func__,
                   mCtx->modemResponse->setRadioCapabilityResponse(toAidl(info), toAidl(rc)));
    }
    return Void();
}

Return<void> ResponseHandler::getHardwareConfigResponse(
        const R10::RadioResponseInfo& info, const hidl_vec<R10::HardwareConfig>& config) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MODEM_GET_HARDWARE_CONFIG) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->modemResponse) {
        std::vector<ar::modem::HardwareConfig> out;
        for (const auto& hw : config) {
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
        callStatus(__func__, mCtx->modemResponse->getHardwareConfigResponse(toAidl(info), out));
    }
    return Void();
}

Return<void> ResponseHandler::requestShutdownResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MODEM_REQUEST_SHUTDOWN) return drop(__func__, info, kind), Void();
    if (mCtx->modemResponse) {
        callStatus(__func__, mCtx->modemResponse->requestShutdownResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::sendDeviceStateResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MODEM_SEND_DEVICE_STATE) return drop(__func__, info, kind), Void();
    if (mCtx->modemResponse) {
        callStatus(__func__, mCtx->modemResponse->sendDeviceStateResponse(toAidl(info)));
    }
    return Void();
}

// ---------------------------------------------------------------------------
// sim
// ---------------------------------------------------------------------------

Return<void> ResponseHandler::getIccCardStatusResponse(const R10::RadioResponseInfo& info,
                                                       const R10::CardStatus& cardStatus) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind == RequestKind::SIM_GET_ICC_CARD_STATUS) {
        if (mCtx->simResponse) {
            callStatus(__func__, mCtx->simResponse->getIccCardStatusResponse(
                                         toAidl(info), toAidl(cardStatus, mCtx->getIccId())));
        }
        // The V1.0 CardStatus has no iccid; fill it from EF-ICCID so the
        // framework can match the subscription to the slot.
        if (mCtx->getIccId().empty() &&
            cardStatus.cardState == R10::CardState::PRESENT) {
            mCtx->refreshIccId();
        }
    } else if (kind == RequestKind::CONFIG_GET_SIM_SLOTS_STATUS) {
        if (mCtx->configResponse) {
            std::vector<ar::config::SimSlotStatus> slots;
            ar::config::SimSlotStatus slot;
            slot.cardState = static_cast<int32_t>(cardStatus.cardState);
            slot.atr = "";
            slot.eid = "";
            ar::config::SimPortInfo port;
            port.iccId = mCtx->getIccId();
            port.logicalSlotId = 0;
            port.portActive = true;
            slot.portInfo.push_back(port);
            slot.supportedMepMode = ar::config::MultipleEnabledProfilesMode::NONE;
            slots.push_back(slot);
            callStatus(__func__,
                       mCtx->configResponse->getSimSlotsStatusResponse(toAidl(info), slots));
        }
    } else {
        drop(__func__, info, kind);
    }
    return Void();
}

Return<void> ResponseHandler::iccIOForAppResponse(const R10::RadioResponseInfo& info,
                                                  const R10::IccIoResult& iccIo) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind == RequestKind::SIM_INTERNAL_READ_ICCID) {
        // GET RESPONSE: the EF size is bytes [2..3] of the response.
        const std::string hex = iccIo.simResponse;
        if (hex.size() >= 8 && iccIo.sw1 == 0x90 && iccIo.sw2 == 0x00) {
            const int size = (toByte(hex[4]) << 4 | toByte(hex[5])) << 8 |
                             (toByte(hex[6]) << 4 | toByte(hex[7]));
            if (size > 0) {
                const int32_t serial = mCtx->nextInternalSerial();
                mCtx->registerRequest(serial, RequestKind::SIM_INTERNAL_READ_ICCID_DATA);
                R10::IccIo io;
                io.command = 0xB0;  // READ BINARY
                io.fileId = 0x2FE2;
                io.path = "3F00";
                io.p1 = 0;
                io.p2 = 0;
                io.p3 = size;
                io.data = "";
                io.pin2 = "";
                io.aid = "";
                const auto status = mCtx->radio->iccIOForApp(serial, io);
                if (!status.isOk()) {
                    LOG(ERROR) << "internal EF-ICCID READ failed";
                    mCtx->dropRequest(serial);
                }
            }
        } else {
            LOG(WARNING) << "internal EF-ICCID GET_RESPONSE failed: " << hex;
            mCtx->scheduleIccIdRetry();
        }
        return Void();
    }
    if (kind == RequestKind::SIM_INTERNAL_READ_ICCID_DATA) {
        if (iccIo.sw1 == 0x90 && iccIo.sw2 == 0x00 && !iccIo.simResponse.empty()) {
            mCtx->setIccId(bcdToString(iccIo.simResponse));
            LOG(INFO) << "EF-ICCID read: " << mCtx->getIccId();
            // Nudge the framework to re-poll getIccCardStatus so the freshly
            // read ICCID reaches UiccSlot and the subscription mapping.
            if (mCtx->simIndication) {
                mCtx->simIndication->simStatusChanged(
                        ar::RadioIndicationType::UNSOLICITED);
            }
        } else {
            LOG(WARNING) << "internal EF-ICCID READ failed: sw=" << iccIo.sw1 << "/"
                         << iccIo.sw2 << " len=" << iccIo.simResponse.size();
            mCtx->scheduleIccIdRetry();
        }
        return Void();
    }
    if (kind != RequestKind::SIM_ICC_IO_FOR_APP) return drop(__func__, info, kind), Void();
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->iccIoForAppResponse(toAidl(info), toAidl(iccIo)));
    }
    return Void();
}

Return<void> ResponseHandler::getIMSIForAppResponse(const R10::RadioResponseInfo& info,
                                                    const hidl_string& imsi) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_GET_IMSI_FOR_APP) return drop(__func__, info, kind), Void();
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->getImsiForAppResponse(toAidl(info), imsi));
    }
    return Void();
}

Return<void> ResponseHandler::supplyIccPinForAppResponse(const R10::RadioResponseInfo& info,
                                                         int32_t remainingRetries) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_SUPPLY_ICC_PIN_FOR_APP) return drop(__func__, info, kind), Void();
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->supplyIccPinForAppResponse(toAidl(info),
                                                                           remainingRetries));
    }
    return Void();
}

Return<void> ResponseHandler::supplyIccPukForAppResponse(const R10::RadioResponseInfo& info,
                                                         int32_t remainingRetries) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_SUPPLY_ICC_PUK_FOR_APP) return drop(__func__, info, kind), Void();
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->supplyIccPukForAppResponse(toAidl(info),
                                                                           remainingRetries));
    }
    return Void();
}

Return<void> ResponseHandler::supplyIccPin2ForAppResponse(const R10::RadioResponseInfo& info,
                                                          int32_t remainingRetries) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_SUPPLY_ICC_PIN2_FOR_APP) return drop(__func__, info, kind), Void();
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->supplyIccPin2ForAppResponse(toAidl(info),
                                                                            remainingRetries));
    }
    return Void();
}

Return<void> ResponseHandler::supplyIccPuk2ForAppResponse(const R10::RadioResponseInfo& info,
                                                          int32_t remainingRetries) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_SUPPLY_ICC_PUK2_FOR_APP) return drop(__func__, info, kind), Void();
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->supplyIccPuk2ForAppResponse(toAidl(info),
                                                                            remainingRetries));
    }
    return Void();
}

Return<void> ResponseHandler::changeIccPinForAppResponse(const R10::RadioResponseInfo& info,
                                                         int32_t remainingRetries) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_CHANGE_ICC_PIN_FOR_APP) return drop(__func__, info, kind), Void();
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->changeIccPinForAppResponse(toAidl(info),
                                                                           remainingRetries));
    }
    return Void();
}

Return<void> ResponseHandler::changeIccPin2ForAppResponse(const R10::RadioResponseInfo& info,
                                                          int32_t remainingRetries) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_CHANGE_ICC_PIN2_FOR_APP) return drop(__func__, info, kind), Void();
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->changeIccPin2ForAppResponse(toAidl(info),
                                                                            remainingRetries));
    }
    return Void();
}

Return<void> ResponseHandler::supplyNetworkDepersonalizationResponse(
        const R10::RadioResponseInfo& info, int32_t remainingRetries) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NONE) {
        // Never forwarded; keep for completeness.
    }
    drop(__func__, info, kind);
    return Void();
}

Return<void> ResponseHandler::getFacilityLockForAppResponse(const R10::RadioResponseInfo& info,
                                                            int32_t response) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_GET_FACILITY_LOCK_FOR_APP) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->simResponse) {
        callStatus(__func__,
                   mCtx->simResponse->getFacilityLockForAppResponse(toAidl(info), response));
    }
    return Void();
}

Return<void> ResponseHandler::setFacilityLockForAppResponse(const R10::RadioResponseInfo& info,
                                                            int32_t retry) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_SET_FACILITY_LOCK_FOR_APP) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->simResponse) {
        callStatus(__func__,
                   mCtx->simResponse->setFacilityLockForAppResponse(toAidl(info), retry));
    }
    return Void();
}

Return<void> ResponseHandler::setSimCardPowerResponse(const R10::RadioResponseInfo& info) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::setSimCardPowerResponse_1_1(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_SET_SIM_CARD_POWER) return drop(__func__, info, kind), Void();
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->setSimCardPowerResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::reportStkServiceIsRunningResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_REPORT_STK_SERVICE_IS_RUNNING) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->reportStkServiceIsRunningResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::sendEnvelopeResponse(const R10::RadioResponseInfo& info,
                                                   const hidl_string& commandResponse) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_SEND_ENVELOPE) return drop(__func__, info, kind), Void();
    if (mCtx->simResponse) {
        callStatus(__func__,
                   mCtx->simResponse->sendEnvelopeResponse(toAidl(info), commandResponse));
    }
    return Void();
}

Return<void> ResponseHandler::sendEnvelopeWithStatusResponse(const R10::RadioResponseInfo& info,
                                                             const R10::IccIoResult& iccIo) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_SEND_ENVELOPE_WITH_STATUS) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->simResponse) {
        callStatus(__func__,
                   mCtx->simResponse->sendEnvelopeWithStatusResponse(toAidl(info), toAidl(iccIo)));
    }
    return Void();
}

Return<void> ResponseHandler::sendTerminalResponseToSimResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_SEND_TERMINAL_RESPONSE_TO_SIM) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->sendTerminalResponseToSimResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::requestIccSimAuthenticationResponse(
        const R10::RadioResponseInfo& info, const R10::IccIoResult& result) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_REQUEST_ICC_SIM_AUTHENTICATION) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->requestIccSimAuthenticationResponse(toAidl(info),
                                                                                    toAidl(result)));
    }
    return Void();
}

Return<void> ResponseHandler::iccTransmitApduBasicChannelResponse(
        const R10::RadioResponseInfo& info, const R10::IccIoResult& result) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_ICC_TRANSMIT_APDU_BASIC_CHANNEL) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->iccTransmitApduBasicChannelResponse(toAidl(info),
                                                                                    toAidl(result)));
    }
    return Void();
}

Return<void> ResponseHandler::iccOpenLogicalChannelResponse(
        const R10::RadioResponseInfo& info, int32_t channelId,
        const hidl_vec<int8_t>& selectResponse) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_ICC_OPEN_LOGICAL_CHANNEL) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->simResponse) {
        std::vector<uint8_t> sel(selectResponse.begin(), selectResponse.end());
        callStatus(__func__, mCtx->simResponse->iccOpenLogicalChannelResponse(toAidl(info),
                                                                              channelId, sel));
    }
    return Void();
}

Return<void> ResponseHandler::iccCloseLogicalChannelResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_ICC_CLOSE_LOGICAL_CHANNEL) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->iccCloseLogicalChannelResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::iccTransmitApduLogicalChannelResponse(
        const R10::RadioResponseInfo& info, const R10::IccIoResult& result) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_ICC_TRANSMIT_APDU_LOGICAL_CHANNEL) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->iccTransmitApduLogicalChannelResponse(
                                     toAidl(info), toAidl(result)));
    }
    return Void();
}

Return<void> ResponseHandler::setCarrierInfoForImsiEncryptionResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::SIM_SET_CARRIER_INFO_FOR_IMSI_ENCRYPTION) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->simResponse) {
        callStatus(__func__, mCtx->simResponse->setCarrierInfoForImsiEncryptionResponse(
                                     toAidl(info)));
    }
    return Void();
}

// ---------------------------------------------------------------------------
// network
// ---------------------------------------------------------------------------

Return<void> ResponseHandler::getSignalStrengthResponse(const R10::RadioResponseInfo& info,
                                                        const R10::SignalStrength& sigStrength) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_GET_SIGNAL_STRENGTH) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        callStatus(__func__, mCtx->networkResponse->getSignalStrengthResponse(
                                     toAidl(info), toAidl(sigStrength)));
    }
    return Void();
}

Return<void> ResponseHandler::getVoiceRegistrationStateResponse(
        const R10::RadioResponseInfo& info, const R10::VoiceRegStateResult& voiceRegResponse) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_GET_VOICE_REGISTRATION_STATE) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        callStatus(__func__, mCtx->networkResponse->getVoiceRegistrationStateResponse(
                                     toAidl(info), toAidl(voiceRegResponse)));
    }
    return Void();
}

Return<void> ResponseHandler::getDataRegistrationStateResponse(
        const R10::RadioResponseInfo& info, const R10::DataRegStateResult& dataRegResponse) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_GET_DATA_REGISTRATION_STATE) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        callStatus(__func__, mCtx->networkResponse->getDataRegistrationStateResponse(
                                     toAidl(info), toAidl(dataRegResponse)));
    }
    return Void();
}

Return<void> ResponseHandler::getOperatorResponse(const R10::RadioResponseInfo& info,
                                                  const hidl_string& longName,
                                                  const hidl_string& shortName,
                                                  const hidl_string& numeric) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_GET_OPERATOR) return drop(__func__, info, kind), Void();
    if (mCtx->networkResponse) {
        callStatus(__func__, mCtx->networkResponse->getOperatorResponse(toAidl(info), longName,
                                                                        shortName, numeric));
    }
    return Void();
}

Return<void> ResponseHandler::getNetworkSelectionModeResponse(const R10::RadioResponseInfo& info,
                                                              bool manual) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_GET_NETWORK_SELECTION_MODE) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        callStatus(__func__, mCtx->networkResponse->getNetworkSelectionModeResponse(toAidl(info),
                                                                                    manual));
    }
    return Void();
}

Return<void> ResponseHandler::setNetworkSelectionModeAutomaticResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_SET_NETWORK_SELECTION_MODE_AUTOMATIC) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        callStatus(__func__, mCtx->networkResponse->setNetworkSelectionModeAutomaticResponse(
                                     toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::setNetworkSelectionModeManualResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_SET_NETWORK_SELECTION_MODE_MANUAL) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        callStatus(__func__, mCtx->networkResponse->setNetworkSelectionModeManualResponse(
                                     toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::getAvailableNetworksResponse(
        const R10::RadioResponseInfo& info, const hidl_vec<R10::OperatorInfo>& networkInfos) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_GET_AVAILABLE_NETWORKS) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        std::vector<ar::network::OperatorInfo> out;
        for (const auto& op : networkInfos) out.push_back(toAidl(op));
        callStatus(__func__,
                   mCtx->networkResponse->getAvailableNetworksResponse(toAidl(info), out));
    }
    return Void();
}

Return<void> ResponseHandler::getVoiceRadioTechnologyResponse(const R10::RadioResponseInfo& info,
                                                              R10::RadioTechnology rat) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_GET_VOICE_RADIO_TECHNOLOGY) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        callStatus(__func__, mCtx->networkResponse->getVoiceRadioTechnologyResponse(
                                     toAidl(info), static_cast<ar::RadioTechnology>(rat)));
    }
    return Void();
}

Return<void> ResponseHandler::getPreferredNetworkTypeResponse(
        const R10::RadioResponseInfo& info, R10::PreferredNetworkType networkType) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_GET_PREFERRED_NETWORK_TYPE) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        callStatus(__func__, mCtx->networkResponse->getAllowedNetworkTypesBitmapResponse(
                                     toAidl(info),
                                     toAidlAllowedNetworkTypesBitmap(
                                             static_cast<int32_t>(networkType))));
    }
    return Void();
}

Return<void> ResponseHandler::setPreferredNetworkTypeResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_SET_PREFERRED_NETWORK_TYPE) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        callStatus(__func__, mCtx->networkResponse->setAllowedNetworkTypesBitmapResponse(
                                     toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::setIndicationFilterResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_SET_INDICATION_FILTER) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        callStatus(__func__, mCtx->networkResponse->setIndicationFilterResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::setLocationUpdatesResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_SET_LOCATION_UPDATES) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        callStatus(__func__, mCtx->networkResponse->setLocationUpdatesResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::setSuppServiceNotificationsResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::NET_SET_SUPP_SERVICE_NOTIFICATIONS) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->networkResponse) {
        callStatus(__func__,
                   mCtx->networkResponse->setSuppServiceNotificationsResponse(toAidl(info)));
    }
    return Void();
}

// ---------------------------------------------------------------------------
// voice
// ---------------------------------------------------------------------------

Return<void> ResponseHandler::getCurrentCallsResponse(const R10::RadioResponseInfo& info,
                                                      const hidl_vec<R10::Call>& calls) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_GET_CURRENT_CALLS) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        std::vector<ar::voice::Call> out;
        for (const auto& c : calls) out.push_back(toAidl(c));
        callStatus(__func__, mCtx->voiceResponse->getCurrentCallsResponse(toAidl(info), out));
    }
    return Void();
}

Return<void> ResponseHandler::dialResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_DIAL) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->dialResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::acceptCallResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_ACCEPT_CALL) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->acceptCallResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::rejectCallResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_REJECT_CALL) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->rejectCallResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::hangupConnectionResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_HANGUP) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->hangupConnectionResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::hangupWaitingOrBackgroundResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_HANGUP_WAITING_OR_BACKGROUND) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->hangupWaitingOrBackgroundResponse(
                                     toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::hangupForegroundResumeBackgroundResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_HANGUP_FOREGROUND_RESUME_BACKGROUND) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->hangupForegroundResumeBackgroundResponse(
                                     toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::switchWaitingOrHoldingAndActiveResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->switchWaitingOrHoldingAndActiveResponse(
                                     toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::conferenceResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_CONFERENCE) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->conferenceResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::separateConnectionResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_SEPARATE_CONNECTION) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->separateConnectionResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::getLastCallFailCauseResponse(
        const R10::RadioResponseInfo& info, const R10::LastCallFailCauseInfo& failCauseinfo) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_GET_LAST_CALL_FAIL_CAUSE) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->voiceResponse) {
        ar::voice::LastCallFailCauseInfo out;
        out.causeCode = static_cast<ar::voice::LastCallFailCause>(failCauseinfo.causeCode);
        out.vendorCause = failCauseinfo.vendorCause;
        callStatus(__func__, mCtx->voiceResponse->getLastCallFailCauseResponse(toAidl(info), out));
    }
    return Void();
}

Return<void> ResponseHandler::sendDtmfResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_SEND_DTMF) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->sendDtmfResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::startDtmfResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_START_DTMF) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->startDtmfResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::stopDtmfResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_STOP_DTMF) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->stopDtmfResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::sendUssdResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_SEND_USSD) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->sendUssdResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::cancelPendingUssdResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_CANCEL_PENDING_USSD) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->cancelPendingUssdResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::getClirResponse(const R10::RadioResponseInfo& info, int32_t n,
                                              int32_t m) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_GET_CLIR) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->getClirResponse(toAidl(info), n, m));
    }
    return Void();
}

Return<void> ResponseHandler::setClirResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_SET_CLIR) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->setClirResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::getClipResponse(const R10::RadioResponseInfo& info,
                                              R10::ClipStatus status) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_GET_CLIP) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->getClipResponse(
                                     toAidl(info), static_cast<ar::voice::ClipStatus>(status)));
    }
    return Void();
}

Return<void> ResponseHandler::getCallForwardStatusResponse(
        const R10::RadioResponseInfo& info, const hidl_vec<R10::CallForwardInfo>& callForwardInfos) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_GET_CALL_FORWARD_STATUS) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->voiceResponse) {
        std::vector<ar::voice::CallForwardInfo> out;
        for (const auto& cfi : callForwardInfos) {
            ar::voice::CallForwardInfo infoOut;
            infoOut.status = static_cast<int32_t>(cfi.status);
            infoOut.reason = cfi.reason;
            infoOut.serviceClass = cfi.serviceClass;
            infoOut.toa = cfi.toa;
            infoOut.number = cfi.number;
            infoOut.timeSeconds = cfi.timeSeconds;
            out.push_back(infoOut);
        }
        callStatus(__func__,
                   mCtx->voiceResponse->getCallForwardStatusResponse(toAidl(info), out));
    }
    return Void();
}

Return<void> ResponseHandler::setCallForwardResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_SET_CALL_FORWARD) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->setCallForwardResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::getCallWaitingResponse(const R10::RadioResponseInfo& info,
                                                     bool enable, int32_t serviceClass) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_GET_CALL_WAITING) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__,
                   mCtx->voiceResponse->getCallWaitingResponse(toAidl(info), enable, serviceClass));
    }
    return Void();
}

Return<void> ResponseHandler::setCallWaitingResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_SET_CALL_WAITING) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->setCallWaitingResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::getMuteResponse(const R10::RadioResponseInfo& info, bool enable) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_GET_MUTE) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->getMuteResponse(toAidl(info), enable));
    }
    return Void();
}

Return<void> ResponseHandler::setMuteResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_SET_MUTE) return drop(__func__, info, kind), Void();
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->setMuteResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::explicitCallTransferResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_EXPLICIT_CALL_TRANSFER) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->explicitCallTransferResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::handleStkCallSetupRequestFromSimResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::VOICE_HANDLE_STK_CALL_SETUP_REQUEST_FROM_SIM) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->voiceResponse) {
        callStatus(__func__, mCtx->voiceResponse->handleStkCallSetupRequestFromSimResponse(
                                     toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::setTTYModeResponse(const R10::RadioResponseInfo& info) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::getTTYModeResponse(const R10::RadioResponseInfo& info,
                                                 R10::TtyMode mode) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

// ---------------------------------------------------------------------------
// messaging
// ---------------------------------------------------------------------------

Return<void> ResponseHandler::sendSmsResponse(const R10::RadioResponseInfo& info,
                                              const R10::SendSmsResult& sms) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MSG_SEND_SMS) return drop(__func__, info, kind), Void();
    if (mCtx->messagingResponse) {
        callStatus(__func__, mCtx->messagingResponse->sendSmsResponse(toAidl(info), toAidl(sms)));
    }
    return Void();
}

Return<void> ResponseHandler::sendSMSExpectMoreResponse(const R10::RadioResponseInfo& info,
                                                        const R10::SendSmsResult& sms) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MSG_SEND_SMS_EXPECT_MORE) return drop(__func__, info, kind), Void();
    if (mCtx->messagingResponse) {
        callStatus(__func__, mCtx->messagingResponse->sendSmsExpectMoreResponse(toAidl(info),
                                                                                toAidl(sms)));
    }
    return Void();
}

Return<void> ResponseHandler::acknowledgeLastIncomingGsmSmsResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MSG_ACK_LAST_INCOMING_GSM_SMS) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->messagingResponse) {
        callStatus(__func__,
                   mCtx->messagingResponse->acknowledgeLastIncomingGsmSmsResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::acknowledgeIncomingGsmSmsWithPduResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MSG_ACK_INCOMING_GSM_SMS_WITH_PDU) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->messagingResponse) {
        callStatus(__func__, mCtx->messagingResponse->acknowledgeIncomingGsmSmsWithPduResponse(
                                     toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::getSmscAddressResponse(const R10::RadioResponseInfo& info,
                                                     const hidl_string& smsc) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MSG_GET_SMSC_ADDRESS) return drop(__func__, info, kind), Void();
    if (mCtx->messagingResponse) {
        callStatus(__func__,
                   mCtx->messagingResponse->getSmscAddressResponse(toAidl(info), smsc));
    }
    return Void();
}

Return<void> ResponseHandler::setSmscAddressResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MSG_SET_SMSC_ADDRESS) return drop(__func__, info, kind), Void();
    if (mCtx->messagingResponse) {
        callStatus(__func__, mCtx->messagingResponse->setSmscAddressResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::writeSmsToSimResponse(const R10::RadioResponseInfo& info,
                                                    int32_t index) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MSG_WRITE_SMS_TO_SIM) return drop(__func__, info, kind), Void();
    if (mCtx->messagingResponse) {
        callStatus(__func__,
                   mCtx->messagingResponse->writeSmsToSimResponse(toAidl(info), index));
    }
    return Void();
}

Return<void> ResponseHandler::deleteSmsOnSimResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MSG_DELETE_SMS_ON_SIM) return drop(__func__, info, kind), Void();
    if (mCtx->messagingResponse) {
        callStatus(__func__, mCtx->messagingResponse->deleteSmsOnSimResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::getGsmBroadcastConfigResponse(
        const R10::RadioResponseInfo& info,
        const hidl_vec<R10::GsmBroadcastSmsConfigInfo>& configs) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MSG_GET_GSM_BROADCAST_CONFIG) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->messagingResponse) {
        std::vector<ar::messaging::GsmBroadcastSmsConfigInfo> out;
        for (const auto& c : configs) {
            ar::messaging::GsmBroadcastSmsConfigInfo cfg;
            cfg.fromServiceId = c.fromServiceId;
            cfg.toServiceId = c.toServiceId;
            cfg.fromCodeScheme = c.fromCodeScheme;
            cfg.toCodeScheme = c.toCodeScheme;
            cfg.selected = c.selected;
            out.push_back(cfg);
        }
        callStatus(__func__,
                   mCtx->messagingResponse->getGsmBroadcastConfigResponse(toAidl(info), out));
    }
    return Void();
}

Return<void> ResponseHandler::setGsmBroadcastConfigResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MSG_SET_GSM_BROADCAST_CONFIG) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->messagingResponse) {
        callStatus(__func__, mCtx->messagingResponse->setGsmBroadcastConfigResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::setGsmBroadcastActivationResponse(
        const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MSG_SET_GSM_BROADCAST_ACTIVATION) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->messagingResponse) {
        callStatus(__func__, mCtx->messagingResponse->setGsmBroadcastActivationResponse(
                                     toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::reportSmsMemoryStatusResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::MSG_REPORT_SMS_MEMORY_STATUS) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->messagingResponse) {
        callStatus(__func__, mCtx->messagingResponse->reportSmsMemoryStatusResponse(toAidl(info)));
    }
    return Void();
}

// ---------------------------------------------------------------------------
// data
// ---------------------------------------------------------------------------

Return<void> ResponseHandler::setupDataCallResponse(const R10::RadioResponseInfo& info,
                                                    const R10::SetupDataCallResult& dcResponse) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::DATA_SETUP_DATA_CALL) return drop(__func__, info, kind), Void();
    const auto aidlResult = toAidl(dcResponse);
    LOG(INFO) << "setupDataCallResponse serial=" << info.serial
              << " error=" << static_cast<int32_t>(info.error)
              << " status=" << static_cast<int32_t>(dcResponse.status)
              << " cid=" << dcResponse.cid << " active=" << dcResponse.active
              << " ifname=" << dcResponse.ifname << " type=" << dcResponse.type;
    if (mCtx->dataResponse) {
        callStatus(__func__, mCtx->dataResponse->setupDataCallResponse(toAidl(info), aidlResult));
    }
    // The mako kernel rejects netd's default-route add while the interface
    // address is still coming up; add the routes from here (NET_ADMIN).
    if (info.error == R10::RadioError::NONE &&
        static_cast<int32_t>(dcResponse.status) == static_cast<int32_t>(R10::DataCallFailCause::NONE) &&
        !dcResponse.ifname.empty() && !dcResponse.gateways.empty()) {
        mCtx->addDataRoutes(std::string(dcResponse.gateways.c_str()), std::string(dcResponse.ifname.c_str()));
    }
    return Void();
}

Return<void> ResponseHandler::deactivateDataCallResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::DATA_DEACTIVATE_DATA_CALL) return drop(__func__, info, kind), Void();
    if (mCtx->dataResponse) {
        callStatus(__func__, mCtx->dataResponse->deactivateDataCallResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::getDataCallListResponse(
        const R10::RadioResponseInfo& info, const hidl_vec<R10::SetupDataCallResult>& dcResponse) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::DATA_GET_DATA_CALL_LIST) return drop(__func__, info, kind), Void();
    if (mCtx->dataResponse) {
        std::vector<ar::data::SetupDataCallResult> out;
        for (const auto& r : dcResponse) out.push_back(toAidl(r));
        callStatus(__func__, mCtx->dataResponse->getDataCallListResponse(toAidl(info), out));
    }
    return Void();
}

Return<void> ResponseHandler::setInitialAttachApnResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::DATA_SET_INITIAL_ATTACH_APN) {
        return drop(__func__, info, kind), Void();
    }
    if (mCtx->dataResponse) {
        callStatus(__func__, mCtx->dataResponse->setInitialAttachApnResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::setDataAllowedResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::DATA_SET_DATA_ALLOWED) return drop(__func__, info, kind), Void();
    if (mCtx->dataResponse) {
        callStatus(__func__, mCtx->dataResponse->setDataAllowedResponse(toAidl(info)));
    }
    return Void();
}

Return<void> ResponseHandler::setDataProfileResponse(const R10::RadioResponseInfo& info) {
    const auto kind = mCtx->popRequest(info.serial);
    if (kind != RequestKind::DATA_SET_DATA_PROFILE) return drop(__func__, info, kind), Void();
    if (mCtx->dataResponse) {
        callStatus(__func__, mCtx->dataResponse->setDataProfileResponse(toAidl(info)));
    }
    return Void();
}

// ---------------------------------------------------------------------------
// Remaining V1.0 methods: never forwarded, drop.
// ---------------------------------------------------------------------------

#define DROP_METHOD(METHOD)                                    \
    Return<void> ResponseHandler::METHOD(const R10::RadioResponseInfo& info) { \
        drop(__func__, info, mCtx->popRequest(info.serial));   \
        return Void();                                         \
    }

DROP_METHOD(setBarringPasswordResponse)
DROP_METHOD(setBandModeResponse)
DROP_METHOD(setCdmaSubscriptionSourceResponse)
DROP_METHOD(setCdmaRoamingPreferenceResponse)
DROP_METHOD(setPreferredVoicePrivacyResponse)
DROP_METHOD(sendCDMAFeatureCodeResponse)
DROP_METHOD(sendBurstDtmfResponse)
DROP_METHOD(acknowledgeLastIncomingCdmaSmsResponse)
DROP_METHOD(setCdmaBroadcastConfigResponse)
DROP_METHOD(setCdmaBroadcastActivationResponse)
DROP_METHOD(exitEmergencyCallbackModeResponse)
DROP_METHOD(setCellInfoListRateResponse)
DROP_METHOD(nvWriteItemResponse)
DROP_METHOD(nvWriteCdmaPrlResponse)
DROP_METHOD(nvResetConfigResponse)
DROP_METHOD(setUiccSubscriptionResponse)
DROP_METHOD(deleteSmsOnRuimResponse)

Return<void> ResponseHandler::getAvailableBandModesResponse(
        const R10::RadioResponseInfo& info, const hidl_vec<R10::RadioBandMode>&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::getNeighboringCidsResponse(
        const R10::RadioResponseInfo& info, const hidl_vec<R10::NeighboringCell>&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::getCdmaRoamingPreferenceResponse(const R10::RadioResponseInfo& info,
                                                               R10::CdmaRoamingType) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::getPreferredVoicePrivacyResponse(const R10::RadioResponseInfo& info,
                                                               bool) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::sendCdmaSmsResponse(const R10::RadioResponseInfo& info,
                                                  const R10::SendSmsResult&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::getCdmaBroadcastConfigResponse(
        const R10::RadioResponseInfo& info, const hidl_vec<R10::CdmaBroadcastSmsConfigInfo>&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::writeSmsToRuimResponse(const R10::RadioResponseInfo& info,
                                                          uint32_t) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::stopLceServiceResponse(const R10::RadioResponseInfo& info,
                                                     const R10::LceStatusInfo&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::getCDMASubscriptionResponse(const R10::RadioResponseInfo& info,
                                                          const hidl_string&, const hidl_string&,
                                                          const hidl_string&, const hidl_string&,
                                                          const hidl_string&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::getCdmaSubscriptionSourceResponse(
        const R10::RadioResponseInfo& info, R10::CdmaSubscriptionSource) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::requestIsimAuthenticationResponse(
        const R10::RadioResponseInfo& info, const hidl_string&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::getCellInfoListResponse(const R10::RadioResponseInfo& info,
                                                      const hidl_vec<R10::CellInfo>&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::getImsRegistrationStateResponse(const R10::RadioResponseInfo& info,
                                                              bool, R10::RadioTechnologyFamily) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::sendImsSmsResponse(const R10::RadioResponseInfo& info,
                                                 const R10::SendSmsResult&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::nvReadItemResponse(const R10::RadioResponseInfo& info,
                                                 const hidl_string&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::startLceServiceResponse(const R10::RadioResponseInfo& info,
                                                      const R10::LceStatusInfo&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::pullLceDataResponse(const R10::RadioResponseInfo& info,
                                                  const R10::LceDataInfo&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::getModemActivityInfoResponse(const R10::RadioResponseInfo& info,
                                                           const R10::ActivityStatsInfo&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::setAllowedCarriersResponse(const R10::RadioResponseInfo& info,
                                                         int32_t) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::getAllowedCarriersResponse(const R10::RadioResponseInfo& info,
                                                         bool, const R10::CarrierRestrictions&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

// ---------------------------------------------------------------------------
// V1.1 methods.
// ---------------------------------------------------------------------------

Return<void> ResponseHandler::startNetworkScanResponse(const R10::RadioResponseInfo& info) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::stopNetworkScanResponse(const R10::RadioResponseInfo& info) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::startKeepaliveResponse(const R10::RadioResponseInfo& info,
                                                     const R11::KeepaliveStatus&) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

Return<void> ResponseHandler::stopKeepaliveResponse(const R10::RadioResponseInfo& info) {
    drop(__func__, info, mCtx->popRequest(info.serial));
    return Void();
}

}  // namespace android::hardware::radio::shim
