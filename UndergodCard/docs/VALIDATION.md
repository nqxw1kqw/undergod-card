# Kiểm chứng

## Hành trình v0.2

- Logic combat nằm ngoài DxLib. `make -C UndergodCard core` chạy 134 kiểm tra trên máy không có Visual Studio. Seed 1337 thắng bốn trận, HP mang sang trận sau là 47, 40 và 40, kết thúc 21/50 ở giai đoạn 2 của Ếch Vương.
- `verify.ps1` trên Windows build solution, chạy cùng `CoreChecks.exe`, rồi smoke DxLib. Smoke tự chơi hành trình và dừng lỗi nếu thiếu `playthrough_victory=1` hoặc `boss_phase=2`.
- Bộ kiểm tra cũ của giáp, Nộ, timeline, tốc độ và RNG Mulberry32 vẫn còn. Công thức legacy của ý định được giữ khi địch không có `patterns`, để đối chiếu chuỗi TypeScript.

## Nhịp chậm, tốc độ và lao rơi — 2026-11-04

- C++17 / MSVC v143 / DxLib / Windows x64, project UndergodCard.sln. Ghi chép dưới đây là lần kiểm cinematic trước v0.2. Số kiểm tra và kết quả một trận 27/50 HP không còn là kết quả hiện tại; xem mục v0.2 ở trên. Renderer smoke vẫn yêu cầu 50/50 PNG. Header DxLib ngoài project còn cảnh báo C4010.
- x1 mặc định mỗi run dùng playback rate 0,65: thẻ khoảng 1 giây, lượt địch khoảng 1,77 giây; x2/x3 nhanh tương ứng. Nút và F1/F2/F3 hoạt động cả khi input gameplay bị khóa/cinematic đang chạy.
- Unit test đo thời gian thực cho cả ba tốc độ, kiểm tra HP, damage, năng lượng, tay rút tiếp và RNG intent giống nhau sau một lượt đầy đủ. Thay speed giữa flight không làm nhảy vị trí hoặc gây damage ngay. Toast dùng thời gian thực riêng.
- Timeline khai báo: pose ném tại 250ms, thả 750ms, bay lên tới 1450ms, rơi vào boss và áp damage tại 2600ms, kết thúc 3600ms của đồng hồ gameplay. x1 cinematic khoảng 5,54 giây. Loader từ chối quỹ đạo rơi không khớp mốc impact.
- Ba ảnh gốc mới do công cụ sinh ảnh tích hợp tạo: sheet ult_throw, lao riêng, impact. Hậu xử lý xuất 6 PNG mới, tách alpha/căn chân/làm sạch viền; thay sheet ngôi sao cũ nên runtime có 50 PNG. Pose throw giữ body scale theo vùng lông/chân (383–384px so với idle 376px), không thu nhỏ thân theo spear giơ cao. Ảnh gốc và công cụ hậu xử lý được lưu riêng ngoài repo.
- Cả ba speed: trước contact boss vẫn 120 HP, sau contact với 6 giáp còn 86 HP, sát thương đúng 40 qua giáp. Skip trước phóng, đang bay, trước contact và sau contact đều resolve một lần; hết/skip/reset dọn projectile/camera và trả lại input.
- Smoke DxLib kiểm tra click ba nút tốc độ, đổi speed trong cinematic, windup/rise/fall/contact, boss phản công, thẻ, kết quả trận, restart. Ảnh: 05-spear-windup.png, 05-spear-rise.png, 05-spear-fall.png, 06-impact.png. Không còn ngôi sao xuất hiện trong cinematic.
- Trước v0.2, trận đơn seed 1337 thắng ở lượt 7 với 27/50 HP. Hành trình hiện tại dùng cùng API và đồng hồ Update; kết quả mới nằm ở mục trên.
- Chạy verify.ps1 -Configuration Release hoặc Debug để sinh báo cáo/ảnh ở bin/<cấu hình>/artifacts. Hai ảnh minh họa giữ tại docs/images; assets/manifest.json ghi hash của toàn bộ PNG hiện dùng. Các đầu ra build/smoke được sinh lại và không commit lên Git.
- Gói release chạy --smoke PASS; 50 PNG ở source, Debug, Release và gói đều khớp SHA-256 trong manifest. Đã xem ảnh render giai đoạn lao rơi trên boss; không còn starfall.png trong các bộ runtime.

Giới hạn: smoke gọi handler UI theo tọa độ, chưa thay thế một lượt chơi tay dài hạn trên thiết bị nhập Windows. Chưa có âm thanh, lưu run hoặc phát MP4. Gói native cần giữ EXE, assets và data cạnh nhau.
