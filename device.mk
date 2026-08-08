#
# Copyright (C) 2011 The Android Open-Source Project
# Copyright (C) 2017-2019 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This file includes all definitions that apply to ALL mako devices, and
# are also specific to mako devices
#
# Everything in this directory will become public

# Soong
PRODUCT_SOONG_NAMESPACES += $(LOCAL_PATH)
PRODUCT_SOONG_NAMESPACES += hardware/qcom/wlan/legacy

# Overlays
DEVICE_PACKAGE_OVERLAYS += \
    $(LOCAL_PATH)/overlay

# This device is xhdpi.  However the platform doesn't
# currently contain all of the bitmaps at xhdpi density so
# we do this little trick to fall back to the hdpi version
# if the xhdpi doesn't exist.
PRODUCT_AAPT_CONFIG := normal
PRODUCT_AAPT_PREF_CONFIG := xhdpi
# A list of dpis to select prebuilt apk, in precedence order.
PRODUCT_AAPT_PREBUILT_DPI := hdpi

# Audio
PRODUCT_PACKAGES += \
    android.hardware.audio@2.0-impl \
    android.hardware.audio@2.0-service \
    android.hardware.audio.effect@2.0-impl \
    audio.primary.msm8960 \
    audio.usb.default \
    audio.r_submix.default \
    libaudio-resampler \
    libqcomvoiceprocessing

PRODUCT_COPY_FILES += \
    frameworks/av/services/audiopolicy/config/a2dp_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/a2dp_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/audio_policy_volumes.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_volumes.xml \
    frameworks/av/services/audiopolicy/config/default_volume_tables.xml:$(TARGET_COPY_OUT_VENDOR)/etc/default_volume_tables.xml \
    frameworks/av/services/audiopolicy/config/r_submix_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/r_submix_audio_policy_configuration.xml \
    frameworks/av/services/audiopolicy/config/usb_audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/usb_audio_policy_configuration.xml

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/audio/audio_effects.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_effects.xml \
    $(LOCAL_PATH)/audio/audio_platform_info.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_platform_info.xml \
    $(LOCAL_PATH)/audio/audio_policy_configuration.xml:$(TARGET_COPY_OUT_VENDOR)/etc/audio_policy_configuration.xml \
    $(LOCAL_PATH)/audio/mixer_paths.xml:$(TARGET_COPY_OUT_VENDOR)/etc/mixer_paths.xml

# Bluetooth
PRODUCT_PACKAGES += \


PRODUCT_PACKAGES += \
    bdAddrLoader

# Camera
PRODUCT_PACKAGES += \
    android.hardware.camera.provider@2.4-impl \
    android.hardware.camera.provider@2.4-service

# Charger
PRODUCT_PACKAGES += \
    charger_res_images

# DRM
PRODUCT_PACKAGES += \
    android.hardware.drm@1.0-impl \
    android.hardware.drm@1.0-service

# Gatekeeper
PRODUCT_PACKAGES += \
    android.hardware.gatekeeper@1.0-service.software

# Graphics
PRODUCT_PACKAGES += \
    android.hardware.graphics.allocator@2.0-impl \
    android.hardware.graphics.allocator@2.0-service \
    android.hardware.graphics.composer@2.1-service \
    android.hardware.graphics.mapper@2.0-impl-2.1 \
    android.hardware.memtrack@1.0-impl \
    android.hardware.memtrack@1.0-service \
    hwcomposer.msm8960 \
    gralloc.msm8960 \
    copybit.msm8960 \
    memtrack.msm8960

# GPS (deferred: requires external/gps.utils + libloc repos not in this tree)

# GNSS HAL
PRODUCT_PACKAGES += \
    android.hardware.gnss@1.0-impl \
    android.hardware.gnss@1.0-service

# Health
PRODUCT_PACKAGES += \
    android.hardware.health-service.example

# HIDL
PRODUCT_ENFORCE_VINTF_MANIFEST_OVERRIDE := true

