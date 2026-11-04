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
@'
UNDERGOD CARD QUEST - C++17 / DxLib / Windows x64

Chạy UndergodCard.exe. Giữ các thư mục assets, data và licenses cạnh EXE.
Không cần Node.js, Electron hay server web.

Chuột hoặc 1–9: chơi thẻ. E: hết lượt. U: Sao Rơi khi đủ 100 Nộ.
Sau trận, 1–3 chọn một lá thưởng. HP và Nộ được giữ sang trận sau.
Space / Esc: bỏ qua cinematic. R / Enter: chơi lại từ đầu khi không bị khóa.

Hành trình: Ếch Con, Cóc Canh, Ếch Tinh Anh, rồi Ếch Vương hai giai đoạn.
Build từ UndergodCard.sln bằng MSVC v143 và static runtime.
Asset: 50 PNG pastel. Dữ liệu: JSON sửa được.
Tốc độ: nút x1/x2/x3 hoặc F1/F2/F3. Run mới bắt đầu ở x1 chậm.
Sao Rơi: gà phóng lao lên trời, lao rơi xuống địch.
Thông báo bên thứ ba: xem thư mục licenses/.
'@ | Set-Content -LiteralPath (Join-Path $bundle 'README.txt') -Encoding UTF8
$archive=Join-Path $nativeRoot 'release/UndergodCard-DxLib-Windows-x64.zip'
$files=@((Join-Path $bundle 'UndergodCard.exe'),(Join-Path $bundle 'assets'),(Join-Path $bundle 'data'),(Join-Path $bundle 'licenses'),(Join-Path $bundle 'README.txt'))
Compress-Archive -LiteralPath $files -DestinationPath $archive -Force
Write-Output $bundle
Write-Output $archive
