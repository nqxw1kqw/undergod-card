# Undergod Card Quest — C++17 / DxLib

Bản v1.3.0-preview sửa icon nhảy về điểm cũ sau khi đến stage và thêm 36 lá riêng cho sáu boss: [docs/BOSS_DECKS.md](docs/BOSS_DECKS.md). Giữ ba Turn Star, hàng bài hai phe luân phiên, chuyển cảnh đen, FIGHT và chia bài. Chi tiết luật/tương thích tại [docs/TURN_STARS.md](docs/TURN_STARS.md); bằng chứng tại docs/VALIDATION.md. Demo HTML giữ độc lập.

Mốc này thêm [hoạt ảnh cho từng kỹ năng](docs/SKILL_ANIMATIONS.md), dùng 18 bộ động tác và 17 bộ VFX mới: lao đâm/ném, phép bay, mổ/giậm/chém/vồ, buff và khiên tồn tại theo giáp thật cho cả hai phe. Lá đang dùng nằm dưới góc phải để không che sân. Game đợi hiệu ứng đòn cuối kết thúc trước khi mở thưởng hoặc trận kế tiếp.

## Build, chạy và đóng gói

Mở UndergodCard.sln trong Visual Studio 2022, chọn UndergodCard, Release | x64. Cần MSVC v143, Windows SDK và DxLib Visual C++ x64; mặc định C:\DxLib.

    .\build.ps1 -Configuration Release
    .\verify.ps1 -Configuration Release
    .\verify.ps1 -Configuration Debug
    .\bin\Release\CoreChecks.exe --simulate-stars 300
    .\package.ps1 -SkipBuild

Các script nhận -DxLibDir. EXE ở bin/Release/UndergodCard.exe; ZIP ở release/UndergodCard-DxLib-Windows-x64.zip. Giữ assets/data/licenses cạnh EXE. Bản đóng gói không cần SDK, Python hay Node.js.

Core không dùng DxLib. Makefile hỗ trợ g++ C++17 cho CoreChecks; kiểm chứng lần này dùng MSVC trên Windows.

## Hành trình và nhân vật

Ba hồi 13 / 13 / 15 tầng, tính cả trùm/nghỉ cuối. Hồi đầu đọc tầng/cột từ map.json, hai hồi sau từ campaign.json. Hồi hai chọn Rắn, Diều Hâu hoặc Hổ Vương theo seed. Hồi ba có Cáo, nghỉ rồi Undergod. Boss hồi hồi 50% HP tối đa và cho chọn di vật boss; hết pool sẽ đổi thành vàng. Tinh anh thường cho một di vật, tinh anh Cầu Gãy thưởng hai di vật và vàng gấp đôi.

| Boss | HP | Cơ chế |
| --- | --- | --- |
| Ếch Vương | 150 | Dưới 50% gọi hai Nòng Nọc, khóa một lá mỗi lượt |
| Rắn Bùn Mẹ | 180 | Lột da ở 65%, siết ba lượt dưới 25%, charge báo trước |
| Diều Hâu Bão | 160 | Bay giảm nửa sát thương cận chiến; nhận 20 sát thương HP sẽ đáp hai lượt |
| Hổ Vương Xích Diệm | 200 | Song trảo/gầm; dưới 50% HP tăng Sức Mạnh, có vòng tụ lực báo trước rồi mới vồ |
| Cáo Thầy Phù | 220 | Biến bài thành Bùn Lầy, dưới 50% tạo hai ảo ảnh/xáo vị trí |
| Undergod | 300 | Sao chép lá cuối lượt trước; hút Nộ ở 66%; Tận Thế năm lượt ở 33% |

Ảo ảnh che tên/HP thật; hạ Cáo làm tan bóng. Nòng Nọc còn sống tiếp tục trận sau khi Ếch Vương chết. HP/Nộ và đồ giữ giữa trận; giáp/trạng thái/năng lực trận reset, cờ hồi sinh đã dùng giữ cả run.

Chiến Binh 50 HP/rút 5, Sao Rơi 40 sát thương. Pháp Sư 40 HP/rút 6, Bão Lông 12 ×4 ngẫu nhiên. Ninja 45 HP/rút 5, Ảnh Phân lặp tối đa ba lá gần nhất. Mỗi lá nay dùng một sao, không giữ giá năng lượng hoặc lá miễn phí cũ; các hiệu ứng năng lượng được đổi sang rút bài. Xem luật chuyển đổi và nâng cấp tại docs/TURN_STARS.md.

Mỗi lớp có ba nhánh × năm kỹ năng; mua tuần tự bằng Lông Vũ và áp dụng từ run mới. Thắng độ khó hiện tại mở Thử Thách kế tiếp tới 10. Sổ run đã nhận thưởng ngăn nhận Lông Vũ/thành tựu trùng khi tải lại.

## Điều khiển

