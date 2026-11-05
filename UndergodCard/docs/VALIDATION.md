# Kiểm chứng — 2026-11-05

Phiên bản v1.0.0-preview trên nhánh draft/v0.3-wip. MSVC v143 / Visual Studio 2022 / DxLib x64. Các tệp báo cáo phát sinh nằm trong artifacts hoặc bin/<cấu hình>/artifacts; ảnh minh họa chọn lọc ở docs/images.

## Kết quả tự động

| Kiểm tra | Kết quả |
| --- | --- |
| MSVC Release x64 | Build thành công; 433 kiểm tra logic, 0 lỗi |
| MSVC Debug x64 | Build thành công; 433 kiểm tra logic, 0 lỗi |
| Smoke DxLib Debug và Release | PASS=1; chiến thắng ba hồi, Undergod pha 3 |
| Ảnh runtime | 190/190 nạp đúng kích thước; 50 icon bài và đủ tham chiếu |
| Âm thanh | 12/12 nạp; API phát/đang phát, mute/unmute và nhạc loop qua kiểm tra |
| Asset manifest | Hash PNG/WAV, RGBA8/kích thước và PCM16 mono 22050 Hz hợp lệ |
| Bot độ khó 0 | 1.000 run, 624 thắng, 376 thua, 0 kẹt |
| Thử Thách | Cả ba lớp đến kết quả ở mỗi cấp 1–10, bất lợi cộng dồn được kiểm tra |
| Sinh lại JSON | tools/expand_content.py không làm thay đổi nội dung hiện tại |

Chạy thêm --simulate 1000 có tổng 434 kiểm tra, 0 lỗi, gồm điều kiện bot không kẹt.

## Mô phỏng

Seed 0–999, luân phiên Chiến Binh / Pháp Sư / Ninja, độ khó 0, không mua kỹ năng meta. Bot chọn hành động bằng heuristic tham lam qua API gameplay; chờ mọi animation/cinematic kết thúc trước quyết định kế tiếp. Mỗi run giới hạn 4.000 quyết định.

| Lớp | Run | Thắng | Thua | Kẹt | Tỉ lệ thắng bot |
| --- | --- | --- | --- | --- | --- |
| Chiến Binh | 334 | 150 | 184 | 0 | 44,9% |
| Pháp Sư | 333 | 226 | 107 | 0 | 67,9% |
| Ninja | 333 | 248 | 85 | 0 | 74,5% |
| Tổng | 1000 | 624 | 376 | 0 | 62,4% |

286.039 quyết định, HP khi thắng trung bình 24,55. Thua ở hồi 1/2/3: 198/59/119. Khác biệt giữa lớp là cơ sở cho vòng cân bằng với người chơi thật; không gán tỉ lệ bot cho người chơi mới.

Seed 1337 Chiến Binh đi qua ba hồi, hạ Undergod ở pha 3, còn 30 HP, 41 lá, 29 màn thưởng bài, 201 vàng, bảy di vật. Có bài kiểm tra lưu/tải sau mỗi quyết định ổn định, bao gồm chuyển hồi và thưởng boss.

## Nội dung được kiểm tra

- RNG tương thích giá trị chuẩn cũ và ba stream tách biệt; x1/x2/x3 không đổi kết quả combat.
- Giáp, Sức Mạnh/Yếu/Dễ Vỡ/Độc, nhiều hit, AOE, ngẫu nhiên, xuyên giáp, Tiêu hao, giữ giáp, năng lực, phản đòn, hồi sinh một lần/run.
- Năm boss: summon/khóa bài, lột da/siết/charge, bay/đáp, nguyền/ảo ảnh và sao chép/hút Nộ/Tận Thế.
- 48 nâng cấp có thay đổi thực; ba lớp, Mana, replay, bonus tuyệt kỹ và di vật tương tác.
- 12 sự kiện có kết quả thực; chuỗi hai trận không hồi giữa, giới hạn ba lượt, chọn bài/đổi đồ và thưởng đôi.
- Map 300 seed: nối đường, trọng số, nhịp mở đầu/nghỉ/trùm, 3–4 cột.
- Session/profile round trip; RNG, summon, khóa, history, powers; file sai/truncated, enum/tham chiếu/counter không hợp lệ, lỗi ghi .tmp và chống thưởng lặp.
- Smoke: chọn lớp/kỹ năng/thành tựu; map/cuộn; đánh/đỡ/contact; khóa input; đủ ba cinematic; bỏ qua; shop/nghỉ trang cuối; thuốc thứ tư; lưu/tải; target bằng handler chuột; renderer năm boss.
- Đã xem ảnh render DxLib của menu lớp, cây kỹ năng, boss nhiều mục tiêu và cinematic ba lớp; frame chuẩn hóa alpha/chân/body tại artifacts/art-v1.

## Đóng gói

package.ps1 tạo ZIP từ EXE Release cùng assets/data/licenses và hướng dẫn UTF-8. tools/validate_package.py so manifest/hash với nguồn, CRC toàn ZIP, kiểm tra EXE đúng bản và danh sách tệp chính xác; loại save/profile, PDB, CoreChecks, cache và SDK. Báo cáo hash/kích thước nằm ở artifacts/package-validation.json. Gói giải nén còn được chạy --smoke độc lập trước phát hành.

Gói đã kiểm tra có 226 tệp, 31.408.140 byte, 190 PNG, 12 WAV và 16 JSON dữ liệu. SHA-256 ZIP: 7c3aae9ded7ff8638ede108947fab528d254052cbb278dbd92a53e5154850dbc. EXE: 92ecb46f538af520b99e42782f5650b0f7fcaa2fc56cf4ae6deedfefd78233f6. dumpbin chỉ liệt kê KERNEL32.dll, USER32.dll và ADVAPI32.dll. Các báo cáo chọn lọc được giữ tại [docs/reports](reports/validation.json).

## Giới hạn bằng chứng

Công cụ Computer Use báo native pipe không khả dụng, os error 2 (không tìm thấy tệp). Chưa hoàn thành chơi tay trọn run qua chuột/phím hệ điều hành. Handler UI được smoke gọi trực tiếp; ảnh là renderer thật, không phải bản dựng HTML.

Chưa đo thời lượng 25–35 phút hoặc tỉ lệ thắng 40–50% của người mới. Không coi bot, ảnh render hoặc build xanh là bằng chứng cho các tiêu chí đó. Chưa thử trên một máy Windows sạch khác.

Cảnh báo C4010 đến từ DxFunctionWin.h của SDK ngoài repo.
