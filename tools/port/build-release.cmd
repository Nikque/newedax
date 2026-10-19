@echo off
rem Build the Windows release binaries of src\NMakefile, one after the other:
rem   wEdax-x86-64-v4.exe without PGO (vc-x64-v4: faster than its PGO build, and than the PGO build of the C version),
rem   wEdax-x86-64-v3.exe, wEdax-x86-64.exe, wEdax-x86-sse.exe, wEdax-x86.exe with PGO (vc-pgo-*: without PGO they are
rem   1 to 12 % slower than the PGO builds of the C version in the middle game), and the 3 DLLs without PGO (vc-lib-*).
rem usage: build-release.cmd <checkout> <log directory>
rem   <checkout>: a working tree whose bin\ may be overwritten, with problem\fforum-20-39.obf (the profile runs).
rem The x86 targets are built from a native x86 environment: the instrumented program needs the x86 pgort.
rem Run it on one logical CPU (the processes inherit the affinity of this one): the profile runs use one thread.
setlocal EnableDelayedExpansion
set ROOT=%~1
set LOGS=%~2
if "%LOGS%"=="" (
    echo usage: build-release.cmd ^<checkout^> ^<log directory^>
    exit /b 2
)
if not exist "%ROOT%\problem\fforum-20-39.obf" (
    echo %ROOT%\problem\fforum-20-39.obf is not found.
    exit /b 2
)
if not exist "%LOGS%" mkdir "%LOGS%"
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
set VSDEV="C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
set FAILED=0
for %%t in (vc-x64-v4:x64 vc-pgo-x64-v3:x64 vc-pgo-x64:x64 vc-pgo-x86-sse:x86 vc-pgo-x86:x86 vc-lib-x64-v4:x64 vc-lib-x64-v3:x64 vc-lib-x64:x64) do (
    for /f "tokens=1,2 delims=:" %%a in ("%%t") do (
        echo %%a start !time!
        cmd /c "%VSDEV% -arch=%%b -host_arch=%%b >nul && cd /d "%ROOT%\src" && nmake /nologo -f NMakefile %%a > "%LOGS%\%%a.log" 2>&1"
        set CODE=!errorlevel!
        if not "!CODE!"=="0" set FAILED=1
        echo %%a exit !CODE! !time!
    )
)
echo all done
exit /b %FAILED%
