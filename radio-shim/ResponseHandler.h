/*
 * Implements the HIDL V1.0/V1.1 IRadioResponse callbacks the shim registers
 * with rild, and forwards translated responses to the AIDL callbacks that the
 * framework registered via setResponseFunctions. Routing is done through the
 * serial -> RequestKind table in ShimContext; duplicate deliveries (the mako
 * qcril can complete the same request twice) are dropped.
 */
#pragma once

#include <android/hardware/radio/1.1/IRadioResponse.h>

#include <android/binder_auto_utils.h>

#include "ShimContext.h"

namespace android::hardware::radio::shim {

class ResponseHandler : public R11::IRadioResponse {
  public:
    explicit ResponseHandler(std::shared_ptr<ShimContext> ctx) : mCtx(std::move(ctx)) {}

    // V1.0 methods.
    Return<void> getIccCardStatusResponse(const R10::RadioResponseInfo& info,
                                          const R10::CardStatus& cardStatus) override;
    Return<void> supplyIccPinForAppResponse(const R10::RadioResponseInfo& info,
                                            int32_t remainingRetries) override;
    Return<void> supplyIccPukForAppResponse(const R10::RadioResponseInfo& info,
                                            int32_t remainingRetries) override;
    Return<void> supplyIccPin2ForAppResponse(const R10::RadioResponseInfo& info,
                                             int32_t remainingRetries) override;
    Return<void> supplyIccPuk2ForAppResponse(const R10::RadioResponseInfo& info,
                                             int32_t remainingRetries) override;
    Return<void> changeIccPinForAppResponse(const R10::RadioResponseInfo& info,
                                            int32_t remainingRetries) override;
    Return<void> changeIccPin2ForAppResponse(const R10::RadioResponseInfo& info,
                                             int32_t remainingRetries) override;
    Return<void> supplyNetworkDepersonalizationResponse(const R10::RadioResponseInfo& info,
                                                        int32_t remainingRetries) override;
    Return<void> getCurrentCallsResponse(const R10::RadioResponseInfo& info,
                                         const hidl_vec<R10::Call>& calls) override;
    Return<void> dialResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getIMSIForAppResponse(const R10::RadioResponseInfo& info,
                                       const hidl_string& imsi) override;
    Return<void> hangupConnectionResponse(const R10::RadioResponseInfo& info) override;
    Return<void> hangupWaitingOrBackgroundResponse(const R10::RadioResponseInfo& info) override;
    Return<void> hangupForegroundResumeBackgroundResponse(
            const R10::RadioResponseInfo& info) override;
    Return<void> switchWaitingOrHoldingAndActiveResponse(const R10::RadioResponseInfo& info) override;
    Return<void> conferenceResponse(const R10::RadioResponseInfo& info) override;
    Return<void> rejectCallResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getLastCallFailCauseResponse(
            const R10::RadioResponseInfo& info,
            const R10::LastCallFailCauseInfo& failCauseinfo) override;
    Return<void> getSignalStrengthResponse(const R10::RadioResponseInfo& info,
                                           const R10::SignalStrength& sigStrength) override;
    Return<void> getVoiceRegistrationStateResponse(
            const R10::RadioResponseInfo& info,
            const R10::VoiceRegStateResult& voiceRegResponse) override;
    Return<void> getDataRegistrationStateResponse(
            const R10::RadioResponseInfo& info,
            const R10::DataRegStateResult& dataRegResponse) override;
    Return<void> getOperatorResponse(const R10::RadioResponseInfo& info,
                                     const hidl_string& longName,
                                     const hidl_string& shortName,
                                     const hidl_string& numeric) override;
    Return<void> setRadioPowerResponse(const R10::RadioResponseInfo& info) override;
    Return<void> sendDtmfResponse(const R10::RadioResponseInfo& info) override;
    Return<void> sendSmsResponse(const R10::RadioResponseInfo& info,
                                 const R10::SendSmsResult& sms) override;
    Return<void> sendSMSExpectMoreResponse(const R10::RadioResponseInfo& info,
                                           const R10::SendSmsResult& sms) override;
    Return<void> setupDataCallResponse(const R10::RadioResponseInfo& info,
                                       const R10::SetupDataCallResult& dcResponse) override;
    Return<void> iccIOForAppResponse(const R10::RadioResponseInfo& info,
                                     const R10::IccIoResult& iccIo) override;
    Return<void> sendUssdResponse(const R10::RadioResponseInfo& info) override;
    Return<void> cancelPendingUssdResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getClirResponse(const R10::RadioResponseInfo& info, int32_t n,
                                 int32_t m) override;
    Return<void> setClirResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getCallForwardStatusResponse(
            const R10::RadioResponseInfo& info,
            const hidl_vec<R10::CallForwardInfo>& callForwardInfos) override;
    Return<void> setCallForwardResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getCallWaitingResponse(const R10::RadioResponseInfo& info, bool enable,
                                        int32_t serviceClass) override;
    Return<void> setCallWaitingResponse(const R10::RadioResponseInfo& info) override;
    Return<void> acknowledgeLastIncomingGsmSmsResponse(const R10::RadioResponseInfo& info) override;
    Return<void> acceptCallResponse(const R10::RadioResponseInfo& info) override;
    Return<void> deactivateDataCallResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getFacilityLockForAppResponse(const R10::RadioResponseInfo& info,
                                               int32_t response) override;
    Return<void> setFacilityLockForAppResponse(const R10::RadioResponseInfo& info,
                                               int32_t retry) override;
    Return<void> getNetworkSelectionModeResponse(const R10::RadioResponseInfo& info,
                                                 bool manual) override;
    Return<void> setNetworkSelectionModeAutomaticResponse(
            const R10::RadioResponseInfo& info) override;
    Return<void> setNetworkSelectionModeManualResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getAvailableNetworksResponse(
            const R10::RadioResponseInfo& info,
            const hidl_vec<R10::OperatorInfo>& networkInfos) override;
    Return<void> startDtmfResponse(const R10::RadioResponseInfo& info) override;
    Return<void> stopDtmfResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getBasebandVersionResponse(const R10::RadioResponseInfo& info,
                                            const hidl_string& version) override;
    Return<void> separateConnectionResponse(const R10::RadioResponseInfo& info) override;
    Return<void> setMuteResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getMuteResponse(const R10::RadioResponseInfo& info, bool enable) override;
    Return<void> getClipResponse(const R10::RadioResponseInfo& info,
                                 R10::ClipStatus status) override;
    Return<void> getDataCallListResponse(
            const R10::RadioResponseInfo& info,
            const hidl_vec<R10::SetupDataCallResult>& dcResponse) override;
    Return<void> setSuppServiceNotificationsResponse(const R10::RadioResponseInfo& info) override;
    Return<void> writeSmsToSimResponse(const R10::RadioResponseInfo& info,
                                       int32_t index) override;
    Return<void> deleteSmsOnSimResponse(const R10::RadioResponseInfo& info) override;
    Return<void> sendEnvelopeResponse(const R10::RadioResponseInfo& info,
                                      const hidl_string& commandResponse) override;
    Return<void> sendTerminalResponseToSimResponse(const R10::RadioResponseInfo& info) override;
    Return<void> handleStkCallSetupRequestFromSimResponse(
            const R10::RadioResponseInfo& info) override;
    Return<void> explicitCallTransferResponse(const R10::RadioResponseInfo& info) override;
    Return<void> setPreferredNetworkTypeResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getPreferredNetworkTypeResponse(
            const R10::RadioResponseInfo& info, R10::PreferredNetworkType networkType) override;
    Return<void> setLocationUpdatesResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getGsmBroadcastConfigResponse(
            const R10::RadioResponseInfo& info,
            const hidl_vec<R10::GsmBroadcastSmsConfigInfo>& configs) override;
    Return<void> setGsmBroadcastConfigResponse(const R10::RadioResponseInfo& info) override;
    Return<void> setGsmBroadcastActivationResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getDeviceIdentityResponse(const R10::RadioResponseInfo& info,
                                           const hidl_string& imei, const hidl_string& imeisv,
                                           const hidl_string& svn,
                                           const hidl_string& spn) override;
    Return<void> getSmscAddressResponse(const R10::RadioResponseInfo& info,
                                        const hidl_string& smsc) override;
    Return<void> setSmscAddressResponse(const R10::RadioResponseInfo& info) override;
    Return<void> reportSmsMemoryStatusResponse(const R10::RadioResponseInfo& info) override;
    Return<void> reportStkServiceIsRunningResponse(const R10::RadioResponseInfo& info) override;
    Return<void> acknowledgeIncomingGsmSmsWithPduResponse(
            const R10::RadioResponseInfo& info) override;
    Return<void> sendEnvelopeWithStatusResponse(const R10::RadioResponseInfo& info,
                                                const R10::IccIoResult& iccIo) override;
    Return<void> getVoiceRadioTechnologyResponse(
            const R10::RadioResponseInfo& info, R10::RadioTechnology rat) override;
    Return<void> setInitialAttachApnResponse(const R10::RadioResponseInfo& info) override;
    Return<void> iccTransmitApduBasicChannelResponse(const R10::RadioResponseInfo& info,
                                                     const R10::IccIoResult& result) override;
    Return<void> iccOpenLogicalChannelResponse(const R10::RadioResponseInfo& info,
                                               int32_t channelId,
                                               const hidl_vec<int8_t>& selectResponse) override;
    Return<void> iccCloseLogicalChannelResponse(const R10::RadioResponseInfo& info) override;
    Return<void> iccTransmitApduLogicalChannelResponse(const R10::RadioResponseInfo& info,
                                                       const R10::IccIoResult& result) override;
    Return<void> requestIccSimAuthenticationResponse(const R10::RadioResponseInfo& info,
                                                     const R10::IccIoResult& result) override;
    Return<void> setDataAllowedResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getHardwareConfigResponse(
            const R10::RadioResponseInfo& info,
            const hidl_vec<R10::HardwareConfig>& config) override;
    Return<void> setDataProfileResponse(const R10::RadioResponseInfo& info) override;
    Return<void> requestShutdownResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getRadioCapabilityResponse(const R10::RadioResponseInfo& info,
                                            const R10::RadioCapability& rc) override;
    Return<void> setRadioCapabilityResponse(const R10::RadioResponseInfo& info,
                                            const R10::RadioCapability& rc) override;
    Return<void> sendDeviceStateResponse(const R10::RadioResponseInfo& info) override;
    Return<void> setIndicationFilterResponse(const R10::RadioResponseInfo& info) override;
    Return<void> setSimCardPowerResponse(const R10::RadioResponseInfo& info) override;

