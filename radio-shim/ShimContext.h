/*
 * Shared context for the mako radio shim.
 *
 * Holds the HIDL V1.1 IRadio handle (the backend), the AIDL response and
 * indication callbacks registered by the framework via setResponseFunctions,
 * and the serial -> request-kind routing table used to deliver HIDL responses
 * to the correct AIDL callback.
 */
#pragma once

#include <android/hardware/radio/1.1/IRadio.h>
#include <android/hardware/radio/1.1/IRadioIndication.h>
#include <android/hardware/radio/1.1/IRadioResponse.h>

#include <aidl/android/hardware/radio/config/IRadioConfigIndication.h>
#include <aidl/android/hardware/radio/config/IRadioConfigResponse.h>
#include <aidl/android/hardware/radio/data/IRadioDataIndication.h>
#include <aidl/android/hardware/radio/data/IRadioDataResponse.h>
#include <aidl/android/hardware/radio/messaging/IRadioMessagingIndication.h>
#include <aidl/android/hardware/radio/messaging/IRadioMessagingResponse.h>
#include <aidl/android/hardware/radio/modem/IRadioModemIndication.h>
#include <aidl/android/hardware/radio/modem/IRadioModemResponse.h>
#include <aidl/android/hardware/radio/network/IRadioNetworkIndication.h>
#include <aidl/android/hardware/radio/network/IRadioNetworkResponse.h>
#include <aidl/android/hardware/radio/sim/IRadioSimIndication.h>
#include <aidl/android/hardware/radio/sim/IRadioSimResponse.h>
#include <aidl/android/hardware/radio/voice/IRadioVoiceIndication.h>
#include <aidl/android/hardware/radio/voice/IRadioVoiceResponse.h>

#include <aidl/android/hardware/radio/RadioError.h>
#include <aidl/android/hardware/radio/RadioResponseInfo.h>
#include <aidl/android/hardware/radio/RadioResponseType.h>

#include <cstdint>
#include <mutex>
#include <unordered_map>

namespace android::hardware::radio::shim {

namespace R10 = ::android::hardware::radio::V1_0;
namespace R11 = ::android::hardware::radio::V1_1;
namespace ar = ::aidl::android::hardware::radio;

// Flat enumeration of every request the shim forwards to the HIDL radio.
// The response handler uses it to translate the HIDL response payload into
// the right AIDL response method.
enum class RequestKind : uint16_t {
    NONE = 0,
    MODEM_SET_RADIO_POWER,
    MODEM_GET_IMEI,
    MODEM_GET_DEVICE_IDENTITY,
    MODEM_GET_BASEBAND_VERSION,
    MODEM_GET_RADIO_CAPABILITY,
    MODEM_SET_RADIO_CAPABILITY,
    MODEM_GET_HARDWARE_CONFIG,
    MODEM_GET_MODEM_STACK_STATUS,
    MODEM_REQUEST_SHUTDOWN,
    MODEM_SEND_DEVICE_STATE,
    SIM_GET_ICC_CARD_STATUS,
    SIM_ICC_IO_FOR_APP,
    SIM_GET_IMSI_FOR_APP,
    SIM_SUPPLY_ICC_PIN_FOR_APP,
    SIM_SUPPLY_ICC_PUK_FOR_APP,
    SIM_SUPPLY_ICC_PIN2_FOR_APP,
    SIM_SUPPLY_ICC_PUK2_FOR_APP,
    SIM_CHANGE_ICC_PIN_FOR_APP,
    SIM_CHANGE_ICC_PIN2_FOR_APP,
    SIM_GET_FACILITY_LOCK_FOR_APP,
    SIM_SET_FACILITY_LOCK_FOR_APP,
    SIM_SET_SIM_CARD_POWER,
    SIM_REPORT_STK_SERVICE_IS_RUNNING,
    SIM_SEND_ENVELOPE,
    SIM_SEND_ENVELOPE_WITH_STATUS,
    SIM_SEND_TERMINAL_RESPONSE_TO_SIM,
    SIM_REQUEST_ICC_SIM_AUTHENTICATION,
    SIM_ICC_TRANSMIT_APDU_BASIC_CHANNEL,
    SIM_ICC_OPEN_LOGICAL_CHANNEL,
    SIM_ICC_CLOSE_LOGICAL_CHANNEL,
    SIM_ICC_TRANSMIT_APDU_LOGICAL_CHANNEL,
    SIM_SET_CARRIER_INFO_FOR_IMSI_ENCRYPTION,
    SIM_GET_ALLOWED_CARRIERS,
    SIM_SET_ALLOWED_CARRIERS,
    NET_GET_SIGNAL_STRENGTH,
    NET_GET_VOICE_REGISTRATION_STATE,
    NET_GET_DATA_REGISTRATION_STATE,
    NET_GET_OPERATOR,
    NET_GET_NETWORK_SELECTION_MODE,
    NET_SET_NETWORK_SELECTION_MODE_AUTOMATIC,
    NET_SET_NETWORK_SELECTION_MODE_MANUAL,
    NET_GET_VOICE_RADIO_TECHNOLOGY,
    NET_GET_PREFERRED_NETWORK_TYPE,
    NET_SET_PREFERRED_NETWORK_TYPE,
    NET_SET_INDICATION_FILTER,
    NET_SET_LOCATION_UPDATES,
    NET_GET_AVAILABLE_NETWORKS,
    NET_SET_SUPP_SERVICE_NOTIFICATIONS,
    VOICE_GET_CURRENT_CALLS,
    VOICE_DIAL,
    VOICE_ACCEPT_CALL,
    VOICE_REJECT_CALL,
    VOICE_HANGUP,
    VOICE_HANGUP_WAITING_OR_BACKGROUND,
    VOICE_HANGUP_FOREGROUND_RESUME_BACKGROUND,
    VOICE_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE,
    VOICE_CONFERENCE,
    VOICE_SEPARATE_CONNECTION,
    VOICE_GET_LAST_CALL_FAIL_CAUSE,
    VOICE_SEND_DTMF,
    VOICE_START_DTMF,
    VOICE_STOP_DTMF,
    VOICE_SEND_USSD,
    VOICE_CANCEL_PENDING_USSD,
    VOICE_GET_CLIR,
    VOICE_SET_CLIR,
    VOICE_GET_CLIP,
    VOICE_GET_CALL_FORWARD_STATUS,
    VOICE_SET_CALL_FORWARD,
    VOICE_GET_CALL_WAITING,
    VOICE_SET_CALL_WAITING,
    VOICE_GET_MUTE,
    VOICE_SET_MUTE,
    VOICE_EXPLICIT_CALL_TRANSFER,
    VOICE_HANDLE_STK_CALL_SETUP_REQUEST_FROM_SIM,
    VOICE_GET_TTY_MODE,
    VOICE_SET_TTY_MODE,
    MSG_SEND_SMS,
    MSG_SEND_SMS_EXPECT_MORE,
    MSG_ACK_LAST_INCOMING_GSM_SMS,
    MSG_ACK_INCOMING_GSM_SMS_WITH_PDU,
    MSG_GET_SMSC_ADDRESS,
    MSG_SET_SMSC_ADDRESS,
    MSG_WRITE_SMS_TO_SIM,
    MSG_DELETE_SMS_ON_SIM,
    MSG_GET_GSM_BROADCAST_CONFIG,
    MSG_SET_GSM_BROADCAST_CONFIG,
    MSG_SET_GSM_BROADCAST_ACTIVATION,
    MSG_REPORT_SMS_MEMORY_STATUS,
    DATA_SETUP_DATA_CALL,
    DATA_DEACTIVATE_DATA_CALL,
    DATA_GET_DATA_CALL_LIST,
    DATA_SET_INITIAL_ATTACH_APN,
    DATA_SET_DATA_ALLOWED,
    DATA_SET_DATA_PROFILE,
    DATA_START_KEEPALIVE,
    DATA_STOP_KEEPALIVE,
    CONFIG_GET_SIM_SLOTS_STATUS,
    SIM_INTERNAL_READ_ICCID,
    SIM_INTERNAL_READ_ICCID_DATA,
};

class ShimContext {
  public:
    // HIDL backend (rild's V1.1 IRadio) and the shim's own callbacks.
    sp<R11::IRadio> radio;
    sp<R11::IRadioResponse> hidlResponse;
    sp<R11::IRadioIndication> hidlIndication;

