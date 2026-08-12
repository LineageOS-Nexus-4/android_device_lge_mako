# mako: first boot on a 1.5GHz 2012 SoC; skip AOT compilation so dexopt
# does not cook the device on the first boot (classes run in verify/JIT mode).
PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.dex2oat-filter=verify \
    dalvik.vm.image-dex2oat-filter=verify

# Audio
# mako bring-up: the audioserver is disabled; never wait for its binder
# service (each call would otherwise block 5s, tripping the Watchdog).
PRODUCT_PROPERTY_OVERRIDES += \
    audio.offload.disable=1 \
    audio.service.client_wait_ms=0 \
    ro.vendor.audio.sdk.fluencetype=fluence \
    persist.vendor.audio.fluence.voicecall=true \
    persist.vendor.audio.fluence.voicerec=false \
    persist.vendor.audio.fluence.speaker=false

# Bluetooth
PRODUCT_PROPERTY_OVERRIDES += \
    ro.bluetooth.hfp.ver=1.6 \
    ro.qualcomm.bt.hci_transport=smd

# Camera
PRODUCT_PROPERTY_OVERRIDES += \
    camera.disable_zsl_mode=1

# Charger
PRODUCT_PROPERTY_OVERRIDES += \
    ro.charger.enable_suspend=true

# Dalvik/ART
PRODUCT_PROPERTY_OVERRIDES += \
    dalvik.vm.dex2oat-swap=false

# DRM
PRODUCT_PROPERTY_OVERRIDES += \
    drm.service.enabled=true

# Graphics
PRODUCT_PROPERTY_OVERRIDES += \
    ro.hardware.egl=adreno \
    ro.opengles.version=196608 \
    persist.hwc.mdpcomp.enable=true \
    debug.hwui.use_buffer_age=false

# Media
PRODUCT_PROPERTY_OVERRIDES += \
    debug.stagefright.ccodec=4 \
    ro.config.media_vol_steps=25 \
    ro.config.vc_call_vol_steps=7 \
    media.aac_51_output_enabled=true \
    mm.enable.smoothstreaming=true \
    qcom.hw.aac.encoder=true

# RIL
PRODUCT_PROPERTY_OVERRIDES += \
    vendor.rild.libpath=/vendor/lib/libril-qc-qmi-1.so \
    telephony.lteOnCdmaDevice=0 \
    persist.radio.apm_sim_not_pwdn=1 \
    ro.telephony.call_ring.multiple=0

# Sensors
PRODUCT_PROPERTY_OVERRIDES += \
    ro.qc.sensors.wl_dis=true \
    ro.qualcomm.sensors.smd=true

# Wi-Fi
PRODUCT_PROPERTY_OVERRIDES += \
    wifi.interface=wlan0

