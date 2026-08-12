/*
 * AIDL <-> HIDL V1.0/V1.1 conversion helpers for the mako radio shim.
 */
#pragma once

#include <android/hardware/radio/1.0/IRadio.h>
#include <android/hardware/radio/1.1/IRadio.h>

#include <aidl/android/hardware/radio/RadioError.h>
#include <aidl/android/hardware/radio/RadioIndicationType.h>
#include <aidl/android/hardware/radio/RadioResponseInfo.h>
#include <aidl/android/hardware/radio/RadioResponseType.h>
#include <aidl/android/hardware/radio/RadioTechnology.h>
#include <aidl/android/hardware/radio/data/DataProfileInfo.h>
#include <aidl/android/hardware/radio/data/PdpProtocolType.h>
#include <aidl/android/hardware/radio/data/SetupDataCallResult.h>
#include <aidl/android/hardware/radio/messaging/GsmSmsMessage.h>
#include <aidl/android/hardware/radio/messaging/SendSmsResult.h>
#include <aidl/android/hardware/radio/messaging/SmsWriteArgs.h>
#include <aidl/android/hardware/radio/modem/RadioCapability.h>
#include <aidl/android/hardware/radio/network/CellIdentity.h>
#include <aidl/android/hardware/radio/network/RegStateResult.h>
#include <aidl/android/hardware/radio/network/SignalStrength.h>
#include <aidl/android/hardware/radio/sim/AppStatus.h>
#include <aidl/android/hardware/radio/sim/CardStatus.h>
#include <aidl/android/hardware/radio/sim/IccIo.h>
#include <aidl/android/hardware/radio/sim/IccIoResult.h>
#include <aidl/android/hardware/radio/voice/Call.h>

#include <string>
#include <vector>

namespace android::hardware::radio::shim {

namespace R10 = ::android::hardware::radio::V1_0;
namespace ar = ::aidl::android::hardware::radio;

// Common
ar::RadioResponseInfo toAidl(const R10::RadioResponseInfo& info);

// modem
ar::modem::RadioCapability toAidl(const R10::RadioCapability& rc);

// network
ar::network::SignalStrength toAidl(const R10::SignalStrength& ss);
ar::network::RegStateResult toAidl(const R10::VoiceRegStateResult& r);
ar::network::RegStateResult toAidl(const R10::DataRegStateResult& r);
ar::network::CellIdentity toAidl(const R10::CellIdentity& ci);
ar::network::OperatorInfo toAidl(const R10::OperatorInfo& op);
int32_t toAidlAllowedNetworkTypesBitmap(int32_t preferredNetworkType);
int32_t toHidlPreferredNetworkType(int32_t networkTypeBitmap);

// sim
ar::sim::CardStatus toAidl(const R10::CardStatus& cs);
ar::sim::AppStatus toAidl(const R10::AppStatus& app);
R10::IccIo toHidl(const ar::sim::IccIo& io);
ar::sim::IccIoResult toAidl(const R10::IccIoResult& r);

// voice
ar::voice::Call toAidl(const R10::Call& call);

// messaging
ar::messaging::SendSmsResult toAidl(const R10::SendSmsResult& r);
R10::GsmSmsMessage toHidl(const ar::messaging::GsmSmsMessage& m);
R10::SmsWriteArgs toHidl(const ar::messaging::SmsWriteArgs& w);

// data
ar::data::SetupDataCallResult toAidl(const R10::SetupDataCallResult& r);
R10::DataProfileInfo toHidl(const ar::data::DataProfileInfo& p);
ar::data::PdpProtocolType toAidlPdpType(const std::string& type);
std::string toHidlPdpTypeString(ar::data::PdpProtocolType type);

// Splits a space-delimited string from the V1.0 SetupDataCallResult into a
// vector of AIDL LinkAddress (address, prefix length in addressProperties).
std::vector<ar::data::LinkAddress> toAidlLinkAddresses(const std::string& spaceDelimited);

}  // namespace android::hardware::radio::shim
