#include "ShimContext.h"

#include <android-base/logging.h>

#include <algorithm>
#include <cstdio>
#include <thread>

namespace android::hardware::radio::shim {

void ShimContext::registerRequest(int32_t serial, RequestKind kind) {
    std::lock_guard<std::mutex> lock(mMutex);
    mSerialToKind[serial] = kind;
    if (mSerialToKind.size() > 4096) {
        // Serials from the framework are monotonically increasing; keep only
        // the most recent ones.
        int64_t maxSerial = serial;
        for (const auto& [s, k] : mSerialToKind) {
            maxSerial = std::max<int64_t>(maxSerial, s);
        }
        const int64_t cutoff = maxSerial - 4096;
        for (auto it = mSerialToKind.begin(); it != mSerialToKind.end();) {
            if (it->first < cutoff) {
                it = mSerialToKind.erase(it);
            } else {
                ++it;
            }
        }
    }
}

RequestKind ShimContext::popRequest(int32_t serial) {
    std::lock_guard<std::mutex> lock(mMutex);
    const auto it = mSerialToKind.find(serial);
    if (it == mSerialToKind.end()) return RequestKind::NONE;
    const RequestKind kind = it->second;
    mSerialToKind.erase(it);
    return kind;
}

void ShimContext::dropRequest(int32_t serial) {
    std::lock_guard<std::mutex> lock(mMutex);
    mSerialToKind.erase(serial);
}

std::string ShimContext::getIccId() {
    std::lock_guard<std::mutex> lock(mMutex);
    return mIccId;
}

void ShimContext::setIccId(const std::string& iccid) {
    std::lock_guard<std::mutex> lock(mMutex);
    mIccId = iccid;
}

int32_t ShimContext::nextInternalSerial() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mNextInternalSerial >= 0x7F000000) mNextInternalSerial = 0x40000000;
    return mNextInternalSerial++;
}

static std::string sanitizeRouteToken(const std::string& in) {
    std::string out;
    for (char c : in) {
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') ||
            c == '.' || c == ':' || c == '/' || c == '-') {
            out += c;
        }
    }
    return out;
}

void ShimContext::addDataRoutes(const std::string& gateway, const std::string& ifname) {
    const std::string gw = sanitizeRouteToken(gateway);
    const std::string dev = sanitizeRouteToken(ifname);
    if (gw.empty() || dev.empty()) return;
    LOG(INFO) << "adding data routes: gw=" << gw << " dev=" << dev;
    std::thread([gw, dev]() {
        for (int attempt = 0; attempt < 10; attempt++) {
            const std::string cmd = "ip route add default via " + gw + " dev " + dev +
                                    " table local_network 2>/dev/null; "
                                    "ip route add default via " + gw + " dev " + dev +
                                    " table rmnet_usb0 2>/dev/null; "
                                    "ip route add default via " + gw + " dev " + dev +
                                    " 2>/dev/null";
            const int rc = system(cmd.c_str());
            if (rc == 0) return;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }).detach();
}

void ShimContext::scheduleIccIdRetry() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mIccId.empty()) return;
        if (mIccIdRetries >= 8) return;
        mIccIdRetries++;
    }
    std::thread([this]() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        refreshIccId();
    }).detach();
}

void ShimContext::refreshIccId() {
    if (!radio) return;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mIccId.empty()) return;
    }
    // Two-step read of EF-ICCID (0x2FE2) in the MF (path "3F00"): first
    // GET RESPONSE to learn the file size, then READ BINARY.
    const int32_t serial = nextInternalSerial();
    registerRequest(serial, RequestKind::SIM_INTERNAL_READ_ICCID);
    R10::IccIo io;
    io.command = 0xC0;  // GET RESPONSE
    io.fileId = 0x2FE2;
    io.path = "3F00";
    io.p1 = 0;
    io.p2 = 0;
    io.p3 = 15;
    io.data = "";
    io.pin2 = "";
    io.aid = "";
    const auto status = radio->iccIOForApp(serial, io);
    if (!status.isOk()) {
        LOG(ERROR) << "internal EF-ICCID GET_RESPONSE failed";
        dropRequest(serial);
    }
}

ar::RadioResponseInfo ShimContext::responseInfo(int32_t serial, ar::RadioError error) {
    ar::RadioResponseInfo info;
    info.type = ar::RadioResponseType::SOLICITED;
    info.serial = serial;
    info.error = error;
    return info;
}

ar::RadioResponseInfo ShimContext::noError(int32_t serial) {
    return responseInfo(serial, ar::RadioError::NONE);
}

ar::RadioResponseInfo ShimContext::notSupported(int32_t serial) {
    return responseInfo(serial, ar::RadioError::REQUEST_NOT_SUPPORTED);
}

}  // namespace android::hardware::radio::shim
