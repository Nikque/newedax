LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := aEdax # should be renamed to lib..aEdax..so afterwords
LOCAL_CFLAGS += -DUNICODE -DANDROID=1
LOCAL_CPPFLAGS += -std=c++23 -fno-exceptions -fno-rtti
# the includes are paths from src/
LOCAL_C_INCLUDES += $(LOCAL_PATH)
# Boost (headers only): third_party/boost_1_92_0 next to the repository
LOCAL_C_INCLUDES += $(LOCAL_PATH)/../../third_party/boost_1_92_0
LOCAL_SRC_FILES := all.cpp core/board_sse.cpp.neon eval/eval_sse.cpp.neon core/flip/flip_neon_bitscan.cpp.neon
LOCAL_ARM_NEON := true
# cmd-strip :=
include $(BUILD_EXECUTABLE)
