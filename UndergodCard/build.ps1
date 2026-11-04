param(
    [ValidateSet('Debug','Release')][string]$Configuration='Release',
    [string]$DxLibDir='C:\DxLib'
)
$ErrorActionPreference='Stop'
$vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Can Visual Studio 2022 voi Desktop development with C++.' }
$msbuild=& $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (!$msbuild) { throw 'Khong tim thay MSBuild.' }
if (!(Test-Path -LiteralPath (Join-Path $DxLibDir 'DxLib.h'))) { throw "Khong tim thay DxLib.h trong $DxLibDir" }
& $msbuild (Join-Path $PSScriptRoot 'UndergodCard.sln') "/p:Configuration=$Configuration" /p:Platform=x64 "/p:DxLibDir=$DxLibDir" /m /nologo /v:minimal
if ($LASTEXITCODE -ne 0) { throw "Build that bai: $LASTEXITCODE" }
Write-Output (Join-Path $PSScriptRoot "bin\$Configuration\UndergodCard.exe")
