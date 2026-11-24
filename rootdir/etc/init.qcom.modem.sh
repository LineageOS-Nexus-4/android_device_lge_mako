#!/system/bin/sh
# mako bring-up: gate rild on MDM9615 readiness.
#
# rild (class main) used to probe QMI over HSIC before the modem finished
# booting; QCRIL then declared RADIO_UNAVAILABLE and never recovered, leaving
# the SIM disposed ("SIM absent" despite an inserted card) until a manual
# `setprop ctl.restart vendor.ril-daemon`.
#
# The modem's QMI ctl pipe is NOT a ready signal: /sys/class/hsicctl/hsicctl0
# is created when the rmnet_usb driver registers (early boot), while the
# modem itself finishes booting ("mdm_modem_ioctl: normal boot ... done" in
# dmesg) and enumerates on HSIC USB a few seconds later (rmnet_usb0-3 net
# devices). This script waits for the data netdev (real enumeration), then
# raises vendor.radio.modem_ready so init starts vendor.ril-daemon.
#
# QCRIL's very first card-status query always races the modem's UIM SIM
# detect (~2s after the UIM service connects) and declares RADIO_UNAVAILABLE
# ("card not present"). The shim's ICCID probe then establishes the SIM
# session a couple of seconds later, so a single deterministic rild restart
# after that window gives QCRIL a warm UIM session and the radio comes up
# without intervention. A short safety net bounces rild until the SIM loads.

# Wait for the modem to enumerate on HSIC USB (rmnet_usb data probe).
i=0
while [ ! -L /sys/class/net/rmnet_usb0 ]; do
    i=$((i+1))
    [ "$i" -ge 300 ] && exit 1
    sleep 0.1
done

# Start rild once the modem is on the bus; QCRIL's first init is expected
# to hit the cold SIM-detect race, and the shim warms the session.
sleep 2
setprop vendor.radio.modem_ready 1

# Deterministic restart: QCRIL's very first card-status query always races
# the modem's UIM SIM init (the query itself triggers it, and it takes up
# to ~53s to complete), declaring RADIO_UNAVAILABLE. Bounce rild once after
# that window so the second start finds a warm UIM session.
sleep 55
setprop vendor.radio.modem_restart 1

# Safety net: if the SIM never reports LOADED, restart rild again, up to
# three attempts.
tries=0
while [ "$tries" -lt 3 ]; do
    j=0
    while [ "$j" -lt 40 ]; do
        [ "$(getprop vendor.radio.mako.sim_state)" = "LOADED" ] && exit 0
        sleep 1
        j=$((j+1))
    done
    tries=$((tries+1))
    setprop vendor.radio.modem_restart "$((tries+1))"
done
exit 0