- Nút Đấu thử Hổ Vương ở menu lớp mở ngay boss với bài nâng cấp/hai thuốc; không ghi đè bản lưu hoặc thưởng meta. Cũng có thể chạy --boss tiger --seed 1337; xem docs/TIGER_BOSS.md.
- Chọn lớp: 1–3 hoặc chuột; Enter bắt đầu. Chuột mua kỹ năng/chọn Thử Thách.
- Map: chuột/1–9 chọn lối; bánh xe, Page Up/Page Down hoặc nút cuộn.
- Combat: chuột/1–9 xếp bài; bấm lá trong hàng chờ để gỡ; đủ ba vị trí thì E/Attack; Tab/bấm địch chọn mục tiêu; U tuyệt kỹ.
- Z/X/C dùng ba ô thuốc, V dùng ô thứ tư nếu di vật mở thêm.
- F1/F2/F3 hoặc x1/x2/x3 đổi tốc độ; run mới bắt đầu chậm ở x1.
- Space/Esc bỏ cinematic, damage chỉ áp một lần. F10 bật/tắt âm thanh.
- Shop: 1–7 mua; bấm lá trong bộ để gỡ (75 vàng); E/Esc rời.
- Nghỉ: H hồi HP hoặc 1–8 nâng cấp trên trang hiện tại. Chỉ chọn một việc.
- Bộ bài/màn chọn bài/đổi đồ có nút trang, bánh xe, Page Up/Page Down.
- F6 lưu, F9 tiếp tục. R/Enter/Chơi lại mở menu lớp; Esc từ menu về run đang có.
- Tham số --seed 1337 khởi tạo run có seed cố định.

## Dữ liệu và thêm nội dung

| Tệp trong data | Nội dung |
| --- | --- |
| cards | 50 lá gốc, type/rarity/class/effects/keywords, exhaust/power/ranged |
| enemies | Sáu địch thường, hai tinh anh, sáu boss, triệu hồi và đối thủ sự kiện |
| boss_cards | Sáu lá riêng/boss, ID/tên/chiêu/trọng số và pha được phép; ảnh riêng ở assets/pastel/boss_cards |
| campaign / map / run | Hồi, pool boss/địch, tầng/cột, trọng số, kinh tế |
| events | 12 sự kiện với đánh đổi, chọn bài/đổi đồ hoặc mở trận thực |
| relics / potions | 17 di vật, sáu thuốc dùng chung resolver |
| classes / skills | Bộ khởi đầu, tuyệt kỹ, 45 kỹ năng và điều kiện mở bài |
| challenges / achievements | Bất lợi cộng dồn và mốc tiến trình |
| cinematic / audio | Timeline và danh sách âm thanh/nhạc |

Thêm một id duy nhất trong cards.json, effects và icon assets/pastel/cards/id.png (RGBA 256×256). Loader sinh bản nâng cấp; lời nguyền không có nâng cấp. Effect mới cần thêm parser/resolver/kiểm thử. Loader kiểm tra tham chiếu bài/lớp/địch/kỹ năng.

Nhân vật: char_<sprite>_<pose>_<frame>.png, RGBA 512×512. Đổi ảnh cần cập nhật assets/manifest.json. tools/validate_assets.py kiểm tra hash, kích thước, tham chiếu và WAV. tools/expand_content.py biên tập JSON, không cần chạy khi build.

## Lưu game

save.json schema v2 giữ đường map, hồi/lớp/độ khó, tài nguyên, deck, các pile gồm Tiêu hao, lời nguyền sinh ra, triệu hồi, mục tiêu, năng lực, lịch sử bài, hệ số trận và ba RNG. profile.json schema v1 giữ Lông Vũ, kỹ năng, thành tựu, Thử Thách và sổ nhận thưởng.

Ghi .tmp rồi thay nguyên tử; lỗi giữ checkpoint cũ. Tải qua đối tượng tạm và kiểm tra đường map, tham chiếu, pile bài bảo toàn và trạng thái trận. Khóa lưu/tải trong animation/cinematic. Bản v1/không hợp lệ giữ thành .invalid.* khi tự mở.

Chép cả save.json và profile.json cùng phiên bản data để chuyển máy; không đưa chúng vào Git hoặc ZIP.

## Cấu trúc và giới hạn

quest_core/effects/status/battle/turn_stars: combat và hàng bài. quest_run: chiến dịch, sự kiện, kinh tế, bot. quest_world: dữ liệu. quest_meta: tiến trình. quest_save/storage/json: lưu/tải. game/game_order: contact, chuyển cảnh, animation và khóa input. ui/ui_world/ui_order/audio: DxLib.

Undergod sao chép sát thương/giáp/hồi máu/trạng thái/năng lực về phía địch. Tài nguyên riêng của người chơi (rút bài/năng lượng/giảm giá tuyệt kỹ) đổi thành Sức Mạnh; Nộ đổi thành hút Nộ. Mắt Diều Hâu dự báo trên bản sao hai lượt nếu không chơi thêm bài; hành động có thể đổi dự báo.

CoreChecks kiểm tra boss, effect, map, checkpoint mỗi quyết định, meta, Thử Thách và luật ba sao. --star-smoke chạy renderer thật với map/FIGHT/chia bài/Attack/tooltip/hổ tụ lực; --smoke giữ đối chiếu chiến dịch theo nền cũ. SDK ngoài repo có cảnh báo C4010 trong DxFunctionWin.h.

Mục tiêu thời lượng/tỉ lệ thắng người chơi mới và chơi tay qua chuột/phím Windows còn cần xác nhận; đọc VALIDATION.md.
