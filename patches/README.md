# mako (Nexus 4) AOSP 17 bring-up patches

Cumulative per-repo patches to apply on a STOCK AOSP 17 (trunk_staging) tree.
Apply with `git apply <patch>` from the AOSP root (or `patch -p1`).
Generated 2026-08-16 from the working build VM. All verified with `git apply --check`.

## Repos covered (26 patches)

| Patch | Repo | Commits | Note |
|-------|------|---------|------|
| art.patch | art | 1 | RWX JIT for system server (legacy 3.4 kernel) |
| bionic.patch | bionic | 1 | linker TEXTREL relax (legacy camera blobs) |
| build-make.patch | build/make | 1 | releasetools tolerate missing dynamic_partitions_info.txt |
| device-qcom-sepolicy-legacy.patch | device/qcom/sepolicy-legacy | 2 | SELinux policy fixes for 3.4 (no eBPF/cgv2) |
| external-skia.patch | external/skia | 1 | EGL_IMAGE_PRESERVED_KHR guard |
| frameworks-av.patch | frameworks/av | 4 | YV12 decoder output, audioserver, cameraserver disconnect-flush |
| frameworks-base.patch | frameworks/base | 2 | boot robustness (kernel-configs, boot-phase, hint), SystemUI flashlight |
| frameworks-native.patch | frameworks/native | 2 | gpuwork guard, legacy gralloc mapper only |
| hardware-broadcom-nfc.patch | hardware/broadcom/nfc | 1 | bcm2079x AOSP 17 fixes |
| hardware-interfaces.patch | hardware/interfaces | 4 | BT short-event pad, hwc2on1adapter FB-handle (black-screen fix), audio 2.0 HAL, empty FCM matrix |
| hardware-libhardware.patch | hardware/libhardware | 2 | legacy HAL loading, nfc stub drop |
| hardware-qcom-audio.patch | hardware/qcom/audio | 7 | msm8960 audio HAL AOSP 17 fixes (configs, power-ndk, UAPI headers) |
| hardware-qcom-bt.patch | hardware/qcom/bt | 2 | WCN3660 BT over SMD build, firmware-download race |
| hardware-qcom-display.patch | hardware/qcom/display | 12 | msm8960 display HAL AOSP 17 fixes (VNDK, ion heap_mask, headers) |
| hardware-qcom-wlan.patch | hardware/qcom/wlan | 2 | qcwcn non-QMI wcnss-service, soong wpa_supplicant |
| hardware-ril.patch | hardware/ril | 2 | legacy string-array data call responses |
| packages-apps-Camera2.patch | packages/apps/Camera2 | 1 | stopRepeating before abortCaptures |
| packages-modules-Bluetooth.patch | packages/modules/Bluetooth | 1 | gd/hci tolerate old-controller (WCN3660) |
| packages-modules-Connectivity.patch | packages/modules/Connectivity | 5 | no-eBPF guards (BpfMap, event polling, Clat, BpfNetMaps, NetworkStats) |
| packages-modules-DnsResolver.patch | packages/modules/DnsResolver | 1 | no-abort when cgroup-v2 unavailable |
| system-apex.patch | system/apex | 1 | ext4 apex payloads, skip verity check |
| system-core.patch | system/core | 2 | cgroup-v2 stub (libprocessgroup), debuggerd tombstone find() |
| system-linkerconfig.patch | system/linkerconfig | 1 | legacy camera HAL platform-lib namespace |
| system-memory-libmeminfo.patch | system/memory/libmeminfo | 1 | guard sysmeminfo BPF/GPU paths |
| system-netd.patch | system/netd | 2 | tolerate BPF-less kernels + missing cgroup v2 |
| system-server_configurable_flags.patch | system/server_configurable_flags | 1 | stub flags health check |

## Not patched (covered elsewhere / not needed)

- **device/lge/mako, kernel/lge/mako, vendor/lge** - maintained as separate repos by the user.
- **frameworks/opt/telephony** - net diff is EMPTY (all mako RIL patches were reverted at the tip;
  the radio shim lives in device/lge/mako).
- **Arm64 host changes** (prebuilts/siso, build-tools, jdk, build/soong, build/make host tweaks) - skipped intentionally.
- **external/python/mako** - template library name match only, no changes.
- **build/release** - restored, no changes.

## On-device extras (not patches)

- Repartition tools: `repartition-tools/` (resize system to 2 GiB + metadata p26).
- The device tree must mount /metadata from `by-name/metadata` and size system at 2 GiB
  (see the device repo's BoardConfig.mk / fstab.mako).
- `prebuilt/zImage`: the custom 3.4 kernel - built separately (kernel repo).
