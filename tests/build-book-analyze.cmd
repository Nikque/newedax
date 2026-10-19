@echo off
setlocal
cd /d "%~dp0\.."
where cl >nul 2>&1
if errorlevel 1 (
    echo Run this script from a Visual Studio 2022 x64 Developer Command Prompt.
    exit /b 1
)
cl /nologo /I"..\third_party\boost_1_92_0" /std:c++23preview /Zc:__cplusplus /EHs-c- /D _HAS_EXCEPTIONS=0 /GR- /utf-8 /O2 /MT /DNDEBUG /D_CRT_SECURE_NO_DEPRECATE /D HAS_CPU_64 /arch:AVX2 /D POPCOUNT /Iinclude /Isrc tests\book-analyze.cpp ws2_32.lib /Fo:tests\book-analyze.obj /Fe:tests\book-analyze.exe
exit /b %errorlevel%