# Keylayouts
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/keylayout/apq8064-tabla-snd-card_Button_Jack.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/apq8064-tabla-snd-card_Button_Jack.kl \
    $(LOCAL_PATH)/keylayout/hs_detect.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/hs_detect.kl \
    $(LOCAL_PATH)/keylayout/keypad_8064.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/keypad_8064.kl \
    $(LOCAL_PATH)/keylayout/pmic8xxx_pwrkey.kl:$(TARGET_COPY_OUT_VENDOR)/usr/keylayout/pmic8xxx_pwrkey.kl

# Keychars
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/keychars/apq8064-tabla-snd-card_Button_Jack.kcm:$(TARGET_COPY_OUT_VENDOR)/usr/keychars/apq8064-tabla-snd-card_Button_Jack.kcm \
    $(LOCAL_PATH)/keychars/hs_detect.kcm:$(TARGET_COPY_OUT_VENDOR)/usr/keychars/hs_detect.kcm \
    $(LOCAL_PATH)/keychars/keypad_8064.kcm:$(TARGET_COPY_OUT_VENDOR)/usr/keychars/keypad_8064.kcm \
    $(LOCAL_PATH)/keychars/pmic8xxx_pwrkey.kcm:$(TARGET_COPY_OUT_VENDOR)/usr/keychars/pmic8xxx_pwrkey.kcm

# Keymaster
PRODUCT_PACKAGES += \
    android.hardware.keymaster@3.0-impl \
    android.hardware.keymaster@3.0-service

# Lights
PRODUCT_PACKAGES += \
    android.hardware.lights-service.mako

# Media
PRODUCT_COPY_FILES += \
    frameworks/av/media/libstagefright/data/media_codecs_google_audio.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_audio.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_telephony.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_telephony.xml \
    frameworks/av/media/libstagefright/data/media_codecs_google_video_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_google_video_le.xml

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/media/media_profiles_V1_0.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_profiles_V1_0.xml \
    $(LOCAL_PATH)/media/media_codecs.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs.xml \
    $(LOCAL_PATH)/media/media_codecs_performance.xml:$(TARGET_COPY_OUT_VENDOR)/etc/media_codecs_performance.xml

# Misc
PRODUCT_CHARACTERISTICS := nosdcard

# NFC (deferred: requires LineageOS packages/apps/Nfc fork for bcm2079x)
# PRODUCT_PACKAGES += \
#     android.hardware.nfc@1.0-impl-bcm \
#     android.hardware.nfc@1.0-service \
#     nfc_nci.bcm2079x.default \
#     NfcNci \
#     Tag
#
# PRODUCT_COPY_FILES += \
#     $(LOCAL_PATH)/nfc/libnfc-nci.conf:$(TARGET_COPY_OUT_VENDOR)/etc/libnfc-nci.conf

# OMX
PRODUCT_PACKAGES += \


# Permissions
PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/android.hardware.camera.flash-autofocus.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.flash-autofocus.xml \
    frameworks/native/data/etc/android.hardware.camera.front.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.camera.front.xml \
    frameworks/native/data/etc/android.hardware.location.gps.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.location.gps.xml \
    frameworks/native/data/etc/android.hardware.wifi.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.xml \
    frameworks/native/data/etc/android.hardware.wifi.direct.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.wifi.direct.xml \
    frameworks/native/data/etc/android.hardware.sensor.proximity.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.sensor.proximity.xml \
    frameworks/native/data/etc/android.hardware.sensor.light.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.sensor.light.xml \
    frameworks/native/data/etc/android.hardware.sensor.gyroscope.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.sensor.gyroscope.xml \
    frameworks/native/data/etc/android.hardware.sensor.barometer.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.sensor.barometer.xml \
    frameworks/native/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
    frameworks/native/data/etc/android.software.sip.voip.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.sip.voip.xml \
    frameworks/native/data/etc/android.software.print.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.software.print.xml \
    frameworks/native/data/etc/android.hardware.usb.accessory.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.usb.accessory.xml \
    frameworks/native/data/etc/android.hardware.telephony.gsm.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.telephony.gsm.xml \
    frameworks/native/data/etc/android.hardware.audio.low_latency.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.audio.low_latency.xml \
    frameworks/native/data/etc/android.hardware.bluetooth_le.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.bluetooth_le.xml \
    frameworks/native/data/etc/android.hardware.ethernet.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.ethernet.xml \
    frameworks/native/data/etc/android.hardware.nfc.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.nfc.xml \
    frameworks/native/data/etc/android.hardware.nfc.hce.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/android.hardware.nfc.hce.xml

