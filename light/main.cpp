/*
 * Copyright 2018 The LineageOS Project
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

#include <android-base/logging.h>
#include <android/binder_manager.h>
#include <android/binder_process.h>

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "Light.h"

using ::aidl::android::hardware::light::Light;

static const std::string kLcdBacklightPath = "/sys/class/leds/lcd-backlight/brightness";
static const std::string kLcdMaxBacklightPath = "/sys/class/leds/lcd-backlight/max_brightness";
static const std::string kRedLedPath = "/sys/class/leds/red/brightness";
static const std::string kGreenLedPath = "/sys/class/leds/green/brightness";
static const std::string kBlueLedPath = "/sys/class/leds/blue/brightness";
static const std::string kLedBlinkPath = "/sys/class/leds/red/device/blink";
static const std::string kLedGrpPath = "/sys/class/leds/red/device/grpfreq";
static const std::string kLedPwmPath = "/sys/class/leds/red/device/grppwm";
static const std::string kLedLockPath = "/sys/class/leds/red/device/lock";

int main() {
    ABinderProcess_setThreadPoolMaxThreadCount(1);

    uint32_t lcdMaxBrightness = 255;

    std::ofstream lcdBacklight(kLcdBacklightPath);
    if (!lcdBacklight) {
        LOG(ERROR) << "Failed to open " << kLcdBacklightPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ifstream lcdMaxBacklight(kLcdMaxBacklightPath);
    if (!lcdMaxBacklight) {
        LOG(ERROR) << "Failed to open " << kLcdMaxBacklightPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    } else {
        lcdMaxBacklight >> lcdMaxBrightness;
    }

    std::ofstream redLed(kRedLedPath);
    if (!redLed) {
        LOG(ERROR) << "Failed to open " << kRedLedPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream greenLed(kGreenLedPath);
    if (!greenLed) {
        LOG(ERROR) << "Failed to open " << kGreenLedPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream blueLed(kBlueLedPath);
    if (!blueLed) {
        LOG(ERROR) << "Failed to open " << kBlueLedPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream ledBlink(kLedBlinkPath);
    if (!ledBlink) {
        LOG(ERROR) << "Failed to open kLedBlinkPath: " << kLedBlinkPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream ledGrp(kLedGrpPath);
    if (!ledGrp) {
        LOG(ERROR) << "Failed to open kLedGrpPath: " << kLedGrpPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream ledPwm(kLedPwmPath);
    if (!ledPwm) {
        LOG(ERROR) << "Failed to open kLedPwmPath: " << kLedPwmPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::ofstream ledLock(kLedLockPath);
    if (!ledLock) {
        LOG(ERROR) << "Failed to open kLedLockPath: " << kLedLockPath << ", error=" << errno
                   << " (" << strerror(errno) << ")";
        return -errno;
    }

    std::shared_ptr<Light> service = ::ndk::SharedRefBase::make<Light>(
            std::make_pair(std::move(lcdBacklight), lcdMaxBrightness),
            std::move(redLed),
            std::move(greenLed),
            std::move(blueLed),
            std::move(ledBlink),
            std::move(ledGrp),
            std::move(ledPwm),
            std::move(ledLock));

    binder_status_t status = AServiceManager_addService(
            service->asBinder().get(), "android.hardware.light.ILights/default");
    if (status != STATUS_OK) {
        LOG(ERROR) << "Cannot register Light HAL service: " << status;
        return 1;
    }

    LOG(INFO) << "Light HAL Ready.";
    ABinderProcess_startThreadPool();
    ABinderProcess_joinThreadPool();
    // Under normal cases, execution will not reach this line.
    LOG(ERROR) << "Light HAL failed to join thread pool.";
    return 1;
}
