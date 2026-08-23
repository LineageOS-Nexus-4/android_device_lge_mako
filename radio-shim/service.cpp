/*
 * mako radio shim: publishes the AIDL radio services the AOSP 17 framework
 * expects and translates them to the V1.1 HIDL IRadio registered by rild.
 */
#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <hidl/HidlSupport.h>

#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>
#include <vector>

#include "IndicationHandler.h"
#include "RadioConfig.h"
#include "RadioData.h"
#include "RadioMessaging.h"
#include "RadioModem.h"
#include "RadioNetwork.h"
#include "RadioSim.h"
#include "RadioVoice.h"
#include "ResponseHandler.h"
#include "ShimContext.h"

namespace android::hardware::radio::shim {

using ::android::hardware::hidl_death_recipient;
using ::android::sp;

static std::vector<std::shared_ptr<::ndk::ICInterface>> gPublishedHals;

namespace {

struct RadioDeathRecipient : public hidl_death_recipient {
    void serviceDied(uint64_t, const wp<::android::hidl::base::V1_0::IBase>&) override {
        // rild was restarted (boot-time UIM warm-up or watchdog); exit cleanly
        // so init respawns the shim and re-binds to the new IRadio without a
        // tombstone.
        LOG(WARNING) << "HIDL IRadio (rild) died; exiting so init can restart the shim";
        exit(0);
    }
};

template <typename T>
void publishRadioHal(std::shared_ptr<ShimContext> ctx, const std::string& descriptor,
                     const std::string& slot) {
    const auto instance = std::string(descriptor) + "/" + slot;
    if (!AServiceManager_isDeclared(instance.c_str())) {
        LOG(INFO) << instance << " is not declared in VINTF (this may be intentional)";
        return;
    }
    LOG(INFO) << "Publishing " << instance;
    auto hal = ::ndk::SharedRefBase::make<T>(ctx);
    gPublishedHals.push_back(hal);
    const auto status = AServiceManager_addService(hal->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK) << "Failed to publish " << instance;
}

void publishRadioConfig(std::shared_ptr<ShimContext> ctx) {
    const auto instance = std::string(ar::config::IRadioConfig::descriptor) + "/default";
    if (!AServiceManager_isDeclared(instance.c_str())) {
        LOG(INFO) << instance << " is not declared in VINTF (this may be intentional)";
        return;
    }
    LOG(INFO) << "Publishing " << instance;
    auto hal = ::ndk::SharedRefBase::make<RadioConfig>(ctx);
    gPublishedHals.push_back(hal);
    const auto status = AServiceManager_addService(hal->asBinder().get(), instance.c_str());
    CHECK_EQ(status, STATUS_OK) << "Failed to publish " << instance;
}

}  // namespace

static void main() {
    base::InitLogging(nullptr, base::LogdLogger(base::RADIO));
    base::SetDefaultTag("radio-shim");
    LOG(INFO) << "mako radio shim starting";

    auto ctx = std::make_shared<ShimContext>();

    // Bind to the HIDL V1.1 IRadio registered in-process by rild. rild is
    // gated on vendor.radio.modem_ready, and a deterministic restart a few
    // seconds later (SIM warm-up) leaves a window with no IRadio; keep
    // retrying instead of crashing so init doesn't respawn us in a loop.
    sp<R11::IRadio> radio = nullptr;
    while (radio == nullptr) {
        radio = R11::IRadio::getService("slot1");
        if (radio == nullptr) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    LOG(INFO) << "Bound to V1.1 IRadio/slot1";
    ctx->radio = radio;

    static sp<RadioDeathRecipient> deathRecipient = new RadioDeathRecipient();
    const auto deathStatus = radio->linkToDeath(deathRecipient, 0);
    if (!deathStatus.isOk()) {
        LOG(WARNING) << "linkToDeath failed: " << deathStatus.description();
    }

    ctx->hidlResponse = new ResponseHandler(ctx);
    ctx->hidlIndication = new IndicationHandler(ctx);

    // Register the shim's own response/indication callbacks with rild. All
    // unsolicited indications and solicited responses arrive here and are
    // routed to the AIDL callbacks set by the framework.
    const auto respStatus = radio->setResponseFunctions(ctx->hidlResponse, ctx->hidlIndication);
    if (!respStatus.isOk()) {
        LOG(FATAL) << "setResponseFunctions failed: " << respStatus.description();
        return;
    }
    LOG(INFO) << "Registered response/indication callbacks with rild";

    // Publish the AIDL radio services.
    publishRadioConfig(ctx);
    publishRadioHal<RadioModem>(ctx, ar::modem::IRadioModem::descriptor, "slot1");
    publishRadioHal<RadioSim>(ctx, ar::sim::IRadioSim::descriptor, "slot1");
    publishRadioHal<RadioNetwork>(ctx, ar::network::IRadioNetwork::descriptor, "slot1");
    publishRadioHal<RadioVoice>(ctx, ar::voice::IRadioVoice::descriptor, "slot1");
    publishRadioHal<RadioMessaging>(ctx, ar::messaging::IRadioMessaging::descriptor, "slot1");
    publishRadioHal<RadioData>(ctx, ar::data::IRadioData::descriptor, "slot1");

    // Populate the SIM ICCID so the framework can match the subscription.
    ctx->refreshIccId();

    LOG(INFO) << "mako radio shim is operational";
    ABinderProcess_joinThreadPool();
}

}  // namespace android::hardware::radio::shim

int main() {
    android::hardware::radio::shim::main();
    return EXIT_FAILURE;  // should not be reached
}
