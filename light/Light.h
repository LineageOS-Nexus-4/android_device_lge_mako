/*
 * Copyright (C) 2018 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ANDROID_HARDWARE_LIGHT_AIDL_LIGHT_H
#define ANDROID_HARDWARE_LIGHT_AIDL_LIGHT_H

#include <aidl/android/hardware/light/BnLights.h>
#include <aidl/android/hardware/light/HwLight.h>
#include <aidl/android/hardware/light/HwLightState.h>

#include <fstream>
#include <functional>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

using ::aidl::android::hardware::light::HwLight;
using ::aidl::android::hardware::light::HwLightState;
using ::aidl::android::hardware::light::LightType;

struct Light : public ::aidl::android::hardware::light::BnLights {
    Light(std::pair<std::ofstream, uint32_t>&& lcd_backlight,
          std::ofstream&& red_led,
          std::ofstream&& green_led,
          std::ofstream&& blue_led,
          std::ofstream&& led_blink,
          std::ofstream&& led_grp,
          std::ofstream&& led_pwm,
          std::ofstream&& led_lock);

    // Methods from ::aidl::android::hardware::light::ILights follow.
    ::ndk::ScopedAStatus setLightState(int32_t id, const HwLightState& state) override;
    ::ndk::ScopedAStatus getLights(std::vector<HwLight>* _aidl_return) override;
    ::ndk::ScopedAStatus setLightEffects(
            const std::vector<::aidl::android::hardware::light::HwLightEffect>& effects) override;

  private:
    void setAttentionLight(const HwLightState& state);
    void setBatteryLight(const HwLightState& state);
    void setLcdBacklight(const HwLightState& state);
    void setNotificationLight(const HwLightState& state);
    void setSpeakerBatteryLightLocked();
    void setSpeakerLightLocked(const HwLightState& state);

    std::pair<std::ofstream, uint32_t> mLcdBacklight;
    std::ofstream mRedLed;
    std::ofstream mGreenLed;
    std::ofstream mBlueLed;
    std::ofstream mLedBlink;
    std::ofstream mLedGrp;
    std::ofstream mLedPwm;
    std::ofstream mLedLock;

    HwLightState mAttentionState;
    HwLightState mBatteryState;
    HwLightState mNotificationState;

    std::unordered_map<int32_t, LightType> mTypes;
    std::mutex mLock;
};

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl

#endif  // ANDROID_HARDWARE_LIGHT_AIDL_LIGHT_H
