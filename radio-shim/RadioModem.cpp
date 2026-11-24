#include "RadioModem.h"

#include <android-base/logging.h>

namespace android::hardware::radio::shim {

using ::ndk::ScopedAStatus;

ScopedAStatus RadioModem::setResponseFunctions(
        const std::shared_ptr<ar::modem::IRadioModemResponse>& response,
        const std::shared_ptr<ar::modem::IRadioModemIndication>& indication) {
    mCtx->setModemResponseFunctions(response, indication);
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::responseAcknowledgement() {
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::setRadioPower(int32_t serial, bool powerOn, bool forEmergencyCall,
                                        bool preferredForEmergencyCall) {
    // V1.1 has no 4-arg setRadioPower; ignore the emergency flags.
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MODEM_SET_RADIO_POWER);
    const auto status = mCtx->radio->setRadioPower(serial, powerOn);
    if (!status.isOk()) {
        LOG(ERROR) << "setRadioPower HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::getImei(int32_t serial) {
    // The V1.1 HAL has no getImei; the device identity carries the IMEI.
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MODEM_GET_IMEI);
    const auto status = mCtx->radio->getDeviceIdentity(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getDeviceIdentity HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::getDeviceIdentity(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MODEM_GET_DEVICE_IDENTITY);
    const auto status = mCtx->radio->getDeviceIdentity(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getDeviceIdentity HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::getBasebandVersion(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MODEM_GET_BASEBAND_VERSION);
    const auto status = mCtx->radio->getBasebandVersion(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getBasebandVersion HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::getRadioCapability(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MODEM_GET_RADIO_CAPABILITY);
    const auto status = mCtx->radio->getRadioCapability(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getRadioCapability HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::setRadioCapability(int32_t serial,
                                             const ar::modem::RadioCapability& rc) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MODEM_SET_RADIO_CAPABILITY);
    R10::RadioCapability hidl;
    hidl.session = rc.session;
    hidl.phase = static_cast<R10::RadioCapabilityPhase>(rc.phase);
    hidl.raf = rc.raf;
    hidl.logicalModemUuid = rc.logicalModemUuid;
    hidl.status = static_cast<R10::RadioCapabilityStatus>(rc.status);
    const auto status = mCtx->radio->setRadioCapability(serial, hidl);
    if (!status.isOk()) {
        LOG(ERROR) << "setRadioCapability HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::getHardwareConfig(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MODEM_GET_HARDWARE_CONFIG);
    const auto status = mCtx->radio->getHardwareConfig(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "getHardwareConfig HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::getModemStackStatus(int32_t serial) {
    // V1.1 has no getModemStackStatus; the modem is always enabled.
    if (const auto response = mCtx->getModemResponse()) {
        response->getModemStackStatusResponse(mCtx->noError(serial), true);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::requestShutdown(int32_t serial) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MODEM_REQUEST_SHUTDOWN);
    const auto status = mCtx->radio->requestShutdown(serial);
    if (!status.isOk()) {
        LOG(ERROR) << "requestShutdown HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::sendDeviceState(int32_t serial, ar::modem::DeviceStateType deviceStateType,
                                          bool state) {
    if (!mCtx->radio) return ScopedAStatus::ok();
    mCtx->registerRequest(serial, RequestKind::MODEM_SEND_DEVICE_STATE);
    const auto status = mCtx->radio->sendDeviceState(
            serial, static_cast<R10::DeviceStateType>(deviceStateType), state);
    if (!status.isOk()) {
        LOG(ERROR) << "sendDeviceState HIDL call failed";
        mCtx->dropRequest(serial);
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::enableModem(int32_t serial, bool) {
    if (const auto response = mCtx->getModemResponse()) {
        response->enableModemResponse(mCtx->notSupported(serial));
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::getModemActivityInfo(int32_t serial) {
    if (const auto response = mCtx->getModemResponse()) {
        response->getModemActivityInfoResponse(mCtx->notSupported(serial), {});
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::nvReadItem(int32_t serial, ar::modem::NvItem) {
    if (const auto response = mCtx->getModemResponse()) {
        response->nvReadItemResponse(mCtx->notSupported(serial), "");
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::nvResetConfig(int32_t serial, ar::modem::ResetNvType) {
    if (const auto response = mCtx->getModemResponse()) {
        response->nvResetConfigResponse(mCtx->notSupported(serial));
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::nvWriteCdmaPrl(int32_t serial, const std::vector<uint8_t>&) {
    if (const auto response = mCtx->getModemResponse()) {
        response->nvWriteCdmaPrlResponse(mCtx->notSupported(serial));
    }
    return ScopedAStatus::ok();
}

ScopedAStatus RadioModem::nvWriteItem(int32_t serial, const ar::modem::NvWriteItem&) {
    if (const auto response = mCtx->getModemResponse()) {
        response->nvWriteItemResponse(mCtx->notSupported(serial));
    }
    return ScopedAStatus::ok();
}

}  // namespace android::hardware::radio::shim
