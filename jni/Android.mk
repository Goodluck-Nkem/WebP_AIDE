LOCAL_PATH := $(call my-dir)

# ==========================================================
# libsharpyuv.so  (PREBUILT)
# ==========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := sharpyuv
LOCAL_SRC_FILES := prebuilt/$(TARGET_ARCH_ABI)/libsharpyuv.so
include $(PREBUILT_SHARED_LIBRARY)

# ==========================================================
# libwebp.so  (PREBUILT)
# ==========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := webp
LOCAL_SRC_FILES := prebuilt/$(TARGET_ARCH_ABI)/libwebp.so
include $(PREBUILT_SHARED_LIBRARY)

# ==========================================================
# libwebpdemux.so  (PREBUILT)
# ==========================================================
include $(CLEAR_VARS)
LOCAL_MODULE := webpdemux
LOCAL_SRC_FILES := prebuilt/$(TARGET_ARCH_ABI)/libwebpdemux.so
include $(PREBUILT_SHARED_LIBRARY)

# ==========================================================
# My JNI library built from Source
# ==========================================================
include $(CLEAR_VARS)

LOCAL_MODULE    := hello-jni
LOCAL_SRC_FILES := hello-jni.c

LOCAL_LDLIBS := -llog -ljnigraphics

# Link to the three prebuilt libraries
LOCAL_SHARED_LIBRARIES := sharpyuv webp webpdemux

ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_CFLAGS += -ffast-math -mtune=atom -mssse3 -mfpmath=sse
endif

include $(BUILD_SHARED_LIBRARY)

