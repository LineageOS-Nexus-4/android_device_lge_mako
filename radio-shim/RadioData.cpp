#include "RadioData.h"
#include "Structs.h"

#include <android-base/logging.h>

namespace android::hardware::radio::shim {

using ::ndk::ScopedAStatus;

ScopedAStatus RadioData::setResponseFunctions(
        const std::shared_ptr<ar::data::IRadioDataResponse>& response,
        const std::shared_ptr<ar::data::IRadioDataIndication>& indication) {
    mCtx->dataResponse = response;
    mCtx->dataIndication = indication;
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::responseAcknowledgement() {
    return ScopedAStatus::ok();
}

namespace {

// AccessNetwork -> V1.0 RadioTechnology for the legacy setupDataCall.
R10::RadioTechnology toHidlRadioTechnology(ar::AccessNetwork accessNetwork) {
    switch (accessNetwork) {
        case ar::AccessNetwork::GERAN:
            return R10::RadioTechnology::GSM;
        case ar::AccessNetwork::UTRAN:
            return R10::RadioTechnology::UMTS;
        case ar::AccessNetwork::EUTRAN:
            return R10::RadioTechnology::LTE;
        default:
            return R10::RadioTechnology::UNKNOWN;
    }
}

}  // namespace

ScopedAStatus RadioData::setupDataCall(int32_t serial, ar::AccessNetwork accessNetwork,
                                       const ar::data::DataProfileInfo& dataProfileInfo,
                                       bool roamingAllowed,
                                       ar::data::DataRequestReason,
                                       const std::vector<ar::data::LinkAddress>&,
                                       const std::vector<std::string>&, int32_t,
                                       const std::optional<ar::data::SliceInfo>&, bool) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::DATA_SETUP_DATA_CALL);
    // V1.1 ignores the reason and the pduSessionId; roamingAllowed is
    // forwarded as the V1.0 "isRoaming" flag.
    const auto status = mCtx->radio->setupDataCall(serial, toHidlRadioTechnology(accessNetwork),
                                                   toHidl(dataProfileInfo), false, roamingAllowed, false);
    if (!status.isOk()) {
        LOG(ERROR) << "setupDataCall HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::deactivateDataCall(int32_t serial, int32_t cid,
                                            ar::data::DataRequestReason reason) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::DATA_DEACTIVATE_DATA_CALL);
    const auto status = mCtx->radio->deactivateDataCall(serial, cid,
                                                        static_cast<int32_t>(reason));
    if (!status.isOk()) {
        LOG(ERROR) << "deactivateDataCall HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::getDataCallList(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::DATA_GET_DATA_CALL_LIST);
    const auto status = mCtx->radio->getDataCallList(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getDataCallList HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::setInitialAttachApn(
        int32_t serial, const std::optional<ar::data::DataProfileInfo>& dataProfileInfo) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::DATA_SET_INITIAL_ATTACH_APN);
    R10::DataProfileInfo hidl;
    if (dataProfileInfo.has_value()) {
        hidl = toHidl(*dataProfileInfo);
    } else {
        hidl.profileId = R10::DataProfileId::DEFAULT;
        hidl.apn = "";
        hidl.protocol = "IP";
        hidl.roamingProtocol = "IP";
        hidl.authType = R10::ApnAuthType::NO_PAP_NO_CHAP;
        hidl.type = R10::DataProfileInfoType::COMMON;
        hidl.enabled = true;
        hidl.supportedApnTypesBitmap = 0;
        hidl.bearerBitmap = 0;
    }
    // V1.0 also takes modemCognitive and isRoaming flags.
    const auto status = mCtx->radio->setInitialAttachApn(serial, hidl, false, false);
    if (!status.isOk()) {
        LOG(ERROR) << "setInitialAttachApn HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::setDataAllowed(int32_t serial, bool allow) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::DATA_SET_DATA_ALLOWED);
    const auto status = mCtx->radio->setDataAllowed(serial, allow);
    if (!status.isOk()) {
        LOG(ERROR) << "setDataAllowed HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::setDataProfile(int32_t serial,
                                        const std::vector<ar::data::DataProfileInfo>& profiles) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::DATA_SET_DATA_PROFILE);
    std::vector<R10::DataProfileInfo> hidl;
    for (const auto& p : profiles) hidl.push_back(toHidl(p));
    // V1.0 also takes an isRoaming flag.
    const auto status = mCtx->radio->setDataProfile(serial, hidl, false);
    if (!status.isOk()) {
        LOG(ERROR) << "setDataProfile HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

// ---------------------------------------------------------------------------
// Unsupported.
// ---------------------------------------------------------------------------

ScopedAStatus RadioData::allocatePduSessionId(int32_t serial) {
    if (mCtx->dataResponse) mCtx->dataResponse->allocatePduSessionIdResponse(
            mCtx->notSupported(serial), 0);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::cancelHandover(int32_t serial, int32_t) {
    if (mCtx->dataResponse) mCtx->dataResponse->cancelHandoverResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::getSlicingConfig(int32_t serial) {
    if (mCtx->dataResponse) mCtx->dataResponse->getSlicingConfigResponse(
            mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::releasePduSessionId(int32_t serial, int32_t) {
    if (mCtx->dataResponse) mCtx->dataResponse->releasePduSessionIdResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::setDataThrottling(int32_t serial, ar::data::DataThrottlingAction,
                                           int64_t) {
    if (mCtx->dataResponse) mCtx->dataResponse->setDataThrottlingResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::startHandover(int32_t serial, int32_t) {
    if (mCtx->dataResponse) mCtx->dataResponse->startHandoverResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::startKeepalive(int32_t serial, const ar::data::KeepaliveRequest&) {
    if (mCtx->dataResponse) mCtx->dataResponse->startKeepaliveResponse(
            mCtx->notSupported(serial), {});
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::stopKeepalive(int32_t serial, int32_t) {
    if (mCtx->dataResponse) mCtx->dataResponse->stopKeepaliveResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::setUserDataEnabled(int32_t serial, bool) {
    if (mCtx->dataResponse) mCtx->dataResponse->setUserDataEnabledResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::setUserDataRoamingEnabled(int32_t serial, bool) {
    if (mCtx->dataResponse) mCtx->dataResponse->setUserDataRoamingEnabledResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

ScopedAStatus RadioData::notifyImsDataNetwork(int32_t serial,
                                              const ar::data::ImsDataNetworkInfo&) {
    if (mCtx->dataResponse) mCtx->dataResponse->notifyImsDataNetworkResponse(
            mCtx->notSupported(serial));
    return ScopedAStatus::ok();
}

}  // namespace android::hardware::radio::shim
