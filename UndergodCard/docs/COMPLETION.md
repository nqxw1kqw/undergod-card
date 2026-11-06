# Hoàn thiện kế hoạch — v1.3.1-preview

Cập nhật theo yêu cầu mới: avatar đi tới điểm trên map, giữ màn đen khi đổi khu vực, ảnh FIGHT và chia bài; mỗi phe ba Turn Star, xếp ba lá rồi Attack luân phiên, xem trước bài địch bằng tooltip. Luật, chuyển đổi năng lượng/nâng cấp và tương thích bản lưu ở [TURN_STARS.md](TURN_STARS.md). Bản HTML/website giữ độc lập.

v1.2.0 sửa vị trí avatar xuyên FadeOut, trở lại map và lưu/tải; thêm sáu bộ riêng, 36 lá boss với chiêu và ảnh thực, giới hạn theo pha. Chi tiết tại [BOSS_DECKS.md](BOSS_DECKS.md).

v1.3.0 thêm 98 cấu hình hoạt ảnh cho player/boss/địch, 72 frame cơ thể và 68 frame VFX mới. Đâm, niệm, phóng, hồi/buff có chuyển động phù hợp từng lá; khiên giữ theo giáp thật của mỗi đơn vị. Đòn cuối chạy hết phần chạm rồi mới đổi màn. Xem [SKILL_ANIMATIONS.md](SKILL_ANIMATIONS.md).

v1.3.1 chuyển backbuffer và cửa sổ sang 1600×900. Giữ tỉ lệ sprite, căn giữa nội dung và đồng bộ vùng bấm mục tiêu; chỉnh chiều cao hàng chờ/bài tay, nút dưới map/shop và thông báo menu cho màn 16:9.

2026-11-06. Giữ nhánh draft/v0.3-wip và PR #2; chưa merge main/PR #1. Các hệ thống trong mốc v0.4/v0.5/v1.0 và yêu cầu Turn Star đã có triển khai. Hai tiêu chí cần chơi thực để mở trong IMPLEMENTATION_CHECKLIST.

- Ba hồi phân nhánh 13/13/15 tầng, phần thưởng/hồi HP boss, sáu boss với ngưỡng HP và cơ chế riêng.
- Nhiều mục tiêu, chọn mục tiêu, AOE/ngẫu nhiên, summon, multi-hit và charge.
- 50 lá gốc +48 nâng cấp có thay đổi thực/mô tả effect; đủ bảng bài thiết kế và các từ khóa.
- Đủ 12 sự kiện có cơ chế: chọn lá, đổi di vật, xiếc ba lượt, hai trận phục kích nối tiếp, cầu tinh anh thưởng đôi, che map, khế ước và các đánh đổi.
- 17 di vật gồm đủ mười cơ chế thiết kế; sáu thuốc; thưởng 60/33/7 theo tầng hiếm, tinh anh Hiếm+, di vật không trùng.
- Ba lớp/bộ khởi đầu/tuyệt kỹ; 45 kỹ năng; tiền meta; Thử Thách cộng dồn 1–10; 15 thành tựu.
- Session v2/profile v1, ba RNG, sổ nhận thưởng chống trùng, ghi nguyên tử, kiểm tra đường map/pile/tham chiếu.
- 379 PNG: ảnh riêng cho 50 bài người chơi, 36 lá boss, ba lớp, sáu boss, địch thường/tinh anh, đồ và banner FIGHT. Giữ gà Chiến Binh đã được Shin duyệt làm chuẩn; bộ ếch dùng chung cũ đã ra khỏi runtime.
- Bão Lông/Ảnh Phân có cinematic riêng; Sao Rơi giữ phóng lao lên trời/rơi đúng contact. Nhạc ba hồi, chín cue PCM gốc và tắt/bật lưu trong profile.
- UI lớp/kỹ năng/thành tựu/Thử Thách/mục tiêu/khóa bài/dự báo/thưởng boss; hàng chờ Turn Star thay thanh năng lượng/Mana.

## Quyết định cụ thể

Boss chuyển pha theo HP hiện có, không nạp lại HP. Bóng Cáo một HP sẽ vỡ khi trúng, UI che HP/tên thật; hạ con thật làm tan bóng. Nòng Nọc còn sống tiếp tục trận sau khi Ếch Vương chết.

Mắt Diều Hâu mô phỏng trên bản sao hai lượt nếu không chơi thêm bài, không tiêu RNG thật; hành động có thể đổi dự báo. Undergod chuyển tài nguyên riêng của player thành Sức Mạnh/hút Nộ khi sao chép.

Địch thường có ảnh riêng và chuyển động code; không nhân bản ảnh thành animation giả. Boss có idle/attack cùng động tác niệm hoặc đỡ/vồ mới. Player có động tác riêng cho đâm, mổ, giậm, chém, ném, niệm và đỡ. Phản ứng nhận đòn/khiên theo từng đơn vị; không dùng chung một phản ứng cho cả nhóm boss và lính.

Raw/QC ở artifacts/art-v1 ngoài Git. Công cụ import chỉ tách grid/chroma key/alpha/căn frame; bitmap do ImageGen tạo. Audio từ tools/synth_audio.py, không dùng bản thu bên ngoài.

## Cần chơi thực để xác nhận

Chưa hoàn tất chơi tay một chiến dịch bằng chuột/phím Windows. Lần kiểm tra v1.0.1 ghi nhận helper Computer Use lỗi native pipe, os error 2 (không tìm thấy tệp); lần này không kiểm tra lại helper. Smoke đã chạy renderer DxLib, handler UI và audio API thật; đó là bằng chứng khác với nhập liệu hệ điều hành.

Thời lượng 25–35 phút và tỉ lệ thắng 40–50% của người mới cần mẫu chơi thực. Bot không chứng minh hai con số đó. Đọc VALIDATION.md để xem số liệu theo lớp và điều kiện đo.

Gói phát hành có nhãn preview vì hai tiêu chí này còn mở. Nguồn/EXE/dữ liệu vẫn là C++/DxLib trên cùng nhánh.
