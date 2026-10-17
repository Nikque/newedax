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
	$map = "$Out\$t.map"	# evsearch: the link map, to check the places of its hot code
	if ($t -eq 'evsearch') { if ($env:LINK -match '/MAP:(\S+)') { $map = $Matches[1] } else { $cl += " /link /MAP:`"$map`"" } }
	$p = Start-Process cmd.exe -ArgumentList '/c', "call `"$vsdev`" -arch=x64 -host_arch=x64 >nul && $cl" -NoNewWindow -PassThru -RedirectStandardOutput "$Out\$t.log"
	try { $p.ProcessorAffinity = [IntPtr](1L -shl $Cpu) } catch {}
	$p.WaitForExit()
	$warn = @(Get-Content "$Out\$t.log" | Select-String ' (error|warning) ' | Where-Object { $_.Line -notmatch 'C4805' } | ForEach-Object { $_.Line })
	if ($t -eq 'evsearch' -and $p.ExitCode -eq 0) {	# the places of the hot code ("Places of the hot code" in evsearch.cpp)
		$lines = if (Test-Path $map) { Get-Content $map } else { @() }
		foreach ($w in @(@('?apply_base$omp$1@', 4096, 0), @('?features@', 64, 48), @('?run_config$omp$1@', 64, 0), @('?load@', 64, 48))) {
			$addr = $null
			foreach ($l in $lines) {
				$f = $l.Trim() -split '\s+'
				if ($f.Count -ge 3 -and $f[0].StartsWith('0001:') -and $f[1].StartsWith($w[0])) { $addr = [Convert]::ToUInt64($f[2], 16); break }
			}
			if ($null -eq $addr) { $warn += "warning: evsearch: $($w[0]) not found in $map" }
			elseif (($addr % $w[1]) -ne $w[2]) { $warn += "warning: evsearch: $($w[0]) at $('{0:x}' -f $addr), not at $($w[2]) modulo $($w[1]) (the pads of the places of the hot code in evsearch.cpp)" }
		}
	}
	"build $t exit $($p.ExitCode) warnings/errors $($warn.Count)"
	$warn | Select-Object -First 8 | ForEach-Object { "  $($_.Trim())" }
	if ($p.ExitCode -ne 0) { $failed++ }
}
"failed: $failed"
