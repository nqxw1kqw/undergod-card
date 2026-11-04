# Nguồn tham chiếu và thay đổi runtime

Theo yêu cầu thay thế bằng Card Quest, runtime chính trong UndergodCard.sln hiện dùng core C++ mới ở quest_core.hpp và renderer DxLib. Nền hành vi được chuyển từ bản CardQuest TypeScript trong workspace: JSON, Mulberry32, luật giáp/HP/energy/Nộ, deck và intent. Native hiện có timeline phóng lao, điều khiển tốc độ và bộ pastel 50 PNG riêng.

Kho RE, bản web và ảnh nguồn được lưu riêng ngoài repo. Không có mã hoặc tài nguyên trích xuất từ Slay the Spire trong bộ file được phát hành. Source runtime cũ cùng pipeline pixel đã được gỡ khỏi project Undergod.

Tham chiếu hiện tại:

- Core TypeScript của bản web tham khảo: hành vi combat được chuyển sang C++; golden test giữ các giá trị RNG và tay bài mở đầu.
- JSON được sao chép độc lập vào `UndergodCard/data`, sau đó điều chỉnh cho native.
- Nền timeline chuyển thành `data/cinematic.json`; bản native hiện dùng quỹ đạo lao lên trời rồi rơi xuống boss.
- Nguồn bộ pastel giai đoạn đầu đã được thay đổi. Native hiện dùng 50 PNG ở `assets/pastel`, gồm Gà Chiến Binh và VFX lao mới; không đọc đường dẫn project web.

Build và chạy native không phụ thuộc CardQuest, Node, Electron hoặc thư mục RE. Đối chiếu RNG/tay mở đầu/intent và smoke native ghi trong VALIDATION.md.

Bản hiện tại dùng Gà Chiến Binh theo ảnh Shin gửi. AI boss có openingAttack và maxConsecutiveGuards trong enemies.json; intent mặc định đã đổi để phản công đều. Hash asset hiện tại nằm trong assets/manifest.json.
