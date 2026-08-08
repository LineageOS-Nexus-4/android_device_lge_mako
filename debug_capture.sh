#!/system/bin/sh
# v3: start markers to /dev/kmsg + writable /metadata (first-stage mount),
# then try /cache. Locked single instance.
echo "debug_capture: start" > /dev/kmsg 2>/dev/null
mkdir -p /metadata 2>/dev/null
echo "debug_capture: started $$ $(date)" > /metadata/boot_kmsg.meta 2>/dev/null
mkdir -p /cache 2>/dev/null
echo "debug_capture: started $$ $(date)" > /cache/boot_kmsg.meta 2>/dev/null
if [ -e /cache/boot_kmsg.lock ]; then
  echo "debug_capture: lock exists, exit" > /dev/kmsg 2>/dev/null
  exit 0
fi
if ! mkdir /cache/boot_kmsg.lock 2>/dev/null; then
  echo "debug_capture: lock busy, exit" > /dev/kmsg 2>/dev/null
  exit 0
fi
echo "debug_capture: lock taken" > /dev/kmsg 2>/dev/null
i=0
while [ $i -lt 60 ]; do
  for dev in /dev/block/mmcblk0p23 /dev/block/platform/msm_sdcc.1/by-name/cache; do
    if [ -b "$dev" ] || [ -f "$dev" ]; then
      mount -t ext4 "$dev" /cache 2>/dev/null
      if grep -q " /cache " /proc/mounts; then
        echo "debug_capture: mounted $dev" > /dev/kmsg 2>/dev/null
        echo "debug_capture: mounted $dev" > /cache/boot_kmsg.meta
        echo "debug_capture: capturing" > /dev/kmsg 2>/dev/null
        exec cat /proc/kmsg >> /cache/boot_kmsg.log
      fi
    fi
  done
  i=$((i+1))
  sleep 2
done
echo "debug_capture: gave up mounting /cache" > /dev/kmsg 2>/dev/null
exit 0
