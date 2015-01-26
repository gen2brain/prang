LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

LOCAL_CFLAGS := -g -D__ANDROID__ -DAUDIO_ENABLED $(LOCAL_CFLAGS)

SDL_PATH := ../SDL2

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include \
	$(LOCAL_PATH)/SDL2_image \
	$(LOCAL_PATH)/SDL2_mixer

FILE_LIST := $(wildcard $(LOCAL_PATH)/*.c)
LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c $(FILE_LIST:$(LOCAL_PATH)/%=%)

LOCAL_SHARED_LIBRARIES := SDL2 SDL2_image SDL2_mixer

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
