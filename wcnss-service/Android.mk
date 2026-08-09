LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE := wcnss_service
LOCAL_LICENSE_KINDS := SPDX-license-identifier-BSD
LOCAL_LICENSE_CONDITIONS := notice
LOCAL_SRC_FILES := wcnss_service.c
LOCAL_PROPRIETARY_MODULE := true
LOCAL_SHARED_LIBRARIES := libc libcutils libutils liblog
LOCAL_MODULE_TAGS := optional
LOCAL_CFLAGS += -Wall -Wno-error
include $(BUILD_EXECUTABLE)
