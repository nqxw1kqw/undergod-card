param([switch]$SkipBuild,[string]$DxLibDir='C:\DxLib')
$ErrorActionPreference='Stop'
if(!$SkipBuild) { & (Join-Path $PSScriptRoot 'build.ps1') -Configuration Release -DxLibDir $DxLibDir }
$nativeRoot=(Resolve-Path -LiteralPath $PSScriptRoot).Path
$bundle=[IO.Path]::GetFullPath((Join-Path $nativeRoot 'release/UndergodCard-DxLib'))
if(!$bundle.StartsWith($nativeRoot+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Bundle outside project' }
New-Item -ItemType Directory -Path $bundle -Force | Out-Null
foreach($folder in @('assets','data','licenses')) {
    $target=[IO.Path]::GetFullPath((Join-Path $bundle $folder))
    if(!$target.StartsWith($bundle+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Bundle target outside release folder' }
    if(Test-Path -LiteralPath $target) { Remove-Item -LiteralPath $target -Recurse -Force }
}
Copy-Item -LiteralPath (Join-Path $nativeRoot 'bin/Release/UndergodCard.exe') -Destination $bundle -Force
Copy-Item -LiteralPath (Join-Path $nativeRoot 'assets') -Destination $bundle -Recurse -Force
Copy-Item -LiteralPath (Join-Path $nativeRoot 'data') -Destination $bundle -Recurse -Force
$licenses=Join-Path $bundle 'licenses'
New-Item -ItemType Directory -Path $licenses -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $nativeRoot 'third_party/nlohmann/LICENSE.MIT') -Destination (Join-Path $licenses 'nlohmann-json-MIT.txt') -Force
Copy-Item -LiteralPath (Join-Path $nativeRoot 'third_party/DxLib-notice.txt') -Destination $licenses -Force
Copy-Item -LiteralPath (Join-Path $nativeRoot 'THIRD_PARTY.md') -Destination $licenses -Force
Copy-Item -LiteralPath (Join-Path $nativeRoot 'docs/PLAYER_GUIDE.txt') -Destination (Join-Path $bundle 'README.txt') -Force
$archive=Join-Path $nativeRoot 'release/UndergodCard-DxLib-Windows-x64.zip'
$files=@((Join-Path $bundle 'UndergodCard.exe'),(Join-Path $bundle 'assets'),(Join-Path $bundle 'data'),(Join-Path $bundle 'licenses'),(Join-Path $bundle 'README.txt'))
Compress-Archive -LiteralPath $files -DestinationPath $archive -Force
$checksum=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant()+'  '+[IO.Path]::GetFileName($archive)+[Environment]::NewLine
[IO.File]::WriteAllText($archive+'.sha256',$checksum,[Text.UTF8Encoding]::new($false))
Write-Output $bundle
Write-Output $archive
