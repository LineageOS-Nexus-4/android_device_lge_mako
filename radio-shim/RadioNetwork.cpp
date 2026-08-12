#include "RadioNetwork.h"
#include "Structs.h"

#include <android-base/logging.h>

namespace android::hardware::radio::shim {

using ::ndk::ScopedAStatus;

ScopedAStatus RadioNetwork::setResponseFunctions(
        const std::shared_ptr<ar::network::IRadioNetworkResponse>& response,
        const std::shared_ptr<ar::network::IRadioNetworkIndication>& indication) {
    mCtx->networkResponse = response;
    mCtx->networkIndication = indication;
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::responseAcknowledgement() {
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getSignalStrength(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_GET_SIGNAL_STRENGTH);
    const auto status = mCtx->radio->getSignalStrength(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getSignalStrength HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getVoiceRegistrationState(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_GET_VOICE_REGISTRATION_STATE);
    const auto status = mCtx->radio->getVoiceRegistrationState(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getVoiceRegistrationState HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getDataRegistrationState(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_GET_DATA_REGISTRATION_STATE);
    const auto status = mCtx->radio->getDataRegistrationState(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getDataRegistrationState HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getOperator(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_GET_OPERATOR);
    const auto status = mCtx->radio->getOperator(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getOperator HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getNetworkSelectionMode(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_GET_NETWORK_SELECTION_MODE);
    const auto status = mCtx->radio->getNetworkSelectionMode(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getNetworkSelectionMode HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setNetworkSelectionModeAutomatic(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_SET_NETWORK_SELECTION_MODE_AUTOMATIC);
    const auto status = mCtx->radio->setNetworkSelectionModeAutomatic(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "setNetworkSelectionModeAutomatic HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setNetworkSelectionModeManual(int32_t serial,
                                                          const std::string& operatorNumeric,
                                                          ar::AccessNetwork) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_SET_NETWORK_SELECTION_MODE_MANUAL);
    const auto status = mCtx->radio->setNetworkSelectionModeManual(serial, operatorNumeric);
    if (!status.isOk()) {
        LOG(ERROR) << "setNetworkSelectionModeManual HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getVoiceRadioTechnology(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_GET_VOICE_RADIO_TECHNOLOGY);
    const auto status = mCtx->radio->getVoiceRadioTechnology(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getVoiceRadioTechnology HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getAllowedNetworkTypesBitmap(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_GET_PREFERRED_NETWORK_TYPE);
    const auto status = mCtx->radio->getPreferredNetworkType(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getPreferredNetworkType HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setAllowedNetworkTypesBitmap(int32_t serial,
                                                         int32_t networkTypeBitmap) {
    LOG(INFO) << "setAllowedNetworkTypesBitmap serial=" << serial
              << " bitmap=0x" << std::hex << networkTypeBitmap << std::dec
              << " -> preferred=" << toHidlPreferredNetworkType(networkTypeBitmap);
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_SET_PREFERRED_NETWORK_TYPE);
    const auto status = mCtx->radio->setPreferredNetworkType(
            serial, static_cast<R10::PreferredNetworkType>(toHidlPreferredNetworkType(
                            networkTypeBitmap)));
    if (!status.isOk()) {
        LOG(ERROR) << "setPreferredNetworkType HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setIndicationFilter(int32_t serial, int32_t indicationFilter) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_SET_INDICATION_FILTER);
    const auto status = mCtx->radio->setIndicationFilter(serial, indicationFilter);
    if (!status.isOk()) {
        LOG(ERROR) << "setIndicationFilter HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setLocationUpdates(int32_t serial, bool enable) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_SET_LOCATION_UPDATES);
    const auto status = mCtx->radio->setLocationUpdates(serial, enable);
    if (!status.isOk()) {
        LOG(ERROR) << "setLocationUpdates HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getAvailableNetworks(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_GET_AVAILABLE_NETWORKS);
    const auto status = mCtx->radio->getAvailableNetworks(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getAvailableNetworks HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setSuppServiceNotifications(int32_t serial, bool enable) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::NET_SET_SUPP_SERVICE_NOTIFICATIONS);
    const auto status = mCtx->radio->setSuppServiceNotifications(serial, enable);
    if (!status.isOk()) {
        LOG(ERROR) << "setSuppServiceNotifications HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// Unsupported: respond REQUEST_NOT_SUPPORTED.
// ---------------------------------------------------------------------------

ScopedAStatus RadioNetwork::getAvailableBandModes(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->getAvailableBandModesResponse(
            mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getBarringInfo(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->getBarringInfoResponse(
            mCtx->notSupported(serial), {}, {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getCdmaRoamingPreference(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->getCdmaRoamingPreferenceResponse(
            mCtx->notSupported(serial), ar::network::CdmaRoamingType::HOME_NETWORK);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getCellInfoList(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->getCellInfoListResponse(
            mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getImsRegistrationState(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->getImsRegistrationStateResponse(
            mCtx->notSupported(serial), false, ar::RadioTechnologyFamily::THREE_GPP);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getSystemSelectionChannels(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->getSystemSelectionChannelsResponse(mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::isNrDualConnectivityEnabled(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->isNrDualConnectivityEnabledResponse(
            mCtx->notSupported(serial), false);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setBandMode(int32_t serial, ar::network::RadioBandMode) {
    if (mCtx->networkResponse) mCtx->networkResponse->setBandModeResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setBarringPassword(int32_t serial, const std::string&,
                                               const std::string&, const std::string&) {
    if (mCtx->networkResponse) mCtx->networkResponse->setBarringPasswordResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setCdmaRoamingPreference(int32_t serial,
                                                     ar::network::CdmaRoamingType) {
    if (mCtx->networkResponse) mCtx->networkResponse->setCdmaRoamingPreferenceResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setCellInfoListRate(int32_t serial, int32_t) {
    if (mCtx->networkResponse) mCtx->networkResponse->setCellInfoListRateResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setLinkCapacityReportingCriteria(
        int32_t serial, int32_t, int32_t, int32_t, const std::vector<int32_t>&,
        const std::vector<int32_t>&, ar::AccessNetwork) {
    if (mCtx->networkResponse) mCtx->networkResponse->setLinkCapacityReportingCriteriaResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setNrDualConnectivityState(
        int32_t serial, ar::network::NrDualConnectivityState) {
    if (mCtx->networkResponse) mCtx->networkResponse->setNrDualConnectivityStateResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setSignalStrengthReportingCriteria(
        int32_t serial, const std::vector<ar::network::SignalThresholdInfo>&) {
    if (mCtx->networkResponse) mCtx->networkResponse->setSignalStrengthReportingCriteriaResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setSystemSelectionChannels(
        int32_t serial, bool, const std::vector<ar::network::RadioAccessSpecifier>&) {
    if (mCtx->networkResponse) mCtx->networkResponse->setSystemSelectionChannelsResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::startNetworkScan(int32_t serial,
                                             const ar::network::NetworkScanRequest&) {
    if (mCtx->networkResponse) mCtx->networkResponse->startNetworkScanResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::stopNetworkScan(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->stopNetworkScanResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::supplyNetworkDepersonalization(int32_t serial, const std::string&) {
    if (mCtx->networkResponse) mCtx->networkResponse->supplyNetworkDepersonalizationResponse(
            mCtx->notSupported(serial), 0);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setUsageSetting(int32_t serial, ar::network::UsageSetting) {
    if (mCtx->networkResponse) mCtx->networkResponse->setUsageSettingResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getUsageSetting(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->getUsageSettingResponse(
            mCtx->notSupported(serial), ar::network::UsageSetting::VOICE_CENTRIC);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setEmergencyMode(int32_t serial, ar::network::EmergencyMode) {
    if (mCtx->networkResponse) mCtx->networkResponse->setEmergencyModeResponse(
            mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::triggerEmergencyNetworkScan(
        int32_t serial, const ar::network::EmergencyNetworkScanTrigger&) {
    if (mCtx->networkResponse) mCtx->networkResponse->triggerEmergencyNetworkScanResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::cancelEmergencyNetworkScan(int32_t serial, bool) {
    if (mCtx->networkResponse) mCtx->networkResponse->cancelEmergencyNetworkScanResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::exitEmergencyMode(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->exitEmergencyModeResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}


ScopedAStatus RadioNetwork::setNullCipherAndIntegrityEnabled(int32_t serial, bool) {
    if (mCtx->networkResponse) mCtx->networkResponse->setNullCipherAndIntegrityEnabledResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::isNullCipherAndIntegrityEnabled(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->isNullCipherAndIntegrityEnabledResponse(
            mCtx->notSupported(serial), false);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::isN1ModeEnabled(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->isN1ModeEnabledResponse(
            mCtx->notSupported(serial), false);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setN1ModeEnabled(int32_t serial, bool) {
    if (mCtx->networkResponse) mCtx->networkResponse->setN1ModeEnabledResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::isCellularIdentifierTransparencyEnabled(int32_t serial) {
    if (mCtx->networkResponse)
        mCtx->networkResponse->isCellularIdentifierTransparencyEnabledResponse(
                mCtx->notSupported(serial), false);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setCellularIdentifierTransparencyEnabled(int32_t serial, bool) {
    if (mCtx->networkResponse)
        mCtx->networkResponse->setCellularIdentifierTransparencyEnabledResponse(
                mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setSecurityAlgorithmsUpdatedEnabled(int32_t serial, bool) {
    if (mCtx->networkResponse) mCtx->networkResponse->setSecurityAlgorithmsUpdatedEnabledResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::isSecurityAlgorithmsUpdatedEnabled(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->isSecurityAlgorithmsUpdatedEnabledResponse(
            mCtx->notSupported(serial), false);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setSatellitePlmn(int32_t serial, const std::vector<std::string>&,
                                             const std::vector<std::string>&) {
    if (mCtx->networkResponse) mCtx->networkResponse->setSatellitePlmnResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setSatelliteEnabledForCarrier(int32_t serial, bool) {
    if (mCtx->networkResponse) mCtx->networkResponse->setSatelliteEnabledForCarrierResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::isSatelliteEnabledForCarrier(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->isSatelliteEnabledForCarrierResponse(
            mCtx->notSupported(serial), false);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::setSatelliteNetworkInfo(int32_t serial,
                                                    const ar::network::SatelliteNetworkInfo&) {
    if (mCtx->networkResponse) mCtx->networkResponse->setSatelliteNetworkInfoResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::enablePrioritizedNetworkScan(
        int32_t serial, const ar::network::PrioritizedNetworkScanRequest&) {
    if (mCtx->networkResponse) mCtx->networkResponse->enablePrioritizedNetworkScanResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::disablePrioritizedNetworkScan(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->disablePrioritizedNetworkScanResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioNetwork::getSupportedNetworkAlertCategories(int32_t serial) {
    if (mCtx->networkResponse) mCtx->networkResponse->getSupportedNetworkAlertCategoriesResponse(
            mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

}  // namespace android::hardware::radio::shim