    // All remaining V1.0 methods are not expected because the corresponding
    // requests are never forwarded; they are implemented as drops.
    Return<void> setBarringPasswordResponse(const R10::RadioResponseInfo& info) override;
    Return<void> setBandModeResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getAvailableBandModesResponse(
            const R10::RadioResponseInfo& info,
            const hidl_vec<R10::RadioBandMode>& bandModes) override;
    Return<void> getNeighboringCidsResponse(
            const R10::RadioResponseInfo& info,
            const hidl_vec<R10::NeighboringCell>& cells) override;
    Return<void> setCdmaSubscriptionSourceResponse(const R10::RadioResponseInfo& info) override;
    Return<void> setCdmaRoamingPreferenceResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getCdmaRoamingPreferenceResponse(const R10::RadioResponseInfo& info,
                                                  R10::CdmaRoamingType type) override;
    Return<void> setTTYModeResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getTTYModeResponse(const R10::RadioResponseInfo& info,
                                    R10::TtyMode mode) override;
    Return<void> setPreferredVoicePrivacyResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getPreferredVoicePrivacyResponse(const R10::RadioResponseInfo& info,
                                                  bool enable) override;
    Return<void> sendCDMAFeatureCodeResponse(const R10::RadioResponseInfo& info) override;
    Return<void> sendBurstDtmfResponse(const R10::RadioResponseInfo& info) override;
    Return<void> sendCdmaSmsResponse(const R10::RadioResponseInfo& info,
                                     const R10::SendSmsResult& sms) override;
    Return<void> acknowledgeLastIncomingCdmaSmsResponse(
            const R10::RadioResponseInfo& info) override;
    Return<void> getCdmaBroadcastConfigResponse(
            const R10::RadioResponseInfo& info,
            const hidl_vec<R10::CdmaBroadcastSmsConfigInfo>& configs) override;
    Return<void> setCdmaBroadcastConfigResponse(const R10::RadioResponseInfo& info) override;
    Return<void> setCdmaBroadcastActivationResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getCDMASubscriptionResponse(const R10::RadioResponseInfo& info,
                                             const hidl_string& mdn, const hidl_string& hSid,
                                             const hidl_string& hNid, const hidl_string& min,
                                             const hidl_string& prl) override;
    Return<void> writeSmsToRuimResponse(const R10::RadioResponseInfo& info,
                                        uint32_t index) override;
    Return<void> deleteSmsOnRuimResponse(const R10::RadioResponseInfo& info) override;
    Return<void> exitEmergencyCallbackModeResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getCdmaSubscriptionSourceResponse(const R10::RadioResponseInfo& info,
                                                   R10::CdmaSubscriptionSource source) override;
    Return<void> requestIsimAuthenticationResponse(const R10::RadioResponseInfo& info,
                                                   const hidl_string& response) override;
    Return<void> getCellInfoListResponse(
            const R10::RadioResponseInfo& info, const hidl_vec<R10::CellInfo>& cellInfo) override;
    Return<void> setCellInfoListRateResponse(const R10::RadioResponseInfo& info) override;
    Return<void> getImsRegistrationStateResponse(const R10::RadioResponseInfo& info,
                                                 bool isRegistered,
                                                 R10::RadioTechnologyFamily ratFamily) override;
    Return<void> sendImsSmsResponse(const R10::RadioResponseInfo& info,
                                    const R10::SendSmsResult& sms) override;
    Return<void> nvReadItemResponse(const R10::RadioResponseInfo& info,
                                    const hidl_string& result) override;
    Return<void> nvWriteItemResponse(const R10::RadioResponseInfo& info) override;
    Return<void> nvWriteCdmaPrlResponse(const R10::RadioResponseInfo& info) override;
    Return<void> nvResetConfigResponse(const R10::RadioResponseInfo& info) override;
    Return<void> setUiccSubscriptionResponse(const R10::RadioResponseInfo& info) override;
    Return<void> startLceServiceResponse(const R10::RadioResponseInfo& info,
                                         const R10::LceStatusInfo& statusInfo) override;
    Return<void> stopLceServiceResponse(const R10::RadioResponseInfo& info,
                                        const R10::LceStatusInfo& statusInfo) override;
    Return<void> pullLceDataResponse(const R10::RadioResponseInfo& info,
                                     const R10::LceDataInfo& lceInfo) override;
    Return<void> getModemActivityInfoResponse(
            const R10::RadioResponseInfo& info, const R10::ActivityStatsInfo& activityInfo) override;
    Return<void> setAllowedCarriersResponse(const R10::RadioResponseInfo& info,
                                            int32_t numAllowed) override;
    Return<void> getAllowedCarriersResponse(
            const R10::RadioResponseInfo& info, bool allAllowed,
            const R10::CarrierRestrictions& carriers) override;
    Return<void> acknowledgeRequest(int32_t serial) override;

    // V1.1 methods.
    Return<void> setCarrierInfoForImsiEncryptionResponse(
            const R10::RadioResponseInfo& info) override;
    Return<void> setSimCardPowerResponse_1_1(const R10::RadioResponseInfo& info) override;
    Return<void> startNetworkScanResponse(const R10::RadioResponseInfo& info) override;
    Return<void> stopNetworkScanResponse(const R10::RadioResponseInfo& info) override;
    Return<void> startKeepaliveResponse(const R10::RadioResponseInfo& info,
                                        const R11::KeepaliveStatus& status) override;
    Return<void> stopKeepaliveResponse(const R10::RadioResponseInfo& info) override;

  private:
    std::shared_ptr<ShimContext> mCtx;

    void drop(const char* method, const R10::RadioResponseInfo& info, RequestKind kind);
    static void callStatus(const char* method, const ::ndk::ScopedAStatus& status);
};

}  // namespace android::hardware::radio::shim
