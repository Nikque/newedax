@echo off
rem Build the 5 Windows PGO targets of src\NMakefile, one after the other.
rem usage: build-pgo.cmd <checkout> <log directory>
rem   <checkout>: a working tree whose bin\ may be overwritten (e.g. a git worktree), with problem\ copied
rem   into it (the profile run solves problem\fforum-20-39.obf; problem\ is not in git).
rem The x86 targets are built from a native x86 environment: the instrumented program needs the x86 pgort.
rem Run it on one logical CPU (the processes inherit the affinity of this one).
setlocal EnableDelayedExpansion
set ROOT=%~1
set LOGS=%~2
if "%LOGS%"=="" (
    echo usage: build-pgo.cmd ^<checkout^> ^<log directory^>
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
for %%t in (vc-pgo-x64-v4:x64 vc-pgo-x64-v3:x64 vc-pgo-x64:x64 vc-pgo-x86-sse:x86 vc-pgo-x86:x86) do (
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
