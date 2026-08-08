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

#define LOG_TAG "lights-service.mako"

#include "Light.h"

#include <aidl/android/hardware/light/FlashMode.h>
#include <android-base/logging.h>

#include <cstdint>

namespace aidl {
namespace android {
namespace hardware {
namespace light {

using ::aidl::android::hardware::light::FlashMode;
using ::aidl::android::hardware::light::HwLight;
using ::aidl::android::hardware::light::HwLightState;
using ::aidl::android::hardware::light::LightType;
using ::ndk::ScopedAStatus;

namespace {
static constexpr int DEFAULT_MAX_BRIGHTNESS = 255;

static uint32_t rgbToBrightness(const HwLightState& state) {
    uint32_t color = state.color & 0x00ffffff;
    return ((77 * ((color >> 16) & 0xff)) + (150 * ((color >> 8) & 0xff)) +
            (29 * (color & 0xff))) >> 8;
}

static bool isLit(const HwLightState& state) {
    return (state.color & 0x00ffffff);
}
}  // anonymous namespace

Light::Light(std::pair<std::ofstream, uint32_t>&& lcd_backlight,
             std::ofstream&& red_led,
             std::ofstream&& green_led,
             std::ofstream&& blue_led,
             std::ofstream&& led_blink,
             std::ofstream&& led_grp,
             std::ofstream&& led_pwm,
             std::ofstream&& led_lock)
    : mLcdBacklight(std::move(lcd_backlight)),
      mRedLed(std::move(red_led)),
      mGreenLed(std::move(green_led)),
      mBlueLed(std::move(blue_led)),
      mLedBlink(std::move(led_blink)),
      mLedGrp(std::move(led_grp)),
      mLedPwm(std::move(led_pwm)),
      mLedLock(std::move(led_lock)) {
    mTypes.emplace(std::make_pair(static_cast<int32_t>(LightType::BACKLIGHT), LightType::BACKLIGHT));
    mTypes.emplace(std::make_pair(static_cast<int32_t>(LightType::BATTERY), LightType::BATTERY));
    mTypes.emplace(std::make_pair(static_cast<int32_t>(LightType::NOTIFICATIONS),
                                  LightType::NOTIFICATIONS));
    mTypes.emplace(std::make_pair(static_cast<int32_t>(LightType::ATTENTION), LightType::ATTENTION));
}

ScopedAStatus Light::setLightState(int32_t id, const HwLightState& state) {
    auto it = mTypes.find(id);
    if (it == mTypes.end()) {
        return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    switch (it->second) {
        case LightType::BACKLIGHT:
            setLcdBacklight(state);
            break;
        case LightType::BATTERY:
            setBatteryLight(state);
            break;
        case LightType::NOTIFICATIONS:
            setNotificationLight(state);
            break;
        case LightType::ATTENTION:
            setAttentionLight(state);
            break;
        default:
            return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
    }

    return ScopedAStatus::ok();
}

ScopedAStatus Light::getLights(std::vector<HwLight>* _aidl_return) {
    std::vector<HwLight> lights;
    for (const auto& [id, type] : mTypes) {
        HwLight light;
        light.id = id;
        light.ordinal = 0;
        light.type = type;
        light.minUpdatePeriodMillis = 0;
        lights.push_back(light);
    }
    *_aidl_return = lights;
    return ScopedAStatus::ok();
}

ScopedAStatus Light::setLightEffects(
        const std::vector<::aidl::android::hardware::light::HwLightEffect>& /*effects*/) {
    return ScopedAStatus::fromExceptionCode(EX_UNSUPPORTED_OPERATION);
}

void Light::setAttentionLight(const HwLightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    mAttentionState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setLcdBacklight(const HwLightState& state) {
    std::lock_guard<std::mutex> lock(mLock);

    uint32_t brightness = rgbToBrightness(state);

    // If max panel brightness is not the default (255),
    // apply linear scaling across the accepted range.
    if (mLcdBacklight.second != DEFAULT_MAX_BRIGHTNESS) {
        int old_brightness = brightness;
        brightness = brightness * mLcdBacklight.second / DEFAULT_MAX_BRIGHTNESS;
        LOG(VERBOSE) << "scaling brightness " << old_brightness << " => " << brightness;
    }

    mLcdBacklight.first << brightness << std::endl;
}

void Light::setBatteryLight(const HwLightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    mBatteryState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setNotificationLight(const HwLightState& state) {
    std::lock_guard<std::mutex> lock(mLock);
    mNotificationState = state;
    setSpeakerBatteryLightLocked();
}

void Light::setSpeakerBatteryLightLocked() {
    if (isLit(mNotificationState)) {
        setSpeakerLightLocked(mNotificationState);
    } else if (isLit(mAttentionState)) {
        setSpeakerLightLocked(mAttentionState);
    } else if (isLit(mBatteryState)) {
        setSpeakerLightLocked(mBatteryState);
    } else {
        // Lights off
        mRedLed << 0 << std::endl;
        mGreenLed << 0 << std::endl;
        mBlueLed << 0 << std::endl;
    }
}

void Light::setSpeakerLightLocked(const HwLightState& state) {
    int red, green, blue;
    int blink, freq, pwm;
    int onMs, offMs;
    uint32_t alpha;

    // Extract brightness from AARRGGBB
    alpha = (state.color >> 24) & 0xff;

    // Retrieve each of the RGB colors
    red = (state.color >> 16) & 0xff;
    green = (state.color >> 8) & 0xff;
    blue = state.color & 0xff;

    // Scale RGB colors if a brightness has been applied by the user
    if (alpha != 0xff) {
        red = (red * alpha) / 0xff;
        green = (green * alpha) / 0xff;
        blue = (blue * alpha) / 0xff;
    }

    switch (state.flashMode) {
        case FlashMode::TIMED:
            onMs = state.flashOnMs;
            offMs = state.flashOffMs;
            break;
        case FlashMode::NONE:
        default:
            onMs = 0;
            offMs = 0;
            break;
    }

    if (onMs > 0 && offMs > 0) {
        int totalMS = onMs + offMs;

        // the freq value must be set as period/50
        freq = totalMS / 50;

        // pwm specifies the ratio of ON versus OFF
        // pwm = 0 -> always off
        // pwm = 255 => always on
        pwm = (onMs * 255) / totalMS;

        blink = 1;
    } else {
        blink = 0;
        freq = 0;
        pwm = 0;
    }

    // LED Synchronization
    mLedLock << 1 << std::endl;

    // Disable all blinking to start
    mLedBlink << 0 << std::endl;

    mRedLed << red << std::endl;
    mGreenLed << green << std::endl;
    mBlueLed << blue << std::endl;

    mLedGrp << freq << std::endl;
    mLedPwm << pwm << std::endl;
    mLedBlink << blink << std::endl;

    // LED Synchronization
    mLedLock << 0 << std::endl;
}

}  // namespace light
}  // namespace hardware
}  // namespace android
}  // namespace aidl
