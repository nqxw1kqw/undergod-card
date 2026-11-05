# undergod-card — v1.0.1-preview

Game thẻ bài roguelike bằng **C++17 / DxLib / Visual Studio 2022 / Windows x64**. Nhánh draft/v0.3-wip hiện triển khai các mốc v0.4, v0.5 và phần hoàn thiện trong kế hoạch.

![Ba lớp trong bản DxLib](UndergodCard/docs/images/classes.png)

- Ba hồi 13 / 13 / 15 tầng, map phân nhánh theo seed; sáu boss có cơ chế riêng.
- 50 lá gốc và 48 bản nâng cấp: Tiêu hao, Giữ giáp, Xuyên giáp, năng lực, hồi sinh, lời nguyền.
- 12 sự kiện thực, shop, nghỉ, 17 di vật và sáu thuốc.
- Chiến Binh, Pháp Sư, Ninja; 45 kỹ năng, Lông Vũ lưu giữa run, Thử Thách 1–10 và 15 thành tựu.
- 202 PNG pastel, nhạc ba hồi và chín cue âm thanh; cinematic riêng cho ba tuyệt kỹ.
- Lưu/tải cả chiến dịch, ba RNG, triệu hồi, năng lực và lịch sử bài; profile riêng.

## Chạy và build

Tải gói Windows tại [Releases](https://github.com/nqxw1kqw/undergod-card/releases), giải nén rồi chạy UndergodCard.exe. Giữ assets, data và licenses cạnh EXE. Gói chơi không cần cài Visual Studio hay SDK.

Để sửa code, mở [UndergodCard.sln](UndergodCard/UndergodCard.sln) bằng VS2022, chọn UndergodCard, Release | x64. Cần bộ công cụ C++ cho desktop, MSVC v143, Windows SDK và DxLib Visual C++ x64 (mặc định C:\DxLib).

    .\UndergodCard\verify.ps1 -Configuration Release
    .\UndergodCard\verify.ps1 -Configuration Debug
    .\UndergodCard\bin\Release\CoreChecks.exe --simulate 1000
    .\UndergodCard\package.ps1 -SkipBuild

Các script nhận -DxLibDir. Nhánh vẫn ở [PR #2](https://github.com/nqxw1kqw/undergod-card/pull/2), chưa merge main hoặc PR #1.

## Điều khiển và tiến trình

Chuột / 1–9 chọn lối và chơi bài; Tab hoặc bấm địch chọn mục tiêu. E hết lượt, U tuyệt kỹ, F1/F2/F3 đổi x1/x2/x3, Space/Esc bỏ cinematic. Z/X/C/V dùng thuốc; F10 bật/tắt âm thanh. F6 lưu, F9 tiếp tục. R/Enter/Chơi lại mở chọn lớp. Các trang bộ bài và map hỗ trợ bánh xe, Page Up/Page Down và nút trang.

profile.json giữ Lông Vũ/kỹ năng/thành tựu; save.json giữ hành trình. Chép cả hai khi chuyển máy. Schema session v2 không tương thích v1 cũ; khi tự mở, game giữ tệp cũ thành .invalid.* trước khi tạo mới.

## Kiểm chứng

Xem [VALIDATION](UndergodCard/docs/VALIDATION.md), [hướng dẫn native](UndergodCard/README.md), [COMPLETION](UndergodCard/docs/COMPLETION.md) và [checklist](UndergodCard/docs/IMPLEMENTATION_CHECKLIST.md).

Mục tiêu 25–35 phút/run và tỉ lệ thắng người chơi mới 40–50% còn cần đo bằng chơi thực. Smoke DxLib kiểm tra renderer và handler UI; không thay kiểm thử nhập liệu chuột/phím Windows.

Repo giữ nguồn native và asset runtime; kho RE, bản web, ảnh nguồn, save, SDK, cache và file build ở ngoài Git.
