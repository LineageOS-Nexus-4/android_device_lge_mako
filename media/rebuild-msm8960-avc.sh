#!/bin/bash
set -euo pipefail
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "Usage: $0 <LineageOS msm8960 media checkout> [output directory]" >&2
    exit 2
fi
reference=$(realpath "$1")
revision=a6a90709117cc790e30935dcb0c681c3f49a53be
media_dir=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$media_dir/../../../.." && pwd)
patch="$root/mako_patches/media/msm8960-avc.patch"
if [ ! -f "$patch" ]; then
    printf 'Missing media patch: %s\n' "$patch" >&2
    exit 1
fi
output=$(realpath -m "${2:-$media_dir/prebuilt}")
d=$(mktemp -d)
trap 'rm -rf "$d"' EXIT
s="$d/legacy-media"
v="$s/mm-video/vidc"
mkdir -p "$s" "$d/kernel-out"
git -C "$reference" archive "$revision" | tar -x -C "$s"
git -C "$s" apply "$patch"
cd "$root"
make -s -C kernel/lge/mako O="$d/kernel-out" ARCH=arm \
    INSTALL_HDR_PATH="$d/legacy-uapi" headers_install
cl=prebuilts/clang/host/linux-x86/clang-r563880c/bin/clang++
display="hardware/qcom/display/msm8960"
args=(--target=armv7a-linux-androideabi30 --sysroot="$root/out/soong/ndk/sysroot" -std=gnu++17 -fno-rtti -fPIC -Wno-error
      -isystem "$root/prebuilts/clang/host/linux-x86/clang-r563880c/include/c++/v1"
      -D_ANDROID_ -D_ANDROID_ICS_ -DMAKO_ONLY_AVC -DMAX_RES_1080P -DMAX_RES_1080P_EBI -DPROCESS_EXTRADATA_IN_OUTPUT_PORT -DUSE_ION
      -I"$v/vdec/inc" -I"$v/common/inc" -I"$s/mm-core/inc" -I"$s/libc2dcolorconvert"
      -I"$display/libgralloc" -I"$display/libcopybit" -I"$display/libqdutils" -I"$display/libqservice"
      -Iframeworks/native/headers/media_plugin -Iframeworks/native/libs/nativewindow/include
      -Iframeworks/native/libs/nativebase/include -Iframeworks/native/libs/binder/include
      -Iframeworks/native/libs/ui/include -Iframeworks/native/libs/gui/include
      -Iframeworks/av/media/libstagefright/include -Iframeworks/av/media/libstagefright/foundation/include
      -Ihardware/libhardware/include -Ihardware/libhardware/include_all -Ihardware/libhardware/include/hardware
      -Isystem/core/libcutils/include -Isystem/core/libutils/include -Isystem/core/libsystem/include
      -Isystem/core/libbase/include -Isystem/libbase/include -Isystem/logging/liblog/include
      -idirafter "$d/legacy-uapi/include")
obj="$d/omx-build"
mkdir -p "$obj"
cc=${cl%++}
inc=(-I"$s/mm-core/src/common" -I"$s/libstagefrighthw")
crt=out/soong/.intermediates/bionic/libc
lib="$root/out/target/product/mako/system/vendor/lib"
link=(-shared --target=armv7a-linux-androideabi30 --sysroot="$root/out/soong/ndk/sysroot"
      -B "$crt/crtbegin_so/android_arm_armv7-a-neon"
      -B "$crt/crtend_so/android_arm_armv7-a-neon"
      -L"$lib" -L"$obj" -Wl,--no-undefined)
"$cl" "${args[@]}" -c "$display/libqdutils/qdMetaData.cpp" -o "$obj/qdMetaData.o"
"$cl" "${link[@]}" -Wl,-soname,libqdMetaData.so -o "$obj/libqdMetaData.so" "$obj/qdMetaData.o" -llog
items=()
for f in "$v/vdec/src/"{frameparser,h264_utils,ts_parser,mp4_utils,omx_vdec}.cpp "$v/common/src/"{extra_data_handler,vidc_color_converter}.cpp; do
    o="$obj/${f##*/}.o"
    "$cl" "${args[@]}" -O2 -c "$f" -o "$o"
    items+=("$o")
done
"$cl" "${link[@]}" -Wl,-soname,libOmxVdec.so -o "$obj/libOmxVdec.so" "${items[@]}" -lqdMetaData -lbinder -lui -lutils -lcutils -llog -ldl
"$cl" "${args[@]}" "${inc[@]}" -O2 -c "$s/mm-core/src/common/omx_core_cmp.cpp" -o "$obj/core_cmp.o"
# C sources do not need the C++ language version flag.
cargs=()
for arg in "${args[@]}"; do if [ "$arg" != -std=gnu++17 ]; then cargs+=("$arg"); fi; done
"$cc" "${cargs[@]}" "${inc[@]}" -O2 -c "$s/mm-core/src/common/qc_omx_core.c" -o "$obj/core_main.o"
"$cc" "${cargs[@]}" "${inc[@]}" -O2 -c "$s/mm-core/src/8960/qc_registry_table_android.c" -o "$obj/core_registry.o"
"$cl" "${link[@]}" -Wl,-soname,libOmxCore.so -o "$obj/libOmxCore.so" "$obj/"core_*.o -llog -ldl
"$cl" "${args[@]}" "${inc[@]}" -O2 -c "$s/libstagefrighthw/QComOMXPlugin.cpp" -o "$obj/plugin.o"
"$cl" "${link[@]}" -Wl,-soname,libstagefrighthw.so -o "$obj/libstagefrighthw.so" "$obj/plugin.o" -lutils -ldl

install -d "$output"
install -m 0644 "$obj/"*.so "$output/"