PRODUCT_COPY_FILES += \
    frameworks/native/data/etc/aosp_excluded_hardware.xml:$(TARGET_COPY_OUT_VENDOR)/etc/permissions/aosp_excluded_hardware.xml

# Power
PRODUCT_PACKAGES += \
    android.hardware.power@1.0-impl \
    android.hardware.power@1.0-service.mako

# Power Stats
PRODUCT_PACKAGES += \
    android.hardware.power.stats@1.0-service.mock

# Properties
include $(LOCAL_PATH)/system_prop.mk

# mako bring-up debug: kernel log capture to /cache (remove after bring-up)
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/zz_debug_capture.rc:$(TARGET_COPY_OUT_SYSTEM)/etc/init/zz_debug_capture.rc \
    $(LOCAL_PATH)/debug_capture.sh:$(TARGET_COPY_OUT_SYSTEM)/bin/debug_capture.sh

# mako bring-up debug: logcat capture to /data (remove after bring-up)
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/zz_debug_logcat.rc:$(TARGET_COPY_OUT_SYSTEM)/etc/init/zz_debug_logcat.rc \
    $(LOCAL_PATH)/debug_logcat.sh:$(TARGET_COPY_OUT_SYSTEM)/bin/debug_logcat.sh

# /metadata dir must exist in the system image so first-stage SwitchRoot
# can move the mounted /metadata partition into the new root.
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/metadata.placeholder:$(TARGET_COPY_OUT_SYSTEM)/metadata/metadata.placeholder

# Ramdisk
PRODUCT_PACKAGES += \
    fstab.mako \
    init.mako.rc \
    init.mako.bt.sh \
    init.mako.power.rc \
    init.mako.usb.rc \
    init.qcom.devstart.sh \
    init.qcom.devwait.sh \
    ueventd.mako.rc

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/fstab.mako:$(TARGET_COPY_OUT_RAMDISK)/fstab.mako


# RenderScript HAL
PRODUCT_PACKAGES += \


# Sensors
PRODUCT_PACKAGES += \
    android.hardware.sensors@1.0-impl \
    sensors.msm8960

# Thermal
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/thermald-mako.conf:$(TARGET_COPY_OUT_VENDOR)/etc/thermald.conf

# Touchscreen calibration
PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/configs/touch_dev.idc:$(TARGET_COPY_OUT_VENDOR)/usr/idc/touch_dev.idc

# Vibrator
PRODUCT_PACKAGES += \


# USB HAL
PRODUCT_PACKAGES += \
    android.hardware.usb@1.0-service

# WCNSS (firmware loader deferred: needs qcom proprietary wcnss_service)
PRODUCT_PACKAGES += \
    libwcnss_qmi

# Wi-Fi (HAL service deferred until android.hardware.wifi@1.0-service.legacy is vendored)
PRODUCT_PACKAGES += \
    libwpa_client \
    hostapd \
    wificond \
    wpa_supplicant \
    wpa_supplicant.conf

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/wifi/WCNSS_cfg.dat:$(TARGET_COPY_OUT_VENDOR)/firmware/wlan/prima/WCNSS_cfg.dat \
    $(LOCAL_PATH)/wifi/WCNSS_qcom_cfg.ini:$(TARGET_COPY_OUT_VENDOR)/firmware/wlan/prima/WCNSS_qcom_cfg.ini \
    $(LOCAL_PATH)/wifi/WCNSS_qcom_wlan_nv.bin:$(TARGET_COPY_OUT_VENDOR)/firmware/wlan/prima/WCNSS_qcom_wlan_nv.bin

PRODUCT_COPY_FILES += \
    $(LOCAL_PATH)/wifi/wpa_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/wpa_supplicant_overlay.conf \
    $(LOCAL_PATH)/wifi/p2p_supplicant_overlay.conf:$(TARGET_COPY_OUT_VENDOR)/etc/wifi/p2p_supplicant_overlay.conf
