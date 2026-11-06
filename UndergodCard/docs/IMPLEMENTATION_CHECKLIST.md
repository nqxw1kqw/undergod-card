# Theo dõi hoàn thiện toàn bộ GAMEPLAY_PLAN

Phạm vi được Shin xác nhận: toàn bộ lộ trình còn lại, cùng nhánh draft/v0.3-wip. C++17 / DxLib / Visual Studio 2022 x64.

## Chiến dịch và combat

- [x] Ba hồi, hồi phục/phần thưởng boss và trận cuối Undergod.
- [x] Đủ năm boss: Frog summon/khóa bài, Snake lột da/siết, Hawk bay/đáp, Fox nguyền/ảo ảnh, Undergod sao chép/hút Nộ/Tận Thế.
- [x] Boss bổ sung Hổ Vương Xích Diệm từ ảnh Shin gửi: 12 frame, hai pha, pool Hồi 2 và đấu thử riêng.
- [x] Nhiều mục tiêu, chọn mục tiêu, AOE/ngẫu nhiên; ý định nhiều hit, triệu hồi và charge báo trước.
- [x] Tiêu hao, giữ giáp, xuyên giáp, năng lực, hồi sinh, lặp bài, lời nguyền.
- [x] Khoảng 50 lá gốc (không tính bản nâng cấp), đủ các lá trong bảng thiết kế.
- [x] Đủ 12 sự kiện và các đánh đổi có cơ chế thực (không chỉ đổi văn bản).
- [x] Di vật/thuốc đúng cơ chế, phần thưởng theo độ hiếm và di vật boss.

## Nhân vật và tiến trình

- [x] Ba lớp, bộ khởi đầu/thuộc tính/tuyệt kỹ khác nhau.
- [x] Cây kỹ năng ba nhánh, năm nút; tiền Lông Vũ lưu giữa các run.
- [x] Thử Thách 1–10 có tác động và tiến trình mở khóa.
- [x] Thành tựu và lưu profile; tránh thưởng lặp khi tải kết quả.
- [x] Lưu/tải đầy đủ chiến dịch, combat nhiều địch, năng lực và meta.

## Trình bày và phát hành

- [x] Art runtime cho bài, lớp và địch/boss mới; alpha/căn frame và manifest.
- [x] Cinematic riêng cho tuyệt kỹ ba lớp; giữ chiêu lao rơi của Chiến Binh.
- [x] Âm thanh/nhạc và nút tắt/bật.
- [x] UI lớp/kỹ năng/Thử Thách, mục tiêu, các màn sự kiện và thưởng boss.
- [x] Kiểm tra có seed, Debug/Release, smoke và ZIP. Mô phỏng 1.000 run ở v1.0.1 là đối chiếu luật năng lượng cũ; luật ba sao có 300 run riêng.
- [x] Tài liệu và gói Windows được cập nhật; nguồn phát triển giữ trên draft/v0.3-wip.

## Turn Star và mở trận — v1.1.0-preview

- [x] Avatar đúng lớp hiện trên map và di chuyển tới điểm được chọn.
- [x] Mờ dần tới đen hoàn toàn, vào khu vực rồi sáng lại; khóa thao tác trong chuyển cảnh.
- [x] Tạo ảnh FIGHT alpha: từ phải vào giữa, dừng rồi ra trái; chia bài sau banner và mỗi vòng mới.
- [x] Mỗi phe ba sao, mỗi lá một sao; chọn/gỡ hàng chờ, nhớ mục tiêu và có Chờ 1 sao.
- [x] Công bố ba lá địch trước Attack; hover thông tin, giữ nguyên kế hoạch và RNG.
- [x] Attack luân phiên ta 1/địch 1/ta 2/địch 2/ta 3/địch 3, hiện lá đang dùng và áp hiệu ứng theo thứ tự.
- [x] Đồng bộ tooltip, nâng cấp, tài nguyên, khóa bài, tụ lực, chuyển pha và trận nối tiếp với luật mới.
- [x] Lưu/tải hàng chờ và đọc session v2 cũ; x1/x2/x3 cho cùng kết quả.
- [x] Debug 3.536 kiểm tra; Release kèm 300 chiến dịch: 3.537 kiểm tra, 0 lỗi. 90 trận boss và 300 chiến dịch đều không kẹt.
- [x] Native smoke cả hai cấu hình; ZIP 239 tệp chạy cả hai smoke sau giải nén. HTML giữ nguyên 220 tệp runtime.

## Map và bộ bài boss — v1.2.0-preview

- [x] Giữ icon ở đích xuyên FadeOut; trở lại map và lưu/tải không quay về lối vào.
- [x] Sáu lá riêng cho mỗi boss; tổng 36 ID/tên/ảnh/chiêu riêng, không vào shop/thưởng người chơi.
- [x] Chọn từ bộ của đúng boss theo pha, không trùng lá thường trong một kế hoạch; giữ lá đã công bố và RNG khi thao tác/lưu/tải.
- [x] Tụ lực dùng ảnh riêng, chỉ đếm một lần/vòng; Undergod sao chép hiệu ứng nhưng giữ thẻ riêng.
- [x] 36 ảnh ImageGen qua tách nền/căn/QC; runtime có 239 PNG, tooltip và tên dài hiển thị trong khung ba dòng.
- [x] Kiểm tra quyền sở hữu mọi pha, hiệu ứng từng lá, ID chéo boss và bản v1.1; số liệu hiện tại tại VALIDATION.md.

## Hoạt ảnh kỹ năng — v1.3.0-preview

- [x] 48 lá player chơi được và 36 lá boss có cấu hình chuyển động/đường bay/ảnh chạm riêng; bản nâng cấp kế thừa kỹ năng.
- [x] 18 bộ cơ thể + 17 bộ VFX ImageGen, 140 frame RGBA mới; 35 bộ đều bốn frame khác nhau và không cắt mép.
- [x] Lao đâm, phép bay trước khi sát thương, mưa vật thể đúng mục tiêu, lưỡi nối miệng/rút về, Hổ vồ không cắt đầu.
- [x] Khiên của player/boss/lính giữ khi còn giáp, phản ứng chặn và tan khi hết giáp.
- [x] Chờ hết hoạt ảnh/cinematic đòn kết liễu trước thưởng hoặc trận tiếp.
- [x] Kiểm tra thời điểm chạm, RNG, Sao Chép, đa mục tiêu và x1/x2/x3; 300 chiến dịch giữ nguyên kết quả.
- [x] Build Debug/Release và ba smoke DxLib; chi tiết và gói giải nén ở VALIDATION.md.

## Các mục cần bằng chứng chơi thực

- [ ] Chơi tay toàn chiến dịch trên Windows (helper trước đó lỗi native pipe).
- [ ] Xác nhận thời lượng 25–35 phút và tỉ lệ thắng người chơi mới 40–50%; không suy ra từ bot.

Chỉ đánh dấu sau khi có code/nội dung và bằng chứng kiểm tra tương ứng.
