# mako shipped with Android 4.2 (API 17); required so A16 treats it as a
# pre-Q device (skips vendor property-namespace enforcement, legacy prop tool
# behavior, and gives checkvintf the right ro.product.first_api_level).
PRODUCT_SHIPPING_API_LEVEL := 17

# Boot animation
TARGET_SCREEN_HEIGHT := 1280
TARGET_SCREEN_WIDTH := 768

# Lineage Charger
WITH_LINEAGE_CHARGER := false

# Inherit from the common product configuration
$(call inherit-product, $(SRC_TARGET_DIR)/product/aosp_base_telephony.mk)

# Inherit from the Lineage vendor
$(call inherit-product, vendor/lineage/config/common_mini_phone.mk)

# Inherit from hardware-specific part of the product configuration
$(call inherit-product, device/lge/mako/device.mk)
$(call inherit-product, vendor/lge/mako/mako-vendor.mk)

# Include the LineageOS camera app in this mini phone product.
PRODUCT_PACKAGES += Aperture

# Enable debugging on userdebug builds. MAKOZ30f does not export the
# modern unlocked-boot property used by recovery's alternative USB enable path.
# This option only affects userdebug; ro.secure and ADB authentication stay on.
PRODUCT_NOT_DEBUGGABLE_IN_USERDEBUG :=

# Dalvik (2 GB configuration)
$(call inherit-product, frameworks/native/build/phone-xhdpi-2048-dalvik-heap.mk)

# Precompile the code responsible for the main interface and its transitions.
PRODUCT_DEXPREOPT_SPEED_APPS += Settings SystemUI Launcher3QuickStep

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := mako
PRODUCT_NAME := lineage_mako
PRODUCT_BRAND := google
PRODUCT_MODEL := Nexus 4
PRODUCT_MANUFACTURER := LGE

# Linux 3.4 can mount ext4 APEX payloads but has no EROFS support.
PRODUCT_DEFAULT_APEX_PAYLOAD_TYPE := ext4

# mako: gen_build_prop.py validates PRODUCT_BUILD_PROP_OVERRIDES keys
# against product_config.json; PRODUCT_NAME/PRIVATE_BUILD_DESC are not
# valid there and fail the build. BUILD_FINGERPRINT alone is fine.
BUILD_FINGERPRINT := google/occam/mako:5.1.1/LMY48T/2237560:user/release-keys
