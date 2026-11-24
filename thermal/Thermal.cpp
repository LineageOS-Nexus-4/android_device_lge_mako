// SPDX-License-Identifier: Apache-2.0
#include "ThermalPolicy.h"

#include <aidl/android/hardware/thermal/BnThermal.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>
#include <glob.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <limits>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace thermal = aidl::android::hardware::thermal;
using thermal::CoolingDevice;
using thermal::CoolingType;
using thermal::ICoolingDeviceChangedCallback;
using thermal::IThermalChangedCallback;
using thermal::Temperature;
using thermal::TemperatureThreshold;
using thermal::TemperatureType;
using thermal::ThrottlingSeverity;
using ndk::ScopedAStatus;

constexpr char kBatteryName[] = "mako-battery";
// Android aggregates app-visible severity from SKIN sensors. This virtual
// sensor represents battery-driven mitigation, not measured enclosure heat.
constexpr char kPolicyName[] = "mako-virtual-battery-policy";
constexpr char kBatteryPath[] = "/sys/class/power_supply/battery/temp";

struct Sensor {
    TemperatureType type;
    std::string name;
    std::string path;
    float divisor;
};

std::vector<Sensor> discoverSensors() {
    std::vector<Sensor> sensors;
    glob_t paths{};
    if (glob("/sys/class/thermal/thermal_zone*/type", 0, nullptr, &paths) == 0) {
        for (size_t i = 0; i < paths.gl_pathc; ++i) {
            std::string path = paths.gl_pathv[i];
            std::ifstream file(path);
            std::string name;
            if (!(file >> name)) continue;
            path.replace(path.size() - 4, 4, "temp");
            if (name == "tsens_tz_sensor7" || name == "tsens_tz_sensor8" ||
                name == "tsens_tz_sensor9" || name == "tsens_tz_sensor10") {
                sensors.push_back({TemperatureType::CPU, name, path, 1.0f});
            } else if (name == "pm8921_tz") {
                sensors.push_back({TemperatureType::UNKNOWN, name, path, 1000.0f});
            }
        }
    }
    globfree(&paths);
    return sensors;
}

std::optional<int> readTemperature(const std::string& path, float divisor) {
    std::ifstream file(path);
    int raw;
    if (!(file >> raw) || raw < -40 * divisor || raw > 160 * divisor) return std::nullopt;
    return raw;
}

TemperatureThreshold threshold(TemperatureType type, const char* name) {
    TemperatureThreshold result;
    result.type = type;
    result.name = name;
    const float unavailable = std::numeric_limits<float>::quiet_NaN();
    const size_t count = static_cast<size_t>(ThrottlingSeverity::SHUTDOWN) + 1;
    result.hotThrottlingThresholds.resize(count, unavailable);
    result.coldThrottlingThresholds.resize(count, unavailable);
    for (size_t i = 0; i < mako::kBatteryHot.size(); ++i) {
        result.hotThrottlingThresholds[i + 1] = mako::kBatteryHot[i] / 10.0f;
        result.coldThrottlingThresholds[i + 1] = mako::kBatteryCold[i] / 10.0f;
    }
    return result;
}

class Thermal final : public thermal::BnThermal {
  public:
    ScopedAStatus getTemperatures(std::vector<Temperature>* out) override {
        *out = temperatures();
        return ScopedAStatus::ok();
    }

    ScopedAStatus getTemperaturesWithType(TemperatureType type,
                                          std::vector<Temperature>* out) override {
        auto current = temperatures();
        std::copy_if(current.begin(), current.end(), std::back_inserter(*out),
                     [type](const Temperature& value) { return value.type == type; });
        return ScopedAStatus::ok();
    }

    ScopedAStatus getTemperatureThresholds(std::vector<TemperatureThreshold>* out) override {
        *out = {threshold(TemperatureType::BATTERY, kBatteryName),
                threshold(TemperatureType::SKIN, kPolicyName)};
        return ScopedAStatus::ok();
    }

    ScopedAStatus getTemperatureThresholdsWithType(
            TemperatureType type, std::vector<TemperatureThreshold>* out) override {
        if (type == TemperatureType::BATTERY) {
            out->push_back(threshold(type, kBatteryName));
        } else if (type == TemperatureType::SKIN) {
            out->push_back(threshold(type, kPolicyName));
        }
        return ScopedAStatus::ok();
    }

    ScopedAStatus getCoolingDevices(std::vector<CoolingDevice>* out) override {
        // This kernel has no standard cooling_device nodes.
        out->clear();
        return ScopedAStatus::ok();
    }

    ScopedAStatus getCoolingDevicesWithType(CoolingType, std::vector<CoolingDevice>* out) override {
        return getCoolingDevices(out);
    }

