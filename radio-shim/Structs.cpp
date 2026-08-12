#include "Structs.h"

#include <aidl/android/hardware/radio/network/CellIdentityGsm.h>
#include <aidl/android/hardware/radio/network/CellIdentityLte.h>
#include <aidl/android/hardware/radio/network/CellIdentityTdscdma.h>
#include <aidl/android/hardware/radio/network/CellIdentityWcdma.h>
#include <aidl/android/hardware/radio/network/OperatorInfo.h>
#include <aidl/android/hardware/radio/network/RegState.h>
#include <aidl/android/hardware/radio/network/RegistrationFailCause.h>
#include <aidl/android/hardware/radio/network/SignalStrength.h>
#include <aidl/android/hardware/radio/config/MultipleEnabledProfilesMode.h>
#include <aidl/android/hardware/radio/config/SlotPortMapping.h>
#include <aidl/android/hardware/radio/sim/PersoSubstate.h>
#include <aidl/android/hardware/radio/sim/PinState.h>

#include <climits>
#include <cstring>

namespace android::hardware::radio::shim {

ar::RadioResponseInfo toAidl(const R10::RadioResponseInfo& info) {
    ar::RadioResponseInfo out;
    out.type = static_cast<ar::RadioResponseType>(info.type);
    out.serial = info.serial;
    out.error = static_cast<ar::RadioError>(info.error);
    return out;
}

// ---------------------------------------------------------------------------
// modem
// ---------------------------------------------------------------------------

ar::modem::RadioCapability toAidl(const R10::RadioCapability& rc) {
    ar::modem::RadioCapability out;
    out.session = rc.session;
    out.phase = static_cast<int32_t>(rc.phase);
    out.raf = rc.raf;
    out.logicalModemUuid = rc.logicalModemUuid;
    out.status = static_cast<int32_t>(rc.status);
    return out;
}

// ---------------------------------------------------------------------------
// network
// ---------------------------------------------------------------------------

ar::network::SignalStrength toAidl(const R10::SignalStrength& ss) {
    ar::network::SignalStrength out;
    // mako is a GSM/WCDMA device; only the V1.0 gw measurements are real.
    // The QC RIL leaves the other RAT fields at 0 when unmeasured, and the
    // AIDL components treat any non-INT_MAX value as a valid measurement,
    // which would make an empty component the signal primary and report
    // level 0. Mark all other RATs invalid.
    out.gsm.signalStrength = ss.gw.signalStrength;
    out.gsm.bitErrorRate = ss.gw.bitErrorRate;
    out.gsm.timingAdvance = ss.gw.timingAdvance;
    out.cdma.dbm = INT_MAX;
    out.cdma.ecio = INT_MAX;
    out.evdo.dbm = INT_MAX;
    out.evdo.ecio = INT_MAX;
    out.evdo.signalNoiseRatio = INT_MAX;
    out.lte.signalStrength = INT_MAX;
    out.lte.rsrp = INT_MAX;
    out.lte.rsrq = INT_MAX;
    out.lte.rssnr = INT_MAX;
    out.lte.cqi = INT_MAX;
    out.lte.timingAdvance = INT_MAX;
    out.tdscdma.signalStrength = INT_MAX;
    out.tdscdma.bitErrorRate = INT_MAX;
    out.tdscdma.rscp = INT_MAX;
    // V1.0 has no WCDMA measurements; mark all fields invalid.
    out.wcdma.signalStrength = INT_MAX;
    out.wcdma.bitErrorRate = INT_MAX;
    out.wcdma.rscp = INT_MAX;
    out.wcdma.ecno = INT_MAX;
    // NR: the AIDL defaults leave ssRsrq/ssSinr/etc at 0, which the
    // framework treats as a valid measurement (CellSignalStrengthNr.isValid
    // compares against the all-UNAVAILABLE default) and would then report
    // as the signal primary with level 0. Mark every NR field unavailable.
    out.nr.ssRsrp = INT_MAX;
    out.nr.ssRsrq = INT_MAX;
    out.nr.ssSinr = INT_MAX;
    out.nr.csiRsrp = INT_MAX;
    out.nr.csiRsrq = INT_MAX;
    out.nr.csiSinr = INT_MAX;
    out.nr.csiCqiTableIndex = INT_MAX;
    out.nr.csiCqiReport = {};
    out.nr.timingAdvance = INT_MAX;
    return out;
}

namespace {

ar::network::OperatorInfo emptyOperatorInfo() {
    ar::network::OperatorInfo op;
    op.alphaLong = "";
    op.alphaShort = "";
    op.operatorNumeric = "";
    op.status = 0;
    return op;
}

}  // namespace

ar::network::CellIdentity toAidl(const R10::CellIdentity& ci) {
    ar::network::CellIdentity out;
    switch (ci.cellInfoType) {
        case R10::CellInfoType::GSM: {
            if (!ci.cellIdentityGsm.size() == 0) {
                const R10::CellIdentityGsm& g = ci.cellIdentityGsm[0];
                ar::network::CellIdentityGsm gsm;
                gsm.mcc = g.mcc;
                gsm.mnc = g.mnc;
                gsm.lac = g.lac;
                gsm.cid = g.cid;
                gsm.arfcn = g.arfcn;
                gsm.bsic = g.bsic;
                gsm.operatorNames = emptyOperatorInfo();
                out.set<ar::network::CellIdentity::Tag::gsm>(gsm);
            }
            break;
        }
        case R10::CellInfoType::WCDMA: {
            if (!ci.cellIdentityWcdma.size() == 0) {
                const R10::CellIdentityWcdma& w = ci.cellIdentityWcdma[0];
                ar::network::CellIdentityWcdma wcdma;
                wcdma.mcc = w.mcc;
                wcdma.mnc = w.mnc;
                wcdma.lac = w.lac;
                wcdma.cid = w.cid;
                wcdma.psc = w.psc;
                wcdma.uarfcn = w.uarfcn;
                wcdma.operatorNames = emptyOperatorInfo();
                out.set<ar::network::CellIdentity::Tag::wcdma>(wcdma);
            }
            break;
        }
        case R10::CellInfoType::LTE: {
            if (!ci.cellIdentityLte.size() == 0) {
                const R10::CellIdentityLte& l = ci.cellIdentityLte[0];
                ar::network::CellIdentityLte lte;
                lte.mcc = l.mcc;
                lte.mnc = l.mnc;
                lte.ci = l.ci;
                lte.pci = l.pci;
                lte.tac = l.tac;
                lte.earfcn = l.earfcn;
                lte.bandwidth = INT_MAX;
                lte.operatorNames = emptyOperatorInfo();
                out.set<ar::network::CellIdentity::Tag::lte>(lte);
            }
            break;
        }
        case R10::CellInfoType::TD_SCDMA: {
            if (!ci.cellIdentityTdscdma.size() == 0) {
                const R10::CellIdentityTdscdma& t = ci.cellIdentityTdscdma[0];
                ar::network::CellIdentityTdscdma tdscdma;
                tdscdma.mcc = t.mcc;
                tdscdma.mnc = t.mnc;
                tdscdma.lac = t.lac;
                tdscdma.cid = t.cid;
                tdscdma.cpid = t.cpid;
                tdscdma.uarfcn = 0;
                tdscdma.operatorNames = emptyOperatorInfo();
                out.set<ar::network::CellIdentity::Tag::tdscdma>(tdscdma);
            }
            break;
        }
        default:
            break;
    }
    return out;
}

ar::network::RegStateResult toAidl(const R10::VoiceRegStateResult& r) {
    ar::network::RegStateResult out;
    out.regState = static_cast<ar::network::RegState>(r.regState);
    out.rat = static_cast<ar::RadioTechnology>(r.rat);
    out.reasonForDenial = static_cast<ar::network::RegistrationFailCause>(r.reasonForDenial);
    out.cellIdentity = toAidl(r.cellIdentity);
    out.registeredPlmn = "";
    ar::network::AccessTechnologySpecificInfo ats;
    out.accessTechnologySpecificInfo = ats;
    return out;
}

ar::network::RegStateResult toAidl(const R10::DataRegStateResult& r) {
    ar::network::RegStateResult out;
    out.regState = static_cast<ar::network::RegState>(r.regState);
    out.rat = static_cast<ar::RadioTechnology>(r.rat);
    out.reasonForDenial = static_cast<ar::network::RegistrationFailCause>(r.reasonDataDenied);
    out.cellIdentity = toAidl(r.cellIdentity);
    out.registeredPlmn = "";
    ar::network::AccessTechnologySpecificInfo ats;
    out.accessTechnologySpecificInfo = ats;
    return out;
}

ar::network::OperatorInfo toAidl(const R10::OperatorInfo& op) {
    ar::network::OperatorInfo out;
    out.alphaLong = op.alphaLong;
    out.alphaShort = op.alphaShort;
    out.operatorNumeric = op.operatorNumeric;
    out.status = static_cast<int32_t>(op.status);
    return out;
}

// The framework's network type bitmask uses TelephonyManager.NETWORK_TYPE
// values shifted by one (NETWORK_TYPE_BITMASK_X = 1 << (NETWORK_TYPE_X - 1)):
//   GPRS=1, EDGE=2, UMTS=4, HSDPA=128, HSUPA=256, HSPA=512, HSPAP=16384,
//   GSM=32768, LTE=4096, NR=524288
// so the GSM family is 0x8003 and the WCDMA family is 0x4384.
// The AIDL radio interfaces express the allowed network types as a bitmask
// of the RadioTechnology values (bit T = 1 << T):
//   GPRS=1->0x2, EDGE=2->0x4, UMTS=3->0x8, HSDPA=9->0x200, HSUPA=10->0x400,
//   HSPA=11->0x800, LTE=14->0x4000, HSPAP=15->0x8000, GSM=16->0x10000
// so the GSM family is 0x10006 and the WCDMA family is 0x8A08.
namespace {
constexpr int32_t kGsm = 0x10006;
constexpr int32_t kWcdma = 0x8A08;
constexpr int32_t kLte = 0x4000;
constexpr int32_t kGsmWcdma = kGsm | kWcdma;
constexpr int32_t kGsmWcdmaLte = kGsmWcdma | kLte;
}  // namespace

// PreferredNetworkType -> network type bitmap.
int32_t toAidlAllowedNetworkTypesBitmap(int32_t preferredNetworkType) {
    switch (static_cast<R10::PreferredNetworkType>(preferredNetworkType)) {
        case R10::PreferredNetworkType::GSM_ONLY:
            return kGsm;
        case R10::PreferredNetworkType::WCDMA:
            return kWcdma;
        case R10::PreferredNetworkType::GSM_WCDMA:
        case R10::PreferredNetworkType::GSM_WCDMA_AUTO:
            return kGsmWcdma;
        case R10::PreferredNetworkType::LTE_ONLY:
            return kLte;
        case R10::PreferredNetworkType::LTE_WCDMA:
            return kLte | kWcdma;
        case R10::PreferredNetworkType::LTE_GSM_WCDMA:
        default:
            return kGsmWcdmaLte;
    }
}

int32_t toHidlPreferredNetworkType(int32_t networkTypeBitmap) {
    const int32_t gsm = networkTypeBitmap & kGsm;
    const int32_t wcdma = networkTypeBitmap & kWcdma;
    const int32_t lte = networkTypeBitmap & kLte;
    if (gsm && wcdma && lte) return static_cast<int32_t>(R10::PreferredNetworkType::LTE_GSM_WCDMA);
    if (gsm && wcdma) return static_cast<int32_t>(R10::PreferredNetworkType::GSM_WCDMA_AUTO);
    if (wcdma && lte) return static_cast<int32_t>(R10::PreferredNetworkType::LTE_WCDMA);
    if (gsm && lte) return static_cast<int32_t>(R10::PreferredNetworkType::LTE_GSM_WCDMA);
    if (wcdma) return static_cast<int32_t>(R10::PreferredNetworkType::WCDMA);
    if (lte) return static_cast<int32_t>(R10::PreferredNetworkType::LTE_ONLY);
    if (gsm) return static_cast<int32_t>(R10::PreferredNetworkType::GSM_ONLY);
    return static_cast<int32_t>(R10::PreferredNetworkType::LTE_GSM_WCDMA);
}

// ---------------------------------------------------------------------------
// sim
// ---------------------------------------------------------------------------

ar::sim::AppStatus toAidl(const R10::AppStatus& app) {
    ar::sim::AppStatus out;
    out.appType = static_cast<int32_t>(app.appType);
    out.appState = static_cast<int32_t>(app.appState);
    out.persoSubstate = static_cast<ar::sim::PersoSubstate>(app.persoSubstate);
    out.aidPtr = app.aidPtr;
    out.appLabelPtr = app.appLabelPtr;
    out.pin1Replaced = app.pin1Replaced != 0;
    out.pin1 = static_cast<ar::sim::PinState>(app.pin1);
    out.pin2 = static_cast<ar::sim::PinState>(app.pin2);
    return out;
}

ar::sim::CardStatus toAidl(const R10::CardStatus& cs, const std::string& iccid) {
    ar::sim::CardStatus out;
    out.cardState = static_cast<int32_t>(cs.cardState);
    out.universalPinState = static_cast<ar::sim::PinState>(cs.universalPinState);
    out.gsmUmtsSubscriptionAppIndex = cs.gsmUmtsSubscriptionAppIndex;
    out.cdmaSubscriptionAppIndex = cs.cdmaSubscriptionAppIndex;
    out.imsSubscriptionAppIndex = cs.imsSubscriptionAppIndex;
    for (const auto& app : cs.applications) {
        out.applications.push_back(toAidl(app));
    }
    // V1.0 has no atr/eid; the iccid is read by the shim from EF-ICCID.
    out.atr = "";
    out.iccid = iccid;
    out.eid = "";
    ar::config::SlotPortMapping slotMap;
    slotMap.physicalSlotId = 0;
    slotMap.portId = 0;
    out.slotMap = slotMap;
    out.supportedMepMode = ar::config::MultipleEnabledProfilesMode::NONE;
    return out;
}

R10::IccIo toHidl(const ar::sim::IccIo& io) {
    R10::IccIo out;
    out.command = io.command;
    out.fileId = io.fileId;
    out.path = io.path;
    out.p1 = io.p1;
    out.p2 = io.p2;
    out.p3 = io.p3;
    out.data = io.data;
    out.pin2 = io.pin2;
    out.aid = io.aid;
    return out;
}

ar::sim::IccIoResult toAidl(const R10::IccIoResult& r) {
    ar::sim::IccIoResult out;
    out.sw1 = r.sw1;
    out.sw2 = r.sw2;
    out.simResponse = r.simResponse;
    return out;
}

// ---------------------------------------------------------------------------
// voice
// ---------------------------------------------------------------------------

ar::voice::Call toAidl(const R10::Call& call) {
    ar::voice::Call out;
    out.state = static_cast<int32_t>(call.state);
    out.index = call.index;
    out.toa = call.toa;
    out.isMpty = call.isMpty;
    out.isMT = call.isMT;
    out.als = call.als;
    out.isVoice = call.isVoice;
    out.isVoicePrivacy = call.isVoicePrivacy;
    out.number = call.number;
    out.numberPresentation = static_cast<int32_t>(call.numberPresentation);
    out.name = call.name;
    out.namePresentation = static_cast<int32_t>(call.namePresentation);
    for (const auto& u : call.uusInfo) {
        ar::voice::UusInfo uus;
        uus.uusType = static_cast<int32_t>(u.uusType);
        uus.uusDcs = static_cast<int32_t>(u.uusDcs);
        uus.uusData = u.uusData;
        out.uusInfo.push_back(uus);
    }
    out.audioQuality = ar::voice::AudioQuality::UNSPECIFIED;
    out.forwardedNumber = "";
    return out;
}

// ---------------------------------------------------------------------------
// messaging
// ---------------------------------------------------------------------------

ar::messaging::SendSmsResult toAidl(const R10::SendSmsResult& r) {
    ar::messaging::SendSmsResult out;
    out.messageRef = r.messageRef;
    out.ackPDU = r.ackPDU;
    out.errorCode = r.errorCode;
    return out;
}

R10::GsmSmsMessage toHidl(const ar::messaging::GsmSmsMessage& m) {
    R10::GsmSmsMessage out;
    out.smscPdu = m.smscPdu;
    out.pdu = m.pdu;
    return out;
}

R10::SmsWriteArgs toHidl(const ar::messaging::SmsWriteArgs& w) {
    R10::SmsWriteArgs out;
    out.status = static_cast<R10::SmsWriteArgsStatus>(w.status);
    out.pdu = w.pdu;
    out.smsc = w.smsc;
    return out;
}

// ---------------------------------------------------------------------------
// data
// ---------------------------------------------------------------------------

ar::data::PdpProtocolType toAidlPdpType(const std::string& type) {
    if (type == "IP") return ar::data::PdpProtocolType::IP;
    if (type == "IPV6") return ar::data::PdpProtocolType::IPV6;
    if (type == "IPV4V6") return ar::data::PdpProtocolType::IPV4V6;
    if (type == "PPP") return ar::data::PdpProtocolType::PPP;
    if (type == "NON-IP") return ar::data::PdpProtocolType::NON_IP;
    return ar::data::PdpProtocolType::UNKNOWN;
}

std::string toHidlPdpTypeString(ar::data::PdpProtocolType type) {
    switch (type) {
        case ar::data::PdpProtocolType::IPV6:
            return "IPV6";
        case ar::data::PdpProtocolType::IPV4V6:
            return "IPV4V6";
        case ar::data::PdpProtocolType::PPP:
            return "PPP";
        case ar::data::PdpProtocolType::NON_IP:
            return "NON-IP";
        case ar::data::PdpProtocolType::IP:
        default:
            return "IP";
    }
}

std::vector<ar::data::LinkAddress> toAidlLinkAddresses(const std::string& spaceDelimited) {
    std::vector<ar::data::LinkAddress> out;
    std::string cur;
    for (size_t i = 0; i <= spaceDelimited.size(); ++i) {
        if (i == spaceDelimited.size() || spaceDelimited[i] == ' ') {
            if (cur.empty()) continue;
            ar::data::LinkAddress la;
            const size_t slash = cur.find('/');
            if (slash == std::string::npos) {
                la.address = cur;
                la.addressProperties = cur.find(':') == std::string::npos ? 32 : 128;
            } else {
                la.address = cur.substr(0, slash);
                la.addressProperties = std::atoi(cur.substr(slash + 1).c_str());
            }
            la.deprecationTime = 0;
            la.expirationTime = 0;
            out.push_back(la);
            cur.clear();
        } else {
            cur += spaceDelimited[i];
        }
    }
    return out;
}

ar::data::SetupDataCallResult toAidl(const R10::SetupDataCallResult& r) {
    ar::data::SetupDataCallResult out;
    out.cause = static_cast<ar::data::DataCallFailCause>(r.status);
    out.suggestedRetryTime = r.suggestedRetryTime;
    out.cid = r.cid;
    out.active = r.active;
    out.type = toAidlPdpType(r.type);
    out.ifname = r.ifname;
    out.addresses = toAidlLinkAddresses(r.addresses);
    out.dnses = {};
    out.gateways = {};
    out.pcscf = {};
    auto splitSpace = [](const hidl_string& in) {
        std::vector<std::string> out;
        std::string cur;
        const std::string str(in.c_str());
        for (size_t i = 0; i <= str.size(); ++i) {
            if (i == str.size() || str[i] == ' ') {
                if (!cur.empty()) {
                    out.push_back(cur);
                    cur.clear();
                }
            } else {
                cur += str[i];
            }
        }
        return out;
    };
    out.dnses = splitSpace(r.dnses);
    out.gateways = splitSpace(r.gateways);
    out.pcscf = splitSpace(r.pcscf);
    out.mtuV4 = r.mtu;
    out.mtuV6 = r.mtu;
    return out;
}

R10::DataProfileInfo toHidl(const ar::data::DataProfileInfo& p) {
    R10::DataProfileInfo out;
    out.profileId = static_cast<R10::DataProfileId>(p.profileId);
    out.apn = p.apn;
    out.protocol = toHidlPdpTypeString(p.protocol);
    out.roamingProtocol = toHidlPdpTypeString(p.roamingProtocol);
    out.authType = static_cast<R10::ApnAuthType>(p.authType);
    out.user = p.user;
    out.password = p.password;
    out.type = static_cast<R10::DataProfileInfoType>(p.type);
    out.maxConnsTime = p.maxConnsTime;
    out.maxConns = p.maxConns;
    out.waitTime = p.waitTime;
    out.enabled = p.enabled;
    out.supportedApnTypesBitmap = p.supportedApnTypesBitmap;
    out.bearerBitmap = p.bearerBitmap;
    out.mtu = (p.mtuV4 > 0) ? p.mtuV4 : p.mtuV6;
    out.mvnoType = R10::MvnoType::NONE;
    out.mvnoMatchData = "";
    return out;
}

}  // namespace android::hardware::radio::shim
