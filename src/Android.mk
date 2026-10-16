LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := aEdax # should be renamed to lib..aEdax..so afterwords
LOCAL_CFLAGS += -DUNICODE -DANDROID=1
LOCAL_CPPFLAGS += -std=c++23 -fno-exceptions -fno-rtti
LOCAL_SRC_FILES := all.cpp board_sse.cpp.neon eval_sse.cpp.neon flip_neon_bitscan.cpp.neon
LOCAL_ARM_NEON := true
# cmd-strip :=
include $(BUILD_EXECUTABLE)
