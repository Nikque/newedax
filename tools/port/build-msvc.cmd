@echo off
rem Build all.cpp with MSVC as the comparison builds of v4.5.5-nikque.17 (no PGO).
rem usage: build-msvc.cmd <x64|x86> <output exe> [extra cl options]
rem   x64 default options: /D HAS_CPU_64 /arch:AVX512 /vlen=256 /D POPCOUNT (as work\repro\build.cmd)
rem   pass the options of another build after the output name to replace them.
setlocal
set HERE=%~dp0
set ARCH=%~1
set OUT=%~2
if "%OUT%"=="" (
    echo usage: build-msvc.cmd ^<x64^|x86^> ^<output exe^> [extra cl options]
    exit /b 2
)
shift
shift
set EXTRA=
:more
if "%~1"=="" goto go
set EXTRA=%EXTRA% %1
shift
goto more
:go
rem cmd splits the arguments at "=": options with "=" (e.g. /vlen=256) are given in CL_EXTRA instead
if defined CL_EXTRA set EXTRA=%CL_EXTRA%
if "%EXTRA%"=="" if /i "%ARCH%"=="x64" set EXTRA=/D HAS_CPU_64 /arch:AVX512 /vlen=256 /D POPCOUNT
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=%ARCH% -host_arch=x64 >nul
if errorlevel 1 exit /b %errorlevel%
cd /d "%HERE%..\..\src"
cl /nologo /std:c++23preview /Zc:__cplusplus /EHs-c- /D _HAS_EXCEPTIONS=0 /GR- /D UNICODE /utf-8 /D _CRT_SECURE_NO_DEPRECATE /I"..\include" /O2 /fp:fast /GS- /D NDEBUG /MT /GL %EXTRA% all.cpp ws2_32.lib /Fo"%TEMP%\\" /Fe"%OUT%" /link /VERSION:4.5
exit /b %errorlevel%
