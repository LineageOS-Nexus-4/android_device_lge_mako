# SPDX-License-Identifier: Apache-2.0
"""Refuse ROM writes outside the reviewed 16-GB mako partition profile."""
import json

# Linux sysfs reports these values in 512-byte sectors, with a final newline.
# This is the proposed install profile, not the stock partition table.
_LAYOUT = (
    ("mmcblk0/size", 30777344),
    ("mmcblk0p6/start", 139264),
    ("mmcblk0p6/size", 45056),
    ("mmcblk0p7/start", 184320),
    ("mmcblk0p7/size", 45056),
    ("mmcblk0p21/start", 311296),
    ("mmcblk0p21/size", 4194304),
    ("mmcblk0p22/start", 4505600),
    ("mmcblk0p22/size", 204800),
    ("mmcblk0p26/start", 4710400),
    ("mmcblk0p26/size", 204800),
    ("mmcblk0p23/start", 4915200),
    ("mmcblk0p23/size", 25860096),
)
_NAMES = (
    ("mmcblk0p6", "boot"),
    ("mmcblk0p7", "recovery"),
    ("mmcblk0p21", "system"),
    ("mmcblk0p22", "cache"),
    ("mmcblk0p26", "metadata"),
    ("mmcblk0p23", "userdata"),
)


def _AssertLayout(info):
    for entry, sectors in _LAYOUT:
        path = json.dumps("/sys/class/block/" + entry)
        value = json.dumps(str(sectors) + "\n")
        message = json.dumps(
            "This mako ZIP requires the reviewed 16GB layout: "
            "system 2GiB, cache 100MiB, metadata p26 100MiB. Mismatch: " + entry)
        info.script.AppendExtra(
            "read_file(%s) == %s || abort(%s);" % (path, value, message))
    for part, name in _NAMES:
        path = json.dumps("/sys/class/block/" + part + "/uevent")
        message = json.dumps("Unexpected GPT partition name for " + part + "; expected " + name)
        info.script.AppendExtra(
            'file_getprop(%s, "PARTNAME") == %s || abort(%s);' %
            (path, json.dumps(name), message))
    # Stock p18 is also named metadata. The reviewed migration must remove
    # that duplicate name before recovery can create an unambiguous alias.
    info.script.AppendExtra(
        'file_getprop("/sys/class/block/mmcblk0p18/uevent", "PARTNAME") != "metadata" '
        '|| abort("Old p18 still owns the metadata name; audit GPT before installation.");')


def FullOTA_Assertions(info):
    _AssertLayout(info)


def IncrementalOTA_Assertions(info):
    _AssertLayout(info)
