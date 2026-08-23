#
# Copyright 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# This contains the module build definitions for the hardware-specific
# components for this device.
#
# As much as possible, those components should be built unconditionally,
# with device-specific names to avoid collisions, to avoid device-specific
# bitrot and build breakages. Building a component unconditionally does
# *not* include it on all devices, so it is safe even with hardware-specific
# components.

ifneq ($(filter mako occam,$(TARGET_DEVICE)),)

LOCAL_PATH := $(call my-dir)

MAKO_DEVICE_PATH := $(LOCAL_PATH)

include $(call first-makefiles-under,$(LOCAL_PATH))

# -------------------------------------------------------------------
# mako has no vendor partition: vendor lives in /system/vendor.
# Provide a /vendor symlink in the ramdisk so that init and its
# imports (/vendor/etc/init/hw/init.mako.rc) resolve correctly.
# -------------------------------------------------------------------
$(TARGET_ROOT_OUT)/vendor: $(MAKO_DEVICE_PATH)/Android.mk
	@mkdir -p $(dir $@)
	rm -rf $@
	ln -sf /system/vendor $@

ALL_DEFAULT_INSTALLED_MODULES += $(TARGET_ROOT_OUT)/vendor

endif
