#!/usr/bin/env bash
set -euo pipefail

if (( $# < 2 || $# > 3 )); then
    printf 'Usage: %s KERNEL_TREE GCC_PREFIX [OUTPUT]\n' "$0" >&2
    exit 2
fi

device=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)
kernel=$(realpath -- "$1")
toolchain=$(realpath -m -- "$2")
output=$(realpath -m -- "${3:-$device/prebuilt/zImage}")
revision=$(git -C "$kernel" rev-parse --verify HEAD)
short_revision=$(git -C "$kernel" rev-parse --short=8 "$revision")
timestamp=$(git -C "$kernel" show -s --format=%ct "$revision")
[[ -x ${toolchain}gcc ]] || { printf 'Missing compiler: %sgcc\n' "$toolchain" >&2; exit 1; }

scratch=$(mktemp -d)
trap 'rm -rf -- "$scratch"' EXIT
git -C "$kernel" archive "$revision" | tar -x -C "$scratch"

export KBUILD_BUILD_USER=builder KBUILD_BUILD_HOST=mako KBUILD_BUILD_VERSION=1
export KBUILD_BUILD_TIMESTAMP
KBUILD_BUILD_TIMESTAMP=$(date -u -d "@$timestamp" '+%a %b %d %T UTC %Y')
kernel_args=(ARCH=arm "CROSS_COMPILE=$toolchain" "LOCALVERSION=-g$short_revision")
make -C "$scratch" "${kernel_args[@]}" lineageos_mako_defconfig
make -C "$scratch" "${kernel_args[@]}" -j"${JOBS:-8}" zImage
install -m 0644 "$scratch/arch/arm/boot/zImage" "$output"
