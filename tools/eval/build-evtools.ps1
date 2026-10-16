# Build the evaluation tools of tools\eval with MSVC (x64 AVX-512, the options of the C tools: /O2 /MT, no /fp:fast),
# one tool after another on one logical CPU.
# usage: pwsh -File build-evtools.ps1 -Out <directory> [-Tools evsearch,w2dat2,...] [-Tree <source tree>] [-Extra '<cl options>'] [-Cpu n]
#   tools that include the search (all.cpp of -Tree, default: this repository) are built with /openmp, the options of
#   the x64 AVX-512 build (HAS_CPU_64, POPCOUNT) and EVAL_MOBILITY=1 (they read the mobility weights of eval2);
#   leafgen also with EVAL_LEAF_HOOK. -Extra adds options to every tool.
param([string]$Out, [string]$Tools = '', [string]$Tree = '', [string]$Extra = '', [int]$Cpu = 0)
$here = $PSScriptRoot
$root = Split-Path -Parent (Split-Path -Parent $here)
if (-not $Tree) { $Tree = $root }
$boost = Join-Path (Split-Path -Parent $root) 'third_party\boost_1_92_0'
if (-not (Test-Path "$boost\boost\version.hpp")) { $boost = 'D:\newedax\third_party\boost_1_92_0' }	# (a work tree under runs)
$all = @('evsearch', 'evtrain', 'w2dat2', 'sprepair', 'sample', 'bookdump', 'bookscan', 'selfplay3', 'label', 'playout', 'gen', 'leafgen', 'evcheck', 'trans2rec')
$list = if ($Tools) { $Tools -split ',' } else { $all }
$omp = @('evsearch', 'evtrain', 'selfplay3', 'label', 'playout', 'gen', 'leafgen')
$engine = @('selfplay3', 'label', 'playout', 'gen', 'leafgen', 'evcheck', 'trans2rec')
$vsdev = 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat'
$env:PATH = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer;$env:PATH"
New-Item -ItemType Directory -Force $Out | Out-Null
$base = '/nologo /std:c++23preview /Zc:__cplusplus /EHs-c- /D _HAS_EXCEPTIONS=0 /GR- /utf-8 /O2 /MT /DNDEBUG /D_CRT_SECURE_NO_DEPRECATE /arch:AVX512'
$failed = 0
foreach ($t in $list) {
	$src = Join-Path $here "$t.cpp"
	if (-not (Test-Path $src)) { "no source $src"; $failed++; continue }
	$o = $base
	if ($omp -contains $t) { $o += ' /openmp' }
	$libs = ''
	if ($engine -contains $t) {
		$o += " /I`"$boost`" /I`"$Tree\include`" /I`"$Tree\src`" /D HAS_CPU_64 /vlen=256 /D POPCOUNT /D EVAL_MOBILITY=1"
		if ($t -eq 'leafgen') { $o += ' /D EVAL_LEAF_HOOK' }
		$libs = 'ws2_32.lib psapi.lib'
	}
	$obj = Join-Path $Out "obj-$t"
	New-Item -ItemType Directory -Force $obj | Out-Null
	$cl = "cl $o $Extra /I`"$here`" `"$src`" $libs /Fo`"$obj\\`" /Fe`"$Out\$t.exe`""
	$p = Start-Process cmd.exe -ArgumentList '/c', "call `"$vsdev`" -arch=x64 -host_arch=x64 >nul && $cl" -NoNewWindow -PassThru -RedirectStandardOutput "$Out\$t.log"
	try { $p.ProcessorAffinity = [IntPtr](1L -shl $Cpu) } catch {}
	$p.WaitForExit()
	$warn = @(Get-Content "$Out\$t.log" | Select-String ' (error|warning) ' | Where-Object { $_.Line -notmatch 'C4805' })
	"build $t exit $($p.ExitCode) warnings/errors $($warn.Count)"
	$warn | Select-Object -First 8 | ForEach-Object { "  $($_.Line.Trim())" }
	if ($p.ExitCode -ne 0) { $failed++ }
}
"failed: $failed"
