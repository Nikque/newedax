LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
# Library (libedax): Edax as a shared library with the api of libedax.h -> libs-lib/<abi>/libedax.so
#   ndk-build -C src NDK_PROJECT_PATH=. NDK_APPLICATION_MK=./Application-lib.mk NDK_OUT=./obj-lib NDK_LIBS_OUT=./libs-lib
LOCAL_MODULE := edax
LOCAL_CFLAGS += -DUNICODE -DANDROID=1 -DLIB_BUILD
LOCAL_CPPFLAGS += -std=c++23 -fno-exceptions -fno-rtti
# Boost (headers only): third_party/boost_1_92_0 next to the repository
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../third_party/boost_1_92_0
# the library calls its own functions, even when the program has functions with the same names (as the Linux library)
LOCAL_LDFLAGS += -Wl,-Bsymbolic
LOCAL_SRC_FILES := all.cpp board_sse.cpp.neon eval_sse.cpp.neon flip_neon_bitscan.cpp.neon
LOCAL_ARM_NEON := true
include $(BUILD_SHARED_LIBRARY)
