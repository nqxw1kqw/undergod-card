param(
    [ValidateSet('Debug','Release')][string]$Configuration='Release',
    [string]$DxLibDir='C:\DxLib'
)
$ErrorActionPreference='Stop'
& (Join-Path $PSScriptRoot 'build.ps1') -Configuration $Configuration -DxLibDir $DxLibDir
& (Join-Path $PSScriptRoot "bin\$Configuration\CoreChecks.exe")
if ($LASTEXITCODE -ne 0) { throw 'CoreChecks that bai.' }
$p=Start-Process -FilePath (Join-Path $PSScriptRoot "bin\$Configuration\UndergodCard.exe") -ArgumentList '--smoke' -WindowStyle Hidden -PassThru
if (!$p.WaitForExit(30000)) { Stop-Process -Id $p.Id; throw 'Smoke test qua 30 giay.' }
$report=Join-Path $PSScriptRoot "bin\$Configuration\artifacts\smoke.txt"
Get-Content -LiteralPath $report
if ($p.ExitCode -ne 0) { throw "Smoke test that bai: $($p.ExitCode)" }
