#!/system/bin/sh

BLUETOOTH_SLEEP_PATH=/proc/bluetooth/sleep/proto
LOG_TAG="mako-bluetooth"
LOG_NAME="${0}:"

loge ()
{
  /system/bin/log -t $LOG_TAG -p e "$LOG_NAME $@"
}

logi ()
{
  /system/bin/log -t $LOG_TAG -p i "$LOG_NAME $@"
}

failed ()
{
  loge "$1: exit code $2"
  exit $2
}

# Note that "hci_qcomm_init -e" prints expressions to set the shell variables
# BTS_DEVICE, BTS_TYPE, BTS_BAUD, and BTS_ADDRESS.

POWER_CLASS=`getprop qcom.bt.dev_power_class`
TRANSPORT=`getprop ro.qualcomm.bt.hci_transport`
DUTADDR=`getprop net.btdut.address`

#find the transport type
logi "Transport : $TRANSPORT"
logi "DUTADDR : $DUTADDR"

#load bd addr
# mako bring-up: fixed the [ ... ] comparisons (missing spaces made them
# syntax errors, so the -b argument was never passed to hci_qcomm_init
# and the BD_ADDR read from NV failed).
if [ "$DUTADDR" == "" ]
then
BDADDR=`/vendor/bin/bdAddrLoader -f /persist/bluetooth/.bdaddr -h -x`
else
BDADDR=`/vendor/bin/bdAddrLoader -p net.btdut.address -s -x`
fi

setprop vendor.bluetooth.status off

logi "BDADDR: $BDADDR"

case $POWER_CLASS in
  1) PWR_CLASS="-p 0" ;
     logi "Power Class: 1";;
  2) PWR_CLASS="-p 1" ;
     logi "Power Class: 2";;
  3) PWR_CLASS="-p 2" ;
     logi "Power Class: CUSTOM";;
  *) PWR_CLASS="";
     logi "Power Class: Ignored. Default(1) used (1-CLASS1/2-CLASS2/3-CUSTOM)";
     logi "Power Class: To override, Before turning BT ON; setprop qcom.bt.dev_power_class <1 or 2 or 3>";;
esac

# The HIDL 1.0 bluetooth service (bt_vendor) also downloads the firmware
# on power-on and both share /dev/ttyHS0. Retry until we win the race.
RC=1
for attempt in 1 2 3 4 5 6 7 8 9 10; do
  logi "Firmware download attempt $attempt"
  if [ "$BDADDR" == "" ]
  then
  /vendor/bin/hci_qcomm_init -e $PWR_CLASS -vv
  else
  /vendor/bin/hci_qcomm_init -b $BDADDR -e $PWR_CLASS -vv
  fi
  RC=$?
  if [ "$RC" == "0" ]; then
    break
  fi
  logi "Firmware download attempt $attempt failed (rc=$RC), retrying in 2s"
  sleep 2
done

case $RC in
  0) logi "Bluetooth QSoC firmware download succeeded, $PWR_CLASS $BDADDR $TRANSPORT";;
  *) failed "Bluetooth QSoC firmware download failed" $RC;
     setprop vendor.bluetooth.status off;
     exit $RC;;
esac

setprop vendor.bluetooth.status on

logi "start bluetooth smd transport"

exit 0
