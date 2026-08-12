#pragma once

#include <aidl/android/hardware/radio/data/BnRadioData.h>

#include "ShimContext.h"

namespace android::hardware::radio::shim {

class RadioData : public ar::data::BnRadioData {
  public:
    explicit RadioData(std::shared_ptr<ShimContext> ctx) : mCtx(std::move(ctx)) {}

    ::ndk::ScopedAStatus allocatePduSessionId(int32_t serial) override;
    ::ndk::ScopedAStatus cancelHandover(int32_t serial, int32_t callId) override;
    ::ndk::ScopedAStatus deactivateDataCall(int32_t serial, int32_t cid,
                                             ar::data::DataRequestReason reason) override;
    ::ndk::ScopedAStatus getDataCallList(int32_t serial) override;
    ::ndk::ScopedAStatus getSlicingConfig(int32_t serial) override;
    ::ndk::ScopedAStatus releasePduSessionId(int32_t serial, int32_t id) override;
    ::ndk::ScopedAStatus responseAcknowledgement() override;
    ::ndk::ScopedAStatus setDataAllowed(int32_t serial, bool allow) override;
    ::ndk::ScopedAStatus setDataProfile(int32_t serial,
                                        const std::vector<ar::data::DataProfileInfo>& profiles) override;
    ::ndk::ScopedAStatus setDataThrottling(int32_t serial,
                                           ar::data::DataThrottlingAction dataThrottlingAction,
                                           int64_t completionDurationMillis) override;
    ::ndk::ScopedAStatus setInitialAttachApn(
            int32_t serial, const std::optional<ar::data::DataProfileInfo>& dataProfileInfo) override;
    ::ndk::ScopedAStatus setResponseFunctions(
            const std::shared_ptr<ar::data::IRadioDataResponse>& radioDataResponse,
            const std::shared_ptr<ar::data::IRadioDataIndication>& radioDataIndication) override;
    ::ndk::ScopedAStatus setupDataCall(int32_t serial, ar::AccessNetwork accessNetwork,
                                       const ar::data::DataProfileInfo& dataProfileInfo,
                                       bool roamingAllowed, ar::data::DataRequestReason reason,
                                       const std::vector<ar::data::LinkAddress>& addresses,
                                       const std::vector<std::string>& dnses,
                                       int32_t pduSessionId,
                                       const std::optional<ar::data::SliceInfo>& sliceInfo,
                                       bool matchAllRuleAllowed) override;
    ::ndk::ScopedAStatus startHandover(int32_t serial, int32_t callId) override;
    ::ndk::ScopedAStatus startKeepalive(int32_t serial,
                                        const ar::data::KeepaliveRequest& keepalive) override;
    ::ndk::ScopedAStatus stopKeepalive(int32_t serial, int32_t sessionHandle) override;
    ::ndk::ScopedAStatus setUserDataEnabled(int32_t serial, bool enabled) override;
    ::ndk::ScopedAStatus setUserDataRoamingEnabled(int32_t serial, bool enabled) override;
    ::ndk::ScopedAStatus notifyImsDataNetwork(int32_t serial,
                                              const ar::data::ImsDataNetworkInfo& info) override;

  private:
    std::shared_ptr<ShimContext> mCtx;
};

}  // namespace android::hardware::radio::shim
