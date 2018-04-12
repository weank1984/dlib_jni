LOCAL_PATH := $(call my-dir)

# =======================================================
include $(CLEAR_VARS)
OpenCV_INSTALL_MODULES := on
OPENCV_CAMERA_MODULES := off
OPENCV_LIB_TYPE := STATIC
include $(OPENCV_PATH)/OpenCV.mk

LOCAL_MODULE := android_dlib

LOCAL_C_INCLUDES +=  \
           $(OPENCV_INCLUDE_DIR)

LOCAL_SRC_FILES += \
           jni_face_ver.cpp \

LOCAL_LDLIBS += -lm -llog -ldl -lz -ljnigraphics
LOCAL_CPPFLAGS += -fexceptions -frtti -std=c++11

# import dlib
LOCAL_STATIC_LIBRARIES += dlib \
                          jni_common


### import miniglog
ifeq ($(MINIGLOG_LIB_TYPE),SHARED)
    LOCAL_SHARED_LIBRARIES += miniglog
else
    LOCAL_STATIC_LIBRARIES += miniglog
endif

# using neon
ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
$(info Enable NEON compile option)
    LOCAL_ARM_MODE := arm
    LOCAL_ARM_NEON := true
endif

ifeq ($(TARGET_ARCH_ABI), arm64-v8a)
$(info Enable NEON compile option)
LOCAL_ARM_MODE := arm
LOCAL_ARM_NEON := true
endif

include $(BUILD_SHARED_LIBRARY)
#-----------------------------------------------------------------------------
