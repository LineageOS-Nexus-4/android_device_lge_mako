#pragma once

#include <aidl/android/hardware/radio/network/BnRadioNetwork.h>

#include "ShimContext.h"

namespace android::hardware::radio::shim {

class RadioNetwork : public ar::network::BnRadioNetwork {
  public:
    explicit RadioNetwork(std::shared_ptr<ShimContext> ctx) : mCtx(std::move(ctx)) {}

    ::ndk::ScopedAStatus getAllowedNetworkTypesBitmap(int32_t serial) override;
    ::ndk::ScopedAStatus getAvailableBandModes(int32_t serial) override;
    ::ndk::ScopedAStatus getAvailableNetworks(int32_t serial) override;
    ::ndk::ScopedAStatus getBarringInfo(int32_t serial) override;
    ::ndk::ScopedAStatus getCdmaRoamingPreference(int32_t serial) override;
    ::ndk::ScopedAStatus getCellInfoList(int32_t serial) override;
    ::ndk::ScopedAStatus getDataRegistrationState(int32_t serial) override;
    ::ndk::ScopedAStatus getImsRegistrationState(int32_t serial) override;
    ::ndk::ScopedAStatus getNetworkSelectionMode(int32_t serial) override;
    ::ndk::ScopedAStatus getOperator(int32_t serial) override;
    ::ndk::ScopedAStatus getSignalStrength(int32_t serial) override;
    ::ndk::ScopedAStatus getSystemSelectionChannels(int32_t serial) override;
    ::ndk::ScopedAStatus getVoiceRadioTechnology(int32_t serial) override;
    ::ndk::ScopedAStatus getVoiceRegistrationState(int32_t serial) override;
    ::ndk::ScopedAStatus isNrDualConnectivityEnabled(int32_t serial) override;
    ::ndk::ScopedAStatus responseAcknowledgement() override;
    ::ndk::ScopedAStatus setAllowedNetworkTypesBitmap(int32_t serial,
                                                      int32_t networkTypeBitmap) override;
    ::ndk::ScopedAStatus setBandMode(int32_t serial, ar::network::RadioBandMode mode) override;
    ::ndk::ScopedAStatus setBarringPassword(int32_t serial, const std::string& facility,
                                            const std::string& oldPassword,
                                            const std::string& newPassword) override;
    ::ndk::ScopedAStatus setCdmaRoamingPreference(int32_t serial,
                                                  ar::network::CdmaRoamingType type) override;
    ::ndk::ScopedAStatus setCellInfoListRate(int32_t serial, int32_t rate) override;
    ::ndk::ScopedAStatus setIndicationFilter(int32_t serial, int32_t indicationFilter) override;
    ::ndk::ScopedAStatus setLinkCapacityReportingCriteria(
            int32_t serial, int32_t hysteresisMs, int32_t hysteresisDlKbps,
            int32_t hysteresisUlKbps, const std::vector<int32_t>& thresholdsDownlinkKbps,
            const std::vector<int32_t>& thresholdsUplinkKbps,
            ar::AccessNetwork accessNetwork) override;
    ::ndk::ScopedAStatus setLocationUpdates(int32_t serial, bool enable) override;
    ::ndk::ScopedAStatus setNetworkSelectionModeAutomatic(int32_t serial) override;
    ::ndk::ScopedAStatus setNetworkSelectionModeManual(int32_t serial,
                                                       const std::string& operatorNumeric,
                                                       ar::AccessNetwork ran) override;
    ::ndk::ScopedAStatus setNrDualConnectivityState(
            int32_t serial, ar::network::NrDualConnectivityState nrDualConnectivityState) override;
    ::ndk::ScopedAStatus setResponseFunctions(
            const std::shared_ptr<ar::network::IRadioNetworkResponse>& radioNetworkResponse,
            const std::shared_ptr<ar::network::IRadioNetworkIndication>& radioNetworkIndication) override;
    ::ndk::ScopedAStatus setSignalStrengthReportingCriteria(
            int32_t serial,
            const std::vector<ar::network::SignalThresholdInfo>& signalThresholdInfos) override;
    ::ndk::ScopedAStatus setSuppServiceNotifications(int32_t serial, bool enable) override;
    ::ndk::ScopedAStatus setSystemSelectionChannels(
            int32_t serial, bool specifyChannels,
            const std::vector<ar::network::RadioAccessSpecifier>& specifiers) override;
    ::ndk::ScopedAStatus startNetworkScan(int32_t serial,
                                          const ar::network::NetworkScanRequest& request) override;
    ::ndk::ScopedAStatus stopNetworkScan(int32_t serial) override;
    ::ndk::ScopedAStatus supplyNetworkDepersonalization(int32_t serial,
                                                        const std::string& netPin) override;
    ::ndk::ScopedAStatus setUsageSetting(int32_t serial,
                                         ar::network::UsageSetting usageSetting) override;
    ::ndk::ScopedAStatus getUsageSetting(int32_t serial) override;
    ::ndk::ScopedAStatus setEmergencyMode(int32_t serial,
                                          ar::network::EmergencyMode emcModeType) override;
    ::ndk::ScopedAStatus triggerEmergencyNetworkScan(
            int32_t serial,
            const ar::network::EmergencyNetworkScanTrigger& request) override;
    ::ndk::ScopedAStatus cancelEmergencyNetworkScan(int32_t serial, bool resetScan) override;
    ::ndk::ScopedAStatus exitEmergencyMode(int32_t serial) override;
    ::ndk::ScopedAStatus setNullCipherAndIntegrityEnabled(int32_t serial, bool enabled) override;
    ::ndk::ScopedAStatus isNullCipherAndIntegrityEnabled(int32_t serial) override;
    ::ndk::ScopedAStatus isN1ModeEnabled(int32_t serial) override;
    ::ndk::ScopedAStatus setN1ModeEnabled(int32_t serial, bool enable) override;
    ::ndk::ScopedAStatus isCellularIdentifierTransparencyEnabled(int32_t serial) override;
    ::ndk::ScopedAStatus setCellularIdentifierTransparencyEnabled(int32_t serial,
                                                                  bool enabled) override;
    ::ndk::ScopedAStatus setSecurityAlgorithmsUpdatedEnabled(int32_t serial,
                                                             bool enable) override;
    ::ndk::ScopedAStatus isSecurityAlgorithmsUpdatedEnabled(int32_t serial) override;
    ::ndk::ScopedAStatus setSatellitePlmn(int32_t serial,
                                          const std::vector<std::string>& carrierPlmnArray,
                                          const std::vector<std::string>& allSatellitePlmnArray) override;
    ::ndk::ScopedAStatus setSatelliteEnabledForCarrier(int32_t serial,
                                                       bool satelliteEnabled) override;
    ::ndk::ScopedAStatus isSatelliteEnabledForCarrier(int32_t serial) override;
    ::ndk::ScopedAStatus setSatelliteNetworkInfo(
            int32_t serial,
            const ar::network::SatelliteNetworkInfo& satelliteNetworkInfo) override;
    ::ndk::ScopedAStatus enablePrioritizedNetworkScan(
            int32_t serial,
            const ar::network::PrioritizedNetworkScanRequest& scanRequest) override;
    ::ndk::ScopedAStatus disablePrioritizedNetworkScan(int32_t serial) override;
    ::ndk::ScopedAStatus getSupportedNetworkAlertCategories(int32_t serial) override;

  private:
    std::shared_ptr<ShimContext> mCtx;
};

}  // namespace android::hardware::radio::shim
