#include "ShimContext.h"

#include <android-base/logging.h>

#include <algorithm>

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
