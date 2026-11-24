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

# Inherit from vendor
-include vendor/lge/mako/BoardConfigVendor.mk

# Legacy device: proprietary ELF blobs are shipped via PRODUCT_COPY_FILES
BUILD_BROKEN_ELF_PREBUILT_PRODUCT_COPY_FILES := true

# Device Path
DEVICE_PATH := device/lge/mako

# CPU
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := generic
TARGET_CPU_VARIANT_RUNTIME := krait

# Kernel
BOARD_KERNEL_BASE := 0x80200000
BOARD_KERNEL_IMAGE_NAME = zImage
BOARD_KERNEL_PAGESIZE := 2048
# The 3.4 EFI parser needs gpt to try the valid secondary after a torn primary.
BOARD_KERNEL_CMDLINE := androidboot.hardware=mako audit=1 gpt

# Real 100MB metadata partition (p26); system resized to 2 GiB
BOARD_USES_METADATA_PARTITION := true
BOARD_MKBOOTIMG_ARGS := --ramdisk_offset 0x01600000
# build-kernel.sh uses GCC 4.9 to build the image installed by kernel.mk.
TARGET_KERNEL_SOURCE := kernel/lge/mako
TARGET_KERNEL_CONFIG := lineageos_mako_defconfig
TARGET_PREBUILT_KERNEL := device/lge/mako/prebuilt/zImage
TARGET_FORCE_PREBUILT_KERNEL := true

# Platform
TARGET_NO_BOOTLOADER := true
TARGET_NO_RADIOIMAGE := true
TARGET_BOARD_PLATFORM := msm8960
TARGET_BOOTLOADER_BOARD_NAME := mako
TARGET_BOOTLOADER_NAME := mako

# mako is an A-only device; AOSP 17 defaults AB_OTA_UPDATER to true
AB_OTA_UPDATER := false
# AOSP 17 OTA tooling assumes recovery-as-boot; mako uses a separate recovery
# partition. OTA generation is enabled so a TWRP-flashable package is produced
# (flashing goes through TWRP to bypass the bootloader fastboot size check).
TARGET_SKIP_OTA_PACKAGE := false
# Assert the selected disk geometry and GPT names before OTA ROM writes.
TARGET_RELEASETOOLS_EXTENSIONS := $(DEVICE_PATH)
# Ship a full recovery image inside /system/vendor/etc so the non-A/B OTA
# generator (HasRecoveryPatch) finds it and the package build can complete.
BOARD_USES_FULL_RECOVERY_IMAGE := true

# Security patch level
VENDOR_SECURITY_PATCH := 2015-09-08

# QCOM hardware
BOARD_USES_QCOM_HARDWARE := true
TARGET_USES_LEGACY_QCOM_HARDWARE := true
# mako: legacy msm8960 display HAL (Android.mk) exposes display_intf_headers;
# SurfaceFlinger resolves its qtidisplay headers from there.
QCOM_SOONG_NAMESPACE := hardware/qcom/display/msm8960
QCOM_DISPLAY_SOONG_NAMESPACE := hardware/qcom/display/msm8960
PRODUCT_SOONG_NAMESPACES += hardware/qcom/display/msm8960
# The primary audio HAL was converted to Soong and lives in its own namespace.
PRODUCT_SOONG_NAMESPACES += hardware/qcom/audio
# mako: expose the legacy wlan defaults (libwifi-hal-qcom) to frameworks/opt/net/wifi
PRODUCT_SOONG_NAMESPACES += hardware/qcom/wlan

# Audio
AUDIO_FEATURE_ENABLED_FLUENCE := true
BOARD_USES_ALSA_AUDIO := true
AUDIO_USE_LL_AS_PRIMARY_OUTPUT := true
USE_CUSTOM_AUDIO_POLICY := 1

# Bluetooth
BOARD_HAVE_BLUETOOTH := true
BOARD_HAVE_BLUETOOTH_QCOM := true
BOARD_BLUETOOTH_QCOM_USE_VENDOR_PROPERTIES := true
SOONG_CONFIG_NAMESPACES += legacy_bluetooth
SOONG_CONFIG_legacy_bluetooth += vendor_address
SOONG_CONFIG_legacy_bluetooth_vendor_address := true
BLUETOOTH_HCI_USE_MCT := true
BOARD_BLUETOOTH_BDROID_BUILDCFG_INCLUDE_DIR := $(DEVICE_PATH)/bluetooth

# WCNSS
TARGET_USES_QCOM_WCNSS_QMI := true
TARGET_PROVIDES_WCNSS_QMI := true
TARGET_DISABLE_WCNSS_CONFIG_COPY := true

