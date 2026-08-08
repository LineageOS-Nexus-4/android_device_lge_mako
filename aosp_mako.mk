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

# Boot animation
TARGET_SCREEN_HEIGHT := 1280
TARGET_SCREEN_WIDTH := 768

# Inherit from the common AOSP product configuration
$(call inherit-product, $(SRC_TARGET_DIR)/product/aosp_base_telephony.mk)

# Inherit from hardware-specific part of the product configuration
$(call inherit-product, device/lge/mako/device.mk)
$(call inherit-product, vendor/lge/mako/mako-vendor.mk)

# Dalvik (2 GB configuration)
$(call inherit-product, frameworks/native/build/phone-xhdpi-2048-dalvik-heap.mk)

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := mako
PRODUCT_NAME := aosp_mako
PRODUCT_BRAND := google
PRODUCT_MODEL := Nexus 4
PRODUCT_MANUFACTURER := LGE

PRODUCT_SHIPPING_API_LEVEL := 17

# mako: kernel 3.4 has no EROFS support; build APEX payloads as ext4 so
# apexd can loop-mount them (ext4+loop+offset is supported on 3.4).
PRODUCT_DEFAULT_APEX_PAYLOAD_TYPE := ext4