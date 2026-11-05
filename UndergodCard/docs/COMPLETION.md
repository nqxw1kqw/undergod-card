# Hoàn thiện kế hoạch — v1.0.0-preview

2026-11-05. Giữ nhánh draft/v0.3-wip và PR #2; chưa merge main/PR #1. Các hệ thống trong mốc v0.4/v0.5/v1.0 đã có triển khai. Hai tiêu chí cần chơi thực để mở trong IMPLEMENTATION_CHECKLIST.

- Ba hồi phân nhánh 13/13/15 tầng, phần thưởng/hồi HP boss, năm boss với ngưỡng HP và cơ chế riêng.
- Nhiều mục tiêu, chọn mục tiêu, AOE/ngẫu nhiên, summon, multi-hit và charge.
- 50 lá gốc +48 nâng cấp có thay đổi thực/mô tả effect; đủ bảng bài thiết kế và các từ khóa.
- Đủ 12 sự kiện có cơ chế: chọn lá, đổi di vật, xiếc ba lượt, hai trận phục kích nối tiếp, cầu tinh anh thưởng đôi, che map, khế ước và các đánh đổi.
- 17 di vật gồm đủ mười cơ chế thiết kế; sáu thuốc; thưởng 60/33/7 theo tầng hiếm, tinh anh Hiếm+, di vật không trùng.
- Ba lớp/bộ khởi đầu/tuyệt kỹ; 45 kỹ năng; tiền meta; Thử Thách cộng dồn 1–10; 15 thành tựu.
- Session v2/profile v1, ba RNG, sổ nhận thưởng chống trùng, ghi nguyên tử, kiểm tra đường map/pile/tham chiếu.
- 190 PNG: ảnh riêng cho 50 bài, ba lớp, năm boss, địch thường/tinh anh và đồ. Giữ gà Chiến Binh đã được Shin duyệt làm chuẩn; bộ ếch dùng chung cũ đã ra khỏi runtime.
- Bão Lông/Ảnh Phân có cinematic riêng; Sao Rơi giữ phóng lao lên trời/rơi đúng contact. Nhạc ba hồi, chín cue PCM gốc và tắt/bật lưu trong profile.
- UI lớp/kỹ năng/thành tựu/Thử Thách/mục tiêu/khóa bài/Mana/dự báo/thưởng boss.

## Quyết định cụ thể

Boss chuyển pha theo HP hiện có, không nạp lại HP. Bóng Cáo một HP sẽ vỡ khi trúng, UI che HP/tên thật; hạ con thật làm tan bóng. Nòng Nọc còn sống tiếp tục trận sau khi Ếch Vương chết.

Mắt Diều Hâu mô phỏng trên bản sao hai lượt nếu không chơi thêm bài, không tiêu RNG thật; hành động có thể đổi dự báo. Undergod chuyển tài nguyên riêng của player thành Sức Mạnh/hút Nộ khi sao chép.

Địch thường có ảnh riêng và chuyển động code; không nhân bản ảnh thành animation giả. Boss có bốn idle/bốn attack. Thiếu hit/defend dùng idle kèm recoil/flash/khiên.

Raw/QC ở artifacts/art-v1 ngoài Git. Công cụ import chỉ tách grid/chroma key/alpha/căn frame; bitmap do ImageGen tạo. Audio từ tools/synth_audio.py, không dùng bản thu bên ngoài.

## Cần chơi thực để xác nhận

Helper Computer Use vẫn lỗi native pipe, os error 2 (không tìm thấy tệp), nên chưa hoàn tất chơi tay một chiến dịch bằng chuột/phím Windows. Smoke đã chạy renderer DxLib, handler UI và audio API thật; đó là bằng chứng khác với nhập liệu hệ điều hành.

Thời lượng 25–35 phút và tỉ lệ thắng 40–50% của người mới cần mẫu chơi thực. Bot không chứng minh hai con số đó. Đọc VALIDATION.md để xem số liệu theo lớp và điều kiện đo.

Gói phát hành có nhãn preview vì hai tiêu chí này còn mở. Nguồn/EXE/dữ liệu vẫn là C++/DxLib trên cùng nhánh.
