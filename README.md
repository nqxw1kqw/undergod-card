# undergod-card — v1.3.1-preview

Game thẻ bài roguelike bằng **C++17 / DxLib / Visual Studio 2022 / Windows x64**. Nhánh draft/v0.3-wip có luật [ba Turn Star](UndergodCard/docs/TURN_STARS.md), avatar đi trên map, chuyển cảnh đen, FIGHT và chia bài có hoạt ảnh. Bản mới sửa icon nhảy về điểm cũ khi tới stage và thêm [36 lá riêng của sáu boss](UndergodCard/docs/BOSS_DECKS.md).

v1.3.1 dùng khung hình/cửa sổ **1600×900 (16:9)**, căn giữa map/menu/sân đấu và thu gọn hàng bài cho chiều cao mới, giữ đúng tỉ lệ sprite. Có [hoạt ảnh riêng theo kỹ năng](UndergodCard/docs/SKILL_ANIMATIONS.md): đâm lao, niệm phép, đạn bay, vuốt/vồ và lớp khiên theo giáp thật của hai phe.

![Ba lớp trong bản DxLib](UndergodCard/docs/images/classes.png)

- Ba hồi 13 / 13 / 15 tầng, map phân nhánh theo seed; sáu boss có cơ chế riêng.
- Xếp ba lá rồi Attack: ta 1 → địch 1 → ta 2 → địch 2 → ta 3 → địch 3. Kế hoạch địch hiện sẵn và có tooltip.
- 50 lá gốc và 48 bản nâng cấp: Tiêu hao, Giữ giáp, Xuyên giáp, năng lực, hồi sinh, lời nguyền.
- 12 sự kiện thực, shop, nghỉ, 17 di vật và sáu thuốc.
- Chiến Binh, Pháp Sư, Ninja; 45 kỹ năng, Lông Vũ lưu giữa run, Thử Thách 1–10 và 15 thành tựu.
- Mỗi boss có bộ sáu lá riêng, chọn theo pha HP; ảnh/tên/chiêu riêng, không vào shop hoặc thưởng người chơi.
- 379 PNG pastel, gồm 36 icon bài boss và chữ FIGHT nền trong suốt; nhạc ba hồi/chín cue, cinematic ba tuyệt kỹ.
- Lưu/tải cả chiến dịch, ba RNG, triệu hồi, năng lực và lịch sử bài; profile riêng.

## Chạy và build

Tải gói Windows tại [Releases](https://github.com/nqxw1kqw/undergod-card/releases), giải nén rồi chạy UndergodCard.exe. Giữ assets, data và licenses cạnh EXE. Gói chơi không cần cài Visual Studio hay SDK.

Để sửa code, mở [UndergodCard.sln](UndergodCard/UndergodCard.sln) bằng VS2022, chọn UndergodCard, Release | x64. Cần bộ công cụ C++ cho desktop, MSVC v143, Windows SDK và DxLib Visual C++ x64 (mặc định C:\DxLib).

    .\UndergodCard\verify.ps1 -Configuration Release
    .\UndergodCard\verify.ps1 -Configuration Debug
    .\UndergodCard\bin\Release\CoreChecks.exe --simulate-stars 300
    .\UndergodCard\package.ps1 -SkipBuild

Các script nhận -DxLibDir. Nhánh vẫn ở [PR #2](https://github.com/nqxw1kqw/undergod-card/pull/2), chưa merge main hoặc PR #1.

## Điều khiển và tiến trình

Chuột / 1–9 chọn lối và xếp bài; bấm lá trong hàng chờ để gỡ. Mỗi lá dùng một sao, đủ ba vị trí thì bấm Attack hoặc E. Có Chờ 1 sao khi thiếu bài. Tab hoặc bấm địch chọn mục tiêu; U tuyệt kỹ. F1/F2/F3 đổi x1/x2/x3, Space/Esc bỏ cinematic; Z/X/C/V dùng thuốc, F10 bật/tắt âm thanh. F6 lưu, F9 tiếp tục; có thể lưu cả hàng bài đang chọn. Các trang bộ bài và map hỗ trợ bánh xe, Page Up/Page Down và nút trang.

profile.json giữ Lông Vũ/kỹ năng/thành tựu; save.json giữ hành trình. Chép cả hai khi chuyển máy. Schema session v2 không tương thích v1 cũ; khi tự mở, game giữ tệp cũ thành .invalid.* trước khi tạo mới.

## Kiểm chứng

Xem [VALIDATION](UndergodCard/docs/VALIDATION.md), [hướng dẫn native](UndergodCard/README.md), [COMPLETION](UndergodCard/docs/COMPLETION.md) và [checklist](UndergodCard/docs/IMPLEMENTATION_CHECKLIST.md).

Mục tiêu 25–35 phút/run và tỉ lệ thắng người chơi mới 40–50% còn cần đo bằng chơi thực. Smoke DxLib kiểm tra renderer và handler UI; không thay kiểm thử nhập liệu chuột/phím Windows.

Repo giữ nguồn native và asset runtime; kho RE, bản web, ảnh nguồn, save, SDK, cache và file build ở ngoài Git.
