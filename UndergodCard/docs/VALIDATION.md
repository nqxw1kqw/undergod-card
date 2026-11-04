# Kiểm chứng v0.3 — 2026-11-05

## Môi trường và cách chạy lại

Windows x64, Visual Studio 2022 / MSVC v143 / C++17 / DxLib ở C:\DxLib.

    .\UndergodCard\verify.ps1 -Configuration Release
    .\UndergodCard\verify.ps1 -Configuration Debug
    .\UndergodCard\bin\Release\CoreChecks.exe --simulate 1000
    .\UndergodCard\package.ps1 -SkipBuild

Bộ core không gọi DxLib. Lần này kiểm tra bằng MSVC; chưa chạy lại Makefile/g++ sau thay đổi persistence.

## Kết quả đã xác nhận

- Debug và Release: **191 kiểm tra, 0 lỗi**; renderer smoke **PASS**, nạp đúng **50/50 PNG**. C4010 còn đến từ header SDK ngoài project.
- Bản ZIP đã giải nén sang một thư mục mới và chạy --smoke PASS. 50 PNG khớp SHA-256 manifest, 10 JSON khớp source; có guide UTF-8 và giấy phép, không kèm save của người chơi.
- Seed 1337 đi hết 13 tầng, thắng boss phase 2, còn **22/50 HP**, **8 lần chọn thưởng**, bộ **18 lá**, **85 vàng**, **1 di vật**. Driver hành trình không ép HP/hand/rage.
- Map 300 seed: lặp đúng cùng seed, đổi nội dung giữa seed, không nút cụt/không tới được, có boss cuối, nghỉ trước boss và chặn tinh anh quá sớm. Trọng số đo ở các nút ngẫu nhiên đủ điều kiện nằm trong 3,5 điểm phần trăm so với JSON; không tính hàng mở đầu/nghỉ bắt buộc.
- Pool sinh đủ 6 địch thường và 2 tinh anh. Vàng trong khoảng, di vật tinh anh chỉ thưởng một lần. Bộ di vật hiện tại có 12 loại, thuốc có 6.
- Smoke click shop mua/gỡ lá ở trang 3; nghỉ nâng cấp lá ở trang 3; hồi HP khớp nhãn có Bình Nước; sử dụng ô thuốc thứ tư; sự kiện, cuộn map tới boss, thưởng, restart và hai nút lưu/tải.
- Lưu/tải round trip các màn Map/Event/Shop/Rest/Reward và kết quả; phục hồi giữa lượt và boss phase 2, giữ RNG/deck/intent/potions. Tự chơi tiếp sau load cho ra cùng JSON session.
- Lưu/tải trong motion/cinematic bị khóa; autosave thực hiện sau khi ổn định. Ghi temp thất bại giữ file trước; version sai, JSON cụt, RNG/HP/gold/node sai, map khác, enemy/card sai hoặc deck không bảo toàn đều không thay run hiện tại.
- Sao Rơi vẫn resolve đúng một lần khi skip nhiều thời điểm. Mốc impact 2600ms gameplay; x1 khoảng 5,54 giây, speed không thay kết quả combat.

## Bot 1.000 seed

Chạy seed 0–999 qua public API, tối đa 4.000 quyết định mỗi run:

- Thắng 983, thua 17; **không run nào kẹt**.
- Tỉ lệ thắng bot **98,3%**, HP trung bình khi thắng khoảng **24,95**.
- Tổng 105.687 quyết định; có sử dụng thuốc, shop, nghỉ, sự kiện và phần thưởng.
- Chế độ simulation thêm một kiểm tra không bị kẹt: tổng **192 kiểm tra, 0 lỗi**.

Báo cáo máy sinh ở bin/Release/artifacts/simulation.json. Đây là bot có chính sách chọn lá/lối đi, không phải số liệu người chơi mới. Chưa đạt mục tiêu đánh giá cân bằng bằng người chơi thật; tỉ lệ thắng bot hiện cao, cần chỉnh độ khó ở đợt cân bằng sau.

## Bằng chứng và giới hạn

Báo cáo/ảnh smoke sinh trong bin/<cấu hình>/artifacts. Ảnh hiện tại được giữ ở docs/images: map, trận đầu, shop và lao rơi. Artifact đầy đủ có ảnh các trang cuối, thuốc, sự kiện, phần thưởng và boss phase 2.

Đã xem ảnh render thật từ DxLib. Thử điều khiển cửa sổ qua computer-use hai lần đều lỗi kết nối native pipe (os error 2); chưa hoàn thành chơi tay bằng chuột/phím Windows hoặc phiên mở/đóng GUI tiếp tục thật. Smoke gọi handler theo tọa độ và core test kiểm tra persistence; không coi hai loại kiểm tra này là chứng minh hoàn tất chơi tay.

Chưa có âm thanh, ba hồi, lớp nhân vật/meta, nhiều địch một trận, art riêng cho các địch mới và 15 lá thưởng. Chưa đo CPU/GPU hoặc FPS.