# Wi-Fi
BOARD_HAS_QCOM_WLAN := true
# mako bring-up: the AIDL ISupplicant is hosted by wpa_supplicant_mainline in
# the com.android.wifi APEX; install the unified supplicant init rc so the
# service starts at boot.
WIFI_HIDL_UNIFIED_SUPPLICANT_SERVICE_RC_ENTRY := true
# mako: use the legacy hardware/qcom/wlan tree (validated since AOSP 17);
# skips the CAF wlan namespaces in qcom-caf/common/BoardConfigQcom.mk and
# registers our display namespace override below.
TARGET_USES_LEGACY_WLAN := true
BOARD_WLAN_DEVICE := qcwcn
WPA_SUPPLICANT_VERSION := VER_0_8_X
BOARD_WPA_SUPPLICANT_DRIVER := NL80211
BOARD_WPA_SUPPLICANT_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
BOARD_HOSTAPD_DRIVER := NL80211
BOARD_HOSTAPD_PRIVATE_LIB := lib_driver_cmd_$(BOARD_WLAN_DEVICE)
WIFI_DRIVER_FW_PATH_STA := "sta"
WIFI_DRIVER_FW_PATH_AP  := "ap"
# qcwcn wpa_supplicant.conf is still built from the legacy Android.mk
FORCE_USE_ANDROIDMK_FOR_WPA_CONF := true

# Display
TARGET_SCREEN_DENSITY := 320

# Graphics
TARGET_USES_ION := true

# Our AIDL light service handles LEDs directly; skip qcom liblight
TARGET_PROVIDES_LIBLIGHT := true
TARGET_ADDITIONAL_GRALLOC_10_USAGE_BITS := 0x2000U | 0x02000000U
TARGET_DISABLE_POSTRENDER_CLEANUP := true

# Dexpreopt: pre-compile system apps (SystemUI, Launcher, Settings, etc.)
WITH_DEXPREOPT := true
WITH_DEXPREOPT_DEBUG_INFO := false

# The target has no boot jars to check
SKIP_BOOT_JARS_CHECK := true

# Filesystem
TARGET_FS_CONFIG_GEN := $(DEVICE_PATH)/config.fs
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_BOOTIMAGE_PARTITION_SIZE := 23068672 # 22M
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 23068672 # 22M
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 2147483648 # 2048M (2 GiB)
# Userdata size for the supported 16-GB partition layout.
# The OTA must not write userdata; this value only constrains explicit images.
BOARD_USERDATAIMAGE_PARTITION_SIZE := 13240369152
BOARD_CACHEIMAGE_PARTITION_SIZE := 104857600 # 100M
BOARD_CACHEIMAGE_FILE_SYSTEM_TYPE := ext4
BOARD_FLASH_BLOCK_SIZE := 131072 # (BOARD_KERNEL_PAGESIZE * 64)
# cpuacct must have its mount point before init switches to read-only system.
BOARD_ROOT_EXTRA_FOLDERS := acct firmware metadata persist

# Reduce space taken by the journal
BOARD_SYSTEMIMAGE_JOURNAL_SIZE := 0

# GPS
BOARD_VENDOR_QCOM_GPS_LOC_API_HARDWARE := $(TARGET_BOARD_PLATFORM)
TARGET_NO_RPC := true
BOARD_VENDOR_QCOM_LOC_PDK_FEATURE_SET := true

# SeLinux Policy
TARGET_QCOM_LEGACY_FACTORY_TESTS := false
TARGET_QCOM_LEGACY_DATASERVICES := false
TARGET_QCOM_STRICT_SEPOLICY := true
include device/qcom/sepolicy-legacy/sepolicy.mk
BOARD_VENDOR_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/vendor
PRODUCT_PRIVATE_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/private

# Export only the attribute ABI needed by the Lineage qcom overlay. The legacy
# policy already owns the FM types; importing modern generic/public duplicates them.
SYSTEM_EXT_PUBLIC_SEPOLICY_DIRS += $(DEVICE_PATH)/sepolicy/public

# Camera
TARGET_PROCESS_SDK_VERSION_OVERRIDE := \
    /system/bin/cameraserver=22 \
    /system/vendor/bin/mm-qcamera-daemon=22

# Recovery
# F2FS FBE is opt-in and requires separately provisioned userdata.
ifeq ($(TARGET_MAKO_EXPERIMENTAL_FBE),true)
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/rootdir/etc/fstab.mako.fbe
else
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/rootdir/etc/fstab.mako
endif

# Device manifest
DEVICE_MANIFEST_FILE := $(DEVICE_PATH)/manifest.xml
DEVICE_FRAMEWORK_COMPATIBILITY_MATRIX_FILE := device/lge/mako/compatibility_matrix.device.xml
