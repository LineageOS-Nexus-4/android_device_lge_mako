LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := nltest
LOCAL_SRC_FILES := nltest.c
LOCAL_CFLAGS += -Wall
LOCAL_MULTILIB := 32
include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)
LOCAL_MODULE := rtest
LOCAL_SRC_FILES := rtest.c
LOCAL_CFLAGS += -Wall
LOCAL_MULTILIB := 32
include $(BUILD_EXECUTABLE)
include $(CLEAR_VARS)
LOCAL_MODULE := nldump
LOCAL_SRC_FILES := nldump.c
LOCAL_MULTILIB := 32
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_MODULE := atone
LOCAL_SRC_FILES := atone.cpp
LOCAL_SHARED_LIBRARIES := libaudioclient libcutils libutils liblog libbinder libbinder_ndk
LOCAL_STATIC_LIBRARIES := framework-permission-aidl-cpp
LOCAL_C_INCLUDES += \
    $(TOP_DIR)frameworks/av/media/libaudioclient/include \
    $(TOP_DIR)frameworks/av/media/libaudiofoundation/include \
    $(TOP_DIR)frameworks/av/media/libmedia/include
LOCAL_CFLAGS += -Wno-unused-variable
LOCAL_MULTILIB := 32
include $(BUILD_EXECUTABLE)
