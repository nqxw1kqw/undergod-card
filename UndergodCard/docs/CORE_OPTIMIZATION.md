# Cấu trúc lõi Card Quest native

Runtime là C++17/DxLib, build MSVC v143/VS2022. Các tối ưu Doom/Nyx trong prototype RE thuộc lịch sử; không phải luật hiện tại. Nhánh `integrate/draft-main-optimizations` giữ chiến dịch draft và chuyển các cải tiến controller/UI từ main; [bảng đối chiếu](DRAFT_MAIN_INTEGRATION.md) ghi rõ quyết định và kiểm tra từng phần.

Combat chỉ dùng thư viện chuẩn C++; effect/status/Turn Star tách khỏi renderer và đồng hồ. RunState quản lý map ba hồi, sự kiện, shop, thưởng và di vật; profile quản lý meta. Ba luồng RNG xáo bài/ý định/mục tiêu được lưu riêng; FX dùng RNG thứ tư ngoài combat. Preview chạy resolver trên bản sao và không đổi ba RNG thật. Event chỉ mang giá trị, không giữ con trỏ tới actor/thẻ đã bị gỡ.

Controller giữ một hành động, gọi core tại contact, khóa input đến hết hồi động tác. Ba lá mỗi phe chạy luân phiên. Timing lấy từ cấu hình kỹ năng; pose/projectile/khiên không phụ thuộc số frame. Impact/end có cờ riêng nên skip lặp không áp hai lần. Core xác định kết liễu ngay, hình ảnh hoàn tất rồi mới mở thưởng.

Bài hiển thị sở hữu snapshot và ID riêng, không giữ tham chiếu vào vector tay đã bị xóa. Chọn/gỡ/dùng/bỏ/tiêu hao giữ đúng bản bài; vùng bấm theo hình ảnh di chuyển. Chồng bài chỉ hiện đa tập, không lộ thứ tự rút. Particle/popup có giới hạn và tuổi thọ; cleanup cả ngoài cinematic, reset xóa trạng thái cũ.

JSON được tải/kiểm tra một lần khi khởi động; PNG/WAV nạp một lần, Shutdown giải phóng handle. Ảnh gục là bản sao GPU dùng bộ lọc đơn sắc DxLib, không thêm asset runtime. Sân dùng render target riêng để zoom/shake không làm lệch HUD/hit-test. Các đoạn zoom lấy mẫu tại timestamp cue nên không nhảy về mặc định khi ngắt đoạn.

Các cải tiến được xác nhận về hành vi, tính tái lập và renderer. Số liệu Debug/Release, mô phỏng, asset và gói ở [VALIDATION.md](VALIDATION.md); các phép đo CPU được tách khỏi kết luận về FPS/GPU toàn game.

v1.4.1 đã đo riêng truy vấn CPU dự báo trước/sau cache và CPU Update/Draw hiện tại. Cache dùng khóa trạng thái byte chính xác, tái sử dụng bộ đệm, chỉ mô phỏng lại khi trạng thái gameplay thay đổi. Chi tiết điều kiện mất hiệu lực, benchmark và giới hạn đo ở [SIX_ACTION_POLISH.md](SIX_ACTION_POLISH.md); chưa có phép so FPS/GPU toàn game trước/sau.

Game.Update nhận thời gian thực, rồi nhân playback rate 0,65 × speed cho action, pose, projectile và timeline. Core không dùng clock nên đổi x1/x2/x3 không đổi kết quả combat; toast dùng thời gian thực riêng.

Giữ Gà Chiến Binh theo ảnh Shin gửi và sáu boss/bộ bài riêng của draft. Mức khó đấu thử đổi sát thương công bố, giữ HP/deck/seed; Thử Thách chiến dịch riêng. Hash asset ở assets/manifest.json.

Session v2/profile v1 đọc vào đối tượng tạm, kiểm tra map/pile/tham chiếu rồi mới áp; ghi nguyên tử. Autosave chờ hết chuyển cảnh/hành động. reducedFeedback là tùy chọn profile tương thích file cũ; các smoke không ghi save/profile người chơi.
