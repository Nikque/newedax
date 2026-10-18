# Quick checks of a change (one logical CPU): builds, fforum-20-39 with one thread against the
# records of v4.5.5-nikque.17, the regression cases, the C API test.
# usage: pwsh -File quick_check.ps1 <output directory> -Inputs <folder of the inputs> [-Asm]
#   -Inputs: the inputs of the comparison of v4.5.5-nikque.16 and 17 (eval.dat, the problem files, ...).
#   -Asm also writes the assembly listing (MSVC x64 AVX-512, /O2 /FAs) to <output directory>\asm\all.asm.
param([Parameter(Mandatory)][string]$Out, [Parameter(Mandatory)][string]$Inputs, [switch]$Asm)

$ErrorActionPreference = 'Continue'
$repo = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$root = Split-Path -Parent $repo
$records = Join-Path $root 'baseline\release17-windows'
$inputs = $Inputs
New-Item -ItemType Directory -Force $Out | Out-Null
$env:PATH = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer;$env:PATH"
$vsdev = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat'
$failed = 0

function Run1([string]$file, [string[]]$arguments, [string]$cwd, [string]$stdout) {
	$p = Start-Process -FilePath $file -ArgumentList $arguments -WorkingDirectory $cwd -NoNewWindow -PassThru -RedirectStandardOutput $stdout
	$p.ProcessorAffinity = 1
	$p.WaitForExit()
	return $p.ExitCode
}

foreach ($c in @(@('x64v4', 'x64', '', 'wEdax-x86-64-v4'), @('x86sse', 'x86', '/D hasSSE2', 'wEdax-x86-sse'))) {
	$exe = Join-Path $Out "edax-$($c[0]).exe"
	$code = Run1 'cmd.exe' @('/c', "$PSScriptRoot\build-msvc.cmd $($c[1]) $exe $($c[2])") $repo (Join-Path $Out "build-$($c[0]).log")
	$warnings = @(Select-String -Path (Join-Path $Out "build-$($c[0]).log") -Pattern 'warning|error' | Where-Object { $_.Line -notmatch 'C4805' })
	if ($code -ne 0) { "$($c[0]): BUILD FAILED"; $warnings | Select-Object -First 5 | ForEach-Object { '  ' + $_.Line }; $failed++; continue }
	if ($warnings.Count) { "$($c[0]): $($warnings.Count) new warnings"; $warnings | Select-Object -First 5 | ForEach-Object { '  ' + $_.Line } }
	$solve = Join-Path $Out "solve-$($c[0]).stdout"
	Run1 $exe @('-n', '1', '-solve', "$inputs\fforum-20-39.obf") (Join-Path $repo 'bin') $solve | Out-Null
	$cmp = py -3 "$PSScriptRoot\compare_solve.py" (Join-Path $records "solve-$($c[3]).exe.stdout") $solve | Select-Object -Last 1
	if ($LASTEXITCODE -ne 0) { $failed++ }
	"$($c[0]): size $((Get-Item $exe).Length), $cmp"
}

$env:EDAX_EVAL_FILE = 'bin\data\eval.dat'
foreach ($s in 'build-regression.cmd', 'build-libedax-test.cmd') {
	$log = Join-Path $Out "$s.log"
	$code = Run1 'cmd.exe' @('/c', "call `"$vsdev`" -arch=x64 -host_arch=x64 >nul && $repo\tests\$s") $repo $log
	if ($code -ne 0) { $failed++ }
	"$s exit $code " + ((Get-Content $log | Select-String 'checks|error C|FAIL') -join ' | ')
}
$r = py -3 "$PSScriptRoot\run_regression.py" (Join-Path $repo 'tests\regression.exe') (Join-Path $Out 'regression') (Join-Path $repo 'bin\data\eval.dat') (Join-Path $repo 'tests\merge-complete-fixture.dat') | Select-Object -Last 1
if ($LASTEXITCODE -ne 0) { $failed++ }
"regression: $r"

if ($Asm) {
	$asmDir = Join-Path $Out 'asm'
	New-Item -ItemType Directory -Force $asmDir | Out-Null
	$flags = '/std:c++23preview /Zc:__cplusplus /EHs-c- /D _HAS_EXCEPTIONS=0 /GR- /D UNICODE /utf-8 /D _CRT_SECURE_NO_DEPRECATE /I..\include /I. /I..\..\third_party\boost_1_92_0 /O2 /fp:fast /GS- /D NDEBUG /MT /D HAS_CPU_64 /arch:AVX512 /vlen=256 /D POPCOUNT'
	$code = Run1 'cmd.exe' @('/c', "call `"$vsdev`" -arch=x64 -host_arch=x64 >nul && cl /nologo $flags /c /FAs /Fa$asmDir\all.asm /Fo$asmDir\all.obj all.cpp") (Join-Path $repo 'src') (Join-Path $asmDir 'build.log')
	"asm: exit $code"
}
"checks failed: $failed"
exit $failed
