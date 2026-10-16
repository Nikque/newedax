@echo off
setlocal
cd /d "%~dp0\.."
where cl >nul 2>&1
if errorlevel 1 (
    echo Run this script from a Visual Studio 2022 x64 Developer Command Prompt.
    exit /b 1
)
cl /nologo /I"..\third_party\boost_1_92_0" /std:c++23preview /Zc:__cplusplus /EHs-c- /D _HAS_EXCEPTIONS=0 /GR- /utf-8 /O1 /MT /DNDEBUG /D_CRT_SECURE_NO_DEPRECATE /D HAS_CPU_64 /arch:AVX512 /vlen=256 /D POPCOUNT /Iinclude tests\regression.cpp ws2_32.lib /Fo:tests\regression.obj /Fe:tests\regression.exe
exit /b %errorlevel%
