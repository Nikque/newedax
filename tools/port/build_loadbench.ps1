# Build loadbench.cpp against the sources of a tree (one logical CPU; MSVC x64 AVX-512, the flags of build-msvc.cmd).
# usage: pwsh -File build_loadbench.ps1 -Tree <tree> -Out <exe> [-Arch x86 -Extra '<cl options>']
# Then: <exe> <book file> <loads> [save file [form]]   (run it with an affinity of one logical CPU)
param([string]$Tree, [string]$Out, [string]$Arch = 'x64', [string]$Extra = '/D HAS_CPU_64 /arch:AVX512 /vlen=256 /D POPCOUNT')
$root = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $PSScriptRoot))
$env:PATH = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer;$env:PATH"
$vsdev = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat'
$cl = "cl /nologo /I$root\third_party\boost_1_92_0 /std:c++23preview /Zc:__cplusplus /EHs-c- /D _HAS_EXCEPTIONS=0 /GR- /D UNICODE /utf-8 /D _CRT_SECURE_NO_DEPRECATE /I$Tree\include /I$Tree\src /O2 /fp:fast /GS- /D NDEBUG /MT /GL $Extra $PSScriptRoot\loadbench.cpp ws2_32.lib /Fo$Out.obj /Fe$Out /link /VERSION:4.5"
$p = Start-Process cmd.exe -ArgumentList '/c', "call `"$vsdev`" -arch=$Arch -host_arch=x64 >nul && $cl" -NoNewWindow -PassThru -RedirectStandardOutput "$Out.log"
$p.ProcessorAffinity = 1; $p.WaitForExit()
"build $Out exit $($p.ExitCode)"; Get-Content "$Out.log" | Select-String ' error ' | Select-Object -First 5