    ScopedAStatus registerThermalChangedCallback(
            const std::shared_ptr<IThermalChangedCallback>& callback) override {
        return registerClient(callback, std::nullopt);
    }

    ScopedAStatus registerThermalChangedCallbackWithType(
            const std::shared_ptr<IThermalChangedCallback>& callback, TemperatureType type) override {
        return registerClient(callback, type);
    }

    ScopedAStatus unregisterThermalChangedCallback(
            const std::shared_ptr<IThermalChangedCallback>& callback) override {
        if (!callback) return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        std::lock_guard<std::mutex> lock(mClientLock);
        return removeClient(callback) ? ScopedAStatus::ok()
                                      : ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    ScopedAStatus registerCoolingDeviceChangedCallbackWithType(
            const std::shared_ptr<ICoolingDeviceChangedCallback>&, CoolingType) override {
        return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    ScopedAStatus unregisterCoolingDeviceChangedCallback(
            const std::shared_ptr<ICoolingDeviceChangedCallback>&) override {
        return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    ScopedAStatus forecastSkinTemperature(int32_t seconds, float*) override {
        return ScopedAStatus::fromExceptionCode(seconds < 0 ? EX_ILLEGAL_ARGUMENT
                                                          : EX_UNSUPPORTED_OPERATION);
    }

    void poll() {
        for (;;) {
            auto current = temperatures();
            std::vector<std::pair<std::shared_ptr<IThermalChangedCallback>, Temperature>> events;
            {
                std::lock_guard<std::mutex> lock(mClientLock);
                for (auto& client : mClients) {
                    for (const auto& value : current) {
                        if (client.filter && *client.filter != value.type) continue;
                        auto [last, inserted] = client.severity.try_emplace(value.name,
                                                                          value.throttlingStatus);
                        if (inserted || last->second != value.throttlingStatus) {
                            last->second = value.throttlingStatus;
                            events.emplace_back(client.callback, value);
                        }
                    }
                }
            }
            for (const auto& [client, value] : events) {
                if (client->notifyThrottling(value).getStatus() == STATUS_DEAD_OBJECT) {
                    std::lock_guard<std::mutex> lock(mClientLock);
                    removeClient(client);
                }
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

  private:
    struct Client {
        std::shared_ptr<IThermalChangedCallback> callback;
        std::optional<TemperatureType> filter;
        std::map<std::string, ThrottlingSeverity> severity;
    };

    std::vector<Temperature> temperatures() {
        std::vector<Temperature> result;
        for (const auto& sensor : mSensors) {
            if (auto raw = readTemperature(sensor.path, sensor.divisor)) {
                result.push_back({sensor.type, sensor.name, *raw / sensor.divisor,
                                  ThrottlingSeverity::NONE});
            }
        }
        std::lock_guard<std::mutex> lock(mBatteryLock);
        if (auto raw = readTemperature(kBatteryPath, 10.0f)) {
            mBatteryLevel = mako::batterySeverity(*raw, mBatteryLevel);
            auto severity = static_cast<ThrottlingSeverity>(mBatteryLevel);
            result.push_back({TemperatureType::BATTERY, kBatteryName, *raw / 10.0f, severity});
            result.push_back({TemperatureType::SKIN, kPolicyName, *raw / 10.0f, severity});
        }
        return result;
    }

    ScopedAStatus registerClient(const std::shared_ptr<IThermalChangedCallback>& callback,
                                 std::optional<TemperatureType> filter) {
        if (!callback) return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
        std::lock_guard<std::mutex> lock(mClientLock);
        for (const auto& client : mClients) {
            if (client.callback->asBinder() == callback->asBinder()) {
                return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
            }
        }
        mClients.push_back({callback, filter, {}});
        return ScopedAStatus::ok();
    }

    bool removeClient(const std::shared_ptr<IThermalChangedCallback>& callback) {
        auto end = std::remove_if(mClients.begin(), mClients.end(), [&](const Client& client) {
            return client.callback->asBinder() == callback->asBinder();
        });
        if (end == mClients.end()) return false;
        mClients.erase(end, mClients.end());
        return true;
    }

    const std::vector<Sensor> mSensors = discoverSensors();
    std::mutex mClientLock;
    std::vector<Client> mClients;
    std::mutex mBatteryLock;
    size_t mBatteryLevel = 0;
};

}  // namespace

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(0);
    auto service = ndk::SharedRefBase::make<Thermal>();
    const std::string instance = std::string(Thermal::descriptor) + "/default";
    const auto status = AServiceManager_addService(service->asBinder().get(), instance.c_str());
    if (status != STATUS_OK) {
        std::fprintf(stderr, "Failed to register %s: %d\n", instance.c_str(), status);
        return 1;
    }
    std::thread([service] { service->poll(); }).detach();
    ABinderProcess_joinThreadPool();
    return 1;
}
