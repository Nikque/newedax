# Build tests\regression.cpp of a tree with other compiler options and run cases (one logical CPU).
# usage: pwsh -File regression_variant.ps1 -Arch <x64|x86> -Name <name> -Extra '<cl options>' -Cases <case,case...> [-Tree <tree>]
#   e.g. -Arch x64 -Name x64sse2 -Extra '/D HAS_CPU_64' -Cases crc32c,bookform2        (CRC-32C of the table)
#        -Arch x86 -Name x86ia32 -Extra '/arch:IA32 /D EVAL_SLOTS=64' -Cases evalslots   (set EDAX_NO_SSE2=1 first:
#        the code for processors without SSE2)
# The build and the outputs go to <newedax folder>\runs\reg-<name>. -Cpu n: the logical CPU of the build and the cases (default 0).
# -Eval <eval.dat>: the evaluation file of the cases (default bin\data\eval.dat, the one of eval2 with the mobility weights,
# that a build with EVAL_MOBILITY=0 refuses in evalform2: give it the eval.dat of Edax 4.5).
# The exit code is 1 when the build or a case fails.
param([string]$Arch, [string]$Name, [string]$Extra, [string[]]$Cases, [string]$Tree = '', [int]$Cpu = 0, [string]$Eval = '')
$env:REGRESSION_CPU = "$Cpu"
$Cases = @($Cases -split ',')
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$root = Split-Path -Parent $repo
if ($Tree -eq '') { $Tree = $repo }
if ($Eval -eq '') { $Eval = "$repo\bin\data\eval.dat" }
$env:PATH = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer;$env:PATH"
$vsdev = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat'
$w = "$root\runs\reg-$Name"; New-Item -ItemType Directory -Force $w | Out-Null
$exe = "$w\regression.exe"; $log = "$w\build.log"
$cl = "cl /nologo /I$root\third_party\boost_1_92_0 /std:c++23preview /Zc:__cplusplus /EHs-c- /D _HAS_EXCEPTIONS=0 /GR- /utf-8 /O1 /MT /DNDEBUG /D_CRT_SECURE_NO_DEPRECATE $Extra /Iinclude /Isrc tests\regression.cpp ws2_32.lib /Fo:$w\regression.obj /Fe:$exe"
$p = Start-Process cmd.exe -ArgumentList '/c', "call `"$vsdev`" -arch=$Arch -host_arch=x64 >nul && $cl" -WorkingDirectory $Tree -NoNewWindow -PassThru -RedirectStandardOutput $log
$p.ProcessorAffinity = [IntPtr](1L -shl $Cpu); $p.WaitForExit()
"$Name build exit $($p.ExitCode)"; Get-Content $log | Select-String 'error' | Select-Object -First 5
if ($p.ExitCode -ne 0) { exit 1 }
py -3 "$PSScriptRoot\run_regression.py" $exe "$w\run" $Eval "$repo\tests\merge-complete-fixture.dat" @Cases
$code = $LASTEXITCODE
foreach ($c in $Cases) { Get-Content "$w\run\$c.out" | Select-Object -Last 2 }
if ($code -ne 0) { exit 1 }
