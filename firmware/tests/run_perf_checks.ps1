$ErrorActionPreference = "Stop"

Push-Location $PSScriptRoot
try {
    if (!(Test-Path ".\module_tests.exe")) {
        throw "module_tests.exe not found, run run_tests.ps1 first"
    }

    $samples = 20
    $durations = @()

    Write-Host "[PERF] start samples=$samples"

    for ($i = 1; $i -le $samples; $i++) {
        $sw = [System.Diagnostics.Stopwatch]::StartNew()
        $p = Start-Process -FilePath ".\module_tests.exe" -NoNewWindow -Wait -PassThru
        $sw.Stop()

        if ($p.ExitCode -ne 0) {
            throw "[PERF] module_tests failed at sample=$i exit=$($p.ExitCode)"
        }

        $durations += $sw.ElapsedMilliseconds
    }

    $avg = [Math]::Round((($durations | Measure-Object -Average).Average), 2)
    $max = ($durations | Measure-Object -Maximum).Maximum
    $min = ($durations | Measure-Object -Minimum).Minimum

    Write-Host "[PERF] min_ms=$min avg_ms=$avg max_ms=$max"
}
finally {
    Pop-Location
}
