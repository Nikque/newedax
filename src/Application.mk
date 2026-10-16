APP_ABI := armeabi-v7a arm64-v8a x86 x86_64
APP_PLATFORM := android-14
APP_BUILD_SCRIPT := Android.mk
# the C++ standard library is linked into the program (no libc++_shared.so to ship)
APP_STL := c++_static
