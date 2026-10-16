APP_ABI := armeabi-v7a arm64-v8a
APP_PLATFORM := android-21
APP_BUILD_SCRIPT := Android-lib.mk
# the C++ standard library is linked into the program (no libc++_shared.so to ship)
APP_STL := c++_static
