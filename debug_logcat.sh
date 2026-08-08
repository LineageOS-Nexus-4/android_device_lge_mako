#!/system/bin/sh
echo "debug_logcat: start" > /dev/kmsg 2>/dev/null
while :; do
  mkdir -p /data/debug 2>/dev/null
  logcat -b all -d > /data/debug/boot.log 2>> /data/debug/boot.log.err
  sleep 10
done