    // AIDL callbacks registered by the framework.
    std::shared_ptr<ar::modem::IRadioModemResponse> modemResponse;
    std::shared_ptr<ar::modem::IRadioModemIndication> modemIndication;
    std::shared_ptr<ar::sim::IRadioSimResponse> simResponse;
    std::shared_ptr<ar::sim::IRadioSimIndication> simIndication;
    std::shared_ptr<ar::network::IRadioNetworkResponse> networkResponse;
    std::shared_ptr<ar::network::IRadioNetworkIndication> networkIndication;
    std::shared_ptr<ar::voice::IRadioVoiceResponse> voiceResponse;
    std::shared_ptr<ar::voice::IRadioVoiceIndication> voiceIndication;
    std::shared_ptr<ar::messaging::IRadioMessagingResponse> messagingResponse;
    std::shared_ptr<ar::messaging::IRadioMessagingIndication> messagingIndication;
    std::shared_ptr<ar::data::IRadioDataResponse> dataResponse;
    std::shared_ptr<ar::data::IRadioDataIndication> dataIndication;
    std::shared_ptr<ar::config::IRadioConfigResponse> configResponse;
    std::shared_ptr<ar::config::IRadioConfigIndication> configIndication;

    // The SIM ICCID (EF-ICCID), read by the shim itself because the V1.0
    // CardStatus carries no iccid. The framework needs it to match the
    // subscription to the slot; without it the subscription never becomes
    // active and the Settings SIM status shows "Not available".
    std::string getIccId();
    void setIccId(const std::string& iccid);
    // Kicks off an asynchronous EF-ICCID read (two iccIOForApp requests).
    void refreshIccId();
    // Retries the EF-ICCID read after a delay; the mako qcril occasionally
    // returns empty responses so the read must be retried.
    void scheduleIccIdRetry();
    // Serial for shim-internal HIDL requests (kept far above the framework
    // serial space so it can never collide).
    int32_t nextInternalSerial();

    // Registers a request before forwarding it to the HIDL radio.
    void registerRequest(int32_t serial, RequestKind kind);

    // Removes and returns the request kind for a serial. Returns NONE when
    // the serial is unknown, which happens for duplicate deliveries (the mako
    // qcril can complete the same request twice) and for responses to
    // requests that were never forwarded. Duplicates are dropped by the
    // response handler.
    RequestKind popRequest(int32_t serial);

    // Removes the request entry without delivering anything.
    void dropRequest(int32_t serial);

    static ar::RadioResponseInfo responseInfo(int32_t serial, ar::RadioError error);
    static ar::RadioResponseInfo noError(int32_t serial);
    static ar::RadioResponseInfo notSupported(int32_t serial);

  private:
    std::mutex mMutex;
    std::unordered_map<int32_t, RequestKind> mSerialToKind;
    std::string mIccId;
    int32_t mIccIdRetries = 0;
    int32_t mNextInternalSerial = 0x40000000;
};

}  // namespace android::hardware::radio::shim
