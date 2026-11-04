# Cấu trúc lõi Card Quest native

Runtime hiện tại đã chuyển sang Card Quest C++/DxLib. Những tối ưu Doom/Nyx của prototype RE trước đây thuộc lịch sử project; không phải luật đang chạy của bản này.

Core mới ở quest_core.hpp chỉ dùng thư viện chuẩn C++: CardDef/Config tách khỏi combat state, không gọi renderer/đồng hồ. Hai luồng Mulberry32 giữ kết quả với TypeScript, tránh việc reshuffle đổi intent. Events chỉ mang giá trị, không giữ con trỏ tới actor/thẻ đã bị gỡ.

Controller Game quản lý một action đang chạy, khóa input cho đến hết recovery và gọi core tại contact. Timeline giữ cursor/cờ impact/end riêng nên skip nhiều lần hoặc frame dài không áp damage lặp. Renderer lấy frame từ tuổi pose, giữ đường chân cố định, UI hit-test dùng cùng Rect với draw.

JSON được tải và kiểm tra một lần lúc khởi động. PNG nạp một lần khi Ui.Init; Shutdown giải phóng graph/font/render target. Sân được vẽ vào offscreen target để zoom/shake chỉ ảnh hưởng thế giới, không lệch HUD và vùng click.

Chưa đo CPU/GPU hoặc mức tăng FPS. Không chuyển các báo cáo cache/103 test của prototype RE cũ sang tuyên bố kiểm chứng cho bản mới; số kiểm tra hiện tại là 93, xem VALIDATION.md.

Game.Update nhận thời gian thực, rồi nhân playback rate 0,65 × speed cho action, pose, projectile và timeline. Core không dùng clock nên đổi x1/x2/x3 không đổi kết quả combat; toast dùng thời gian thực riêng.

Bản hiện tại dùng Gà Chiến Binh theo ảnh Shin gửi. AI boss có openingAttack và maxConsecutiveGuards trong enemies.json; intent mặc định đã đổi để phản công đều. Hash asset hiện tại nằm trong assets/manifest.json.
