param(
    [ValidateSet('Debug','Release')][string]$Configuration='Release',
    [string]$DxLibDir='C:\DxLib'
)
$ErrorActionPreference='Stop'
& python (Join-Path $PSScriptRoot 'tools/validate_assets.py')
if ($LASTEXITCODE -ne 0) { throw 'Asset validation that bai.' }
& (Join-Path $PSScriptRoot 'build.ps1') -Configuration $Configuration -DxLibDir $DxLibDir
& (Join-Path $PSScriptRoot "bin\$Configuration\CoreChecks.exe")
if ($LASTEXITCODE -ne 0) { throw 'CoreChecks that bai.' }
$p=Start-Process -FilePath (Join-Path $PSScriptRoot "bin\$Configuration\UndergodCard.exe") -ArgumentList '--smoke' -WindowStyle Hidden -PassThru
if (!$p.WaitForExit(60000)) { Stop-Process -Id $p.Id; throw 'Smoke test qua 60 giay.' }
$report=Join-Path $PSScriptRoot "bin\$Configuration\artifacts\smoke.txt"
$reportText=Get-Content -LiteralPath $report -Raw
Write-Output $reportText
if ($p.ExitCode -ne 0) { throw "Smoke test that bai: $($p.ExitCode)" }
if ($reportText -notmatch 'playthrough_victory=1') { throw 'Auto-playthrough khong thang hanh trinh.' }
if ($reportText -notmatch 'boss_phase=3') { throw 'Undergod khong vao giai doan 3.' }
$stars=Start-Process -FilePath (Join-Path $PSScriptRoot "bin\$Configuration\UndergodCard.exe") -ArgumentList '--star-smoke' -WindowStyle Hidden -PassThru
if (!$stars.WaitForExit(60000)) { Stop-Process -Id $stars.Id; throw 'Turn Star smoke qua 60 giay.' }
$starReport=Get-Content -LiteralPath (Join-Path $PSScriptRoot "bin\$Configuration\artifacts\turn-stars\smoke.txt") -Raw
Write-Output $starReport
if ($stars.ExitCode -ne 0 -or $starReport -notmatch 'PASS=1') { throw 'Turn Star DxLib smoke that bai.' }
