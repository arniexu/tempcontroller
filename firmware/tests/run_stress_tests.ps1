$ErrorActionPreference = "Stop"

Push-Location $PSScriptRoot
try {
    $rounds = 200
    $pass = 0

    Write-Host "[STRESS] start rounds=$rounds"

    for ($i = 1; $i -le $rounds; $i++) {
        $p = Start-Process -FilePath ".\module_tests.exe" -NoNewWindow -Wait -PassThru
        if ($p.ExitCode -ne 0) {
            throw "[STRESS] module_tests failed at round=$i exit=$($p.ExitCode)"
        }
        $pass++
        if (($i % 20) -eq 0) {
            Write-Host "[STRESS] progress $i/$rounds"
        }
    }

    Write-Host "[STRESS] done pass=$pass"
}
finally {
    Pop-Location
}
