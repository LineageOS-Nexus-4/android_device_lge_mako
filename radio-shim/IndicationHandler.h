/*
 * Implements the HIDL V1.0/V1.1 IRadioIndication callbacks the shim registers
 * with rild and forwards translated unsolicited messages to the AIDL
 * indication callbacks registered by the framework.
 */
#pragma once

#include <android/hardware/radio/1.1/IRadioIndication.h>

#include <android/binder_auto_utils.h>

#include "ShimContext.h"

namespace android::hardware::radio::shim {

class IndicationHandler : public R11::IRadioIndication {
  public:
    explicit IndicationHandler(std::shared_ptr<ShimContext> ctx) : mCtx(std::move(ctx)) {}

    // V1.0 methods.
    Return<void> radioStateChanged(R10::RadioIndicationType type,
                                   R10::RadioState radioState) override;
    Return<void> callStateChanged(R10::RadioIndicationType type) override;
    Return<void> networkStateChanged(R10::RadioIndicationType type) override;
    Return<void> newSms(R10::RadioIndicationType type, const hidl_vec<uint8_t>& pdu) override;
    Return<void> newSmsStatusReport(R10::RadioIndicationType type,
                                    const hidl_vec<uint8_t>& pdu) override;
    Return<void> newSmsOnSim(R10::RadioIndicationType type, int32_t recordNumber) override;
    Return<void> onUssd(R10::RadioIndicationType type, R10::UssdModeType modeType,
                        const hidl_string& msg) override;
    Return<void> nitzTimeReceived(R10::RadioIndicationType type, const hidl_string& nitzTime,
                                  uint64_t receivedTime) override;
    Return<void> currentSignalStrength(R10::RadioIndicationType type,
                                       const R10::SignalStrength& signalStrength) override;
    Return<void> dataCallListChanged(R10::RadioIndicationType type,
                                     const hidl_vec<R10::SetupDataCallResult>& dcList) override;
    Return<void> suppSvcNotify(R10::RadioIndicationType type,
                               const R10::SuppSvcNotification& suppSvc) override;
    Return<void> stkSessionEnd(R10::RadioIndicationType type) override;
    Return<void> stkProactiveCommand(R10::RadioIndicationType type,
                                     const hidl_string& cmd) override;
    Return<void> stkEventNotify(R10::RadioIndicationType type, const hidl_string& cmd) override;
    Return<void> stkCallSetup(R10::RadioIndicationType type, int64_t timeout) override;
    Return<void> simSmsStorageFull(R10::RadioIndicationType type) override;
    Return<void> simRefresh(R10::RadioIndicationType type,
                            const R10::SimRefreshResult& refreshResult) override;
    Return<void> callRing(R10::RadioIndicationType type, bool isGsm,
                          const R10::CdmaSignalInfoRecord& record) override;
    Return<void> simStatusChanged(R10::RadioIndicationType type) override;
    Return<void> cdmaNewSms(R10::RadioIndicationType type,
                            const R10::CdmaSmsMessage& msg) override;
    Return<void> newBroadcastSms(R10::RadioIndicationType type,
                                 const hidl_vec<uint8_t>& data) override;
    Return<void> cdmaRuimSmsStorageFull(R10::RadioIndicationType type) override;
    Return<void> restrictedStateChanged(R10::RadioIndicationType type,
                                        R10::PhoneRestrictedState state) override;
    Return<void> enterEmergencyCallbackMode(R10::RadioIndicationType type) override;
    Return<void> cdmaCallWaiting(R10::RadioIndicationType type,
                                 const R10::CdmaCallWaiting& callWaitingRecord) override;
    Return<void> cdmaOtaProvisionStatus(R10::RadioIndicationType type,
                                        R10::CdmaOtaProvisionStatus status) override;
    Return<void> cdmaInfoRec(R10::RadioIndicationType type,
                             const R10::CdmaInformationRecords& records) override;
    Return<void> indicateRingbackTone(R10::RadioIndicationType type, bool start) override;
    Return<void> resendIncallMute(R10::RadioIndicationType type) override;
    Return<void> cdmaSubscriptionSourceChanged(R10::RadioIndicationType type,
                                               R10::CdmaSubscriptionSource cdmaSub) override;
    Return<void> cdmaPrlChanged(R10::RadioIndicationType type, int32_t version) override;
    Return<void> exitEmergencyCallbackMode(R10::RadioIndicationType type) override;
    Return<void> rilConnected(R10::RadioIndicationType type) override;
    Return<void> voiceRadioTechChanged(R10::RadioIndicationType type,
                                       R10::RadioTechnology rat) override;
    Return<void> cellInfoList(R10::RadioIndicationType type,
                              const hidl_vec<R10::CellInfo>& records) override;
    Return<void> imsNetworkStateChanged(R10::RadioIndicationType type) override;
    Return<void> subscriptionStatusChanged(R10::RadioIndicationType type,
                                           bool activate) override;
    Return<void> srvccStateNotify(R10::RadioIndicationType type, R10::SrvccState state) override;
    Return<void> hardwareConfigChanged(R10::RadioIndicationType type,
                                       const hidl_vec<R10::HardwareConfig>& configs) override;
    Return<void> radioCapabilityIndication(R10::RadioIndicationType type,
                                           const R10::RadioCapability& rc) override;
    Return<void> onSupplementaryServiceIndication(
            R10::RadioIndicationType type, const R10::StkCcUnsolSsResult& ss) override;
    Return<void> stkCallControlAlphaNotify(R10::RadioIndicationType type,
                                           const hidl_string& alpha) override;
    Return<void> lceData(R10::RadioIndicationType type, const R10::LceDataInfo& lce) override;
    Return<void> pcoData(R10::RadioIndicationType type, const R10::PcoDataInfo& pco) override;
    Return<void> modemReset(R10::RadioIndicationType type, const hidl_string& reason) override;

    // V1.1 methods.
    Return<void> carrierInfoForImsiEncryption(R10::RadioIndicationType type) override;
    Return<void> networkScanResult(R10::RadioIndicationType type,
                                   const R11::NetworkScanResult& result) override;
    Return<void> keepaliveStatus(R10::RadioIndicationType type,
                                 const R11::KeepaliveStatus& status) override;

  private:
    std::shared_ptr<ShimContext> mCtx;
    static void callStatus(const char* method, const ::ndk::ScopedAStatus& status);
    static void drop(const char* method, R10::RadioIndicationType type);
};

}  // namespace android::hardware::radio::shim
