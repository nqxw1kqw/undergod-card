# Undergod Card – Kế hoạch nâng cấp gameplay

Đưa game từ 1 trận Gà Chiến Binh đấu Ếch Vương thành chế độ roguelike nhiều tầng: bản đồ phân nhánh, 5 boss, khoảng 50 lá bài, sự kiện, lớp nhân vật và thánh vật, tất cả nạp từ JSON.

## Cập nhật thực hiện 2026-11-05

Nhánh draft/v0.3-wip đã tiếp tục mốc một hồi roguelike: map theo seed/JSON, 6 địch thường, 2 tinh anh, 12 di vật, 6 thuốc, shop/nghỉ phân trang và lưu/tải cả trạng thái combat. Chi tiết và giới hạn trong [V0.3_WIP_NOTES.md](V0.3_WIP_NOTES.md), số liệu kiểm chứng trong [VALIDATION.md](VALIDATION.md). Chưa merge vào main. Các bảng dưới đây là thiết kế đích dài hạn; không phải mọi cơ chế đã có trong v0.3.

## Hiện trạng lúc lập kế hoạch

Nền tảng tốt: logic tách khỏi giao diện và đã dùng JSON, nên mở rộng chủ yếu là thêm dữ liệu và một lớp Run bao quanh Combat.

| Hạng mục | Giá trị |
| --- | --- |
| Engine | C++17, DxLib, VS2022 x64, nlohmann/json |
| Code | `quest_core` (Combat, Rng), `game` (Game, Motion), `quest_timeline` (cinematic), `ui` |
| Dữ liệu | `balance.json`, `cards.json` (4 loại, 10 lá), `enemies.json` (1 boss), `characters.json`, `cinematic.json` |
| Chiến đấu | 50 HP, 3 năng lượng, rút 5; Nộ 100 (+8 khi đánh, +5 khi phòng) → Sao Rơi 40 dmg |
| Địch | Ếch Vương 120 HP, 70% tấn công 8–13, phòng 6, tối đa 1 lần phòng liên tiếp |
| Kiểm thử | 93 kiểm tra trong `CoreChecks`, combat tách khỏi renderer, RNG có seed |

## Trụ cột thiết kế

- Lựa chọn có ý nghĩa: mỗi nút bản đồ, phần thưởng và sự kiện là đánh đổi rủi ro với lợi ích.
- Ý định rõ ràng: boss luôn báo trước, người chơi thua vì quyết định chứ không vì may rủi.
- Nộ và Sao Rơi là bản sắc: nhiều lá, thánh vật xoay quanh tích và tiêu Nộ.
- Dữ liệu là trên hết: thêm lá, địch, sự kiện chỉ bằng JSON, không sửa C++.
- Lượt chơi ngắn: 25–35 phút cho một lần chạy 3 hồi.

## Cấu trúc một lượt chơi

Một hồi đi từ điểm bắt đầu, qua các nhánh trận thường / sự kiện / cửa hàng / elite, rồi chỗ nghỉ, và kết bằng boss hồi.

| Nút | Tỉ lệ / hồi | Thưởng |
| --- | --- | --- |
| Trận thường | 45% | 15–25 vàng, chọn 1 trong 3 lá |
| Elite | 12% | 30–40 vàng, thánh vật, lá Hiếm+ |
| Sự kiện | 22% | Theo lựa chọn |
| Cửa hàng | 8% | Mua lá, thánh vật, thuốc, xoá lá (75 vàng) |
| Nghỉ ngơi | 13% | Hồi 30% HP hoặc nâng cấp 1 lá |
| Boss | 1 / hồi | Thánh vật boss, hồi 50% HP |

Mỗi hồi 12–15 tầng, 3–4 cột song song; nút sinh từ seed để chạy lại được và kiểm thử được.

## Boss

Ý định mở rộng từ `Intent` (attack, value) thành nhiều loại: Attack, MultiAttack, Block, Buff, Debuff, Summon, Charge (báo đòn lớn 2 lượt).

| Boss | Vị trí | HP | Pha & cơ chế | Loại ý định |
| --- | --- | --- | --- | --- |
| Ếch Vương | Hồi 1 | 120 → 150 | P1: đánh 8–13 / phòng 6. P2 (<50%): Lưỡi Quấn khóa 1 lá trên tay, gọi 2 Nòng Nọc | Tấn công, Phòng thủ, Triệu hồi |
| Rắn Bùn Mẹ | Hồi 2 | 180 | P1: rải Độc. P2: lột da xoá debuff, +Sức mạnh. P3 (<25%): siết 3 lượt | Debuff, Tích tụ, Đòn lớn báo trước |
| Diều Hâu Bão | Hồi 2 (thay) | 160 | Bay: miễn 50% cận chiến, phải dùng Lao/Gió. Đáp đất khi nhận 20 dmg | Né tránh, Nhiều đòn |
| Cáo Thầy Phù | Hồi 3 | 220 | Xào lá: biến 1 lá thành Lời nguyền. P2: tạo 2 phân thân, chỉ 1 thật | Phép, Ảo ảnh |
| Undergod | Trùm cuối | 300 | P1 bắt chước lá bạn chơi lượt trước. P2 hút Nộ. P3: đếm ngược 5 lượt Tận Thế | Sao chép, Hút, Đếm ngược |

## Lá bài

Từ khoá mới: Tiêu hao, Giữ giáp, Xuyên giáp, Độc, Yếu, Dễ Vỡ, Sức mạnh, Nâng cấp (+). Tỉ lệ thưởng: Thường 60%, Hiếm 33%, Huyền thoại 7%.

| Tên | Loại | Độ hiếm | Năng lượng | Hiệu ứng |
| --- | --- | --- | --- | --- |
| Chọc Giáo | Tấn công | Thường | 1 | 6 sát thương |
| Khiên Gỗ | Phòng thủ | Thường | 1 | 5 giáp |
| Đâm Mạnh | Tấn công | Thường | 2 | 12 sát thương |
| Bánh Quy | Kỹ năng | Thường | 2 | Hồi 8 HP, Tiêu hao |
| Mổ Liên Hoàn | Tấn công | Thường | 1 | 3 sát thương ×3 |
| Vỗ Cánh | Kỹ năng | Thường | 0 | Rút 1 lá, +5 Nộ |
| Gáy Sáng | Kỹ năng | Thường | 1 | +2 Sức mạnh lượt này |
| Khiên Lông Vũ | Phòng thủ | Thường | 1 | 4 giáp, Giữ giáp 1 lượt |
| Lao Xuyên | Tấn công | Thường | 2 | 10 sát thương, Xuyên giáp |
| Phản Đòn | Phòng thủ | Hiếm | 1 | 6 giáp; bị đánh: phản 4 |
| Cú Mổ Độc | Tấn công | Hiếm | 1 | 5 sát thương, 3 Độc |
| Trứng Vàng | Kỹ năng | Hiếm | 1 | +1 năng lượng lượt sau, Tiêu hao |
| Gió Xoáy | Tấn công | Hiếm | 2 | 8 sát thương mọi địch |
| Thủ Thế | Phòng thủ | Hiếm | 2 | 12 giáp, rút 1 lá |
| Nộ Hỏa | Kỹ năng | Hiếm | 1 | +25 Nộ, nhận 1 Yếu |
| Đánh Dấu Mồi | Kỹ năng | Hiếm | 1 | Địch nhận Dễ Vỡ 2 lượt (+50% sát thương) |
| Lao Hồi Mã | Tấn công | Hiếm | 1 | 7 sát thương; lá về tay nếu kết liễu |
| Thánh Giáo | Năng lực | Huyền thoại | 3 | Mỗi lượt: lá tấn công đầu tiên ×2 |
| Pháo Đài Gà | Năng lực | Huyền thoại | 2 | Cuối lượt nhận 4 giáp |
| Mưa Sao Băng | Tấn công | Huyền thoại | 3 | 4 sát thương ×6 ngẫu nhiên |
| Tái Sinh Phượng | Kỹ năng | Huyền thoại | 2 | Lần chết đầu: hồi 30% HP, Tiêu hao |
| Cộng Hưởng Sao | Năng lực | Huyền thoại | 1 | Sao Rơi giảm 20 Nộ cần |
| Bùn Lầy | Lời nguyền | Lời nguyền | — | Không chơi được; chiếm chỗ |
| Mệt Mỏi | Lời nguyền | Lời nguyền | 1 | Tiêu hao, không hiệu ứng |

## Sự kiện

| Sự kiện | Lựa chọn → kết quả |
| --- | --- |
| Giếng Ước | Ném 30 vàng: 50% thánh vật \| Uống nước: hồi 15 HP \| Bỏ qua |
| Lò Rèn Cũ | Nâng cấp 1 lá \| Bỏ 1 lá \| Rèn giáo: +1 sát thương mọi Chọc Giáo, mất 8 HP |
| Ổ Trứng Lạ | Ấp: lá Huyền thoại + 1 Lời nguyền \| Ăn: +6 HP tối đa |
| Thương Nhân Rong | Đổi 1 thánh vật lấy 2 thánh vật thường \| Bỏ đi |
| Ếch Con Lạc | Đưa về: +1 đồng hành hồi 2 HP/trận \| Lấy đồ: 60 vàng + Lời nguyền |
| Đền Sao | Hiến 10 HP tối đa: Sao Rơi +15 sát thương \| Cầu nguyện: +20 Nộ đầu trận |
| Cầu Gãy | Liều qua: 50% đến thẳng elite có quà x2 \| Đi vòng: thêm 1 trận thường |
| Gánh Xiếc Gà | Thi đấu mini (3 lượt) thắng: 100 vàng \| Xem: rút 1 lá ngẫu nhiên |
| Sương Mù | Ẩn nút trên bản đồ 2 tầng; nhận 1 thuốc |
| Bóng Undergod | Ký khế ước: +1 năng lượng vĩnh viễn, boss cuối +25% HP \| Từ chối |
| Thư Viện Lông Vũ | Chọn 1 trong 3 lá Hiếm \| Học: biến 1 lá thường thành bản nâng cấp |
| Ổ Phục Kích | Đánh 2 địch thường liên tiếp, không hồi giữa trận; thưởng thánh vật |

## Lớp nhân vật và cây kỹ năng

Cây kỹ năng giữa các lần chạy dùng Lông Vũ (tiền meta): 3 nhánh × 5 nút.

| Lớp | Chủ đề | Tuyệt kỹ | Đặc điểm |
| --- | --- | --- | --- |
| Gà Chiến Binh | Giáo & Khiên, Nộ | Sao Rơi (40 dmg) | Cân bằng; nhánh Giáo / Khiên / Sao |
| Gà Pháp Sư | Phép & Độc, Mana tích lũy | Bão Lông (12 ×4 ngẫu nhiên) | Ít HP (40), rút thêm 1 lá |
| Gà Ninja | Đòn nhiều hit, Tiêu hao | Ảnh Phân (lặp 3 lá cuối) | Lá 0 năng lượng, combo |

Nhánh của Gà Chiến Binh:

| Nhánh | Nút 1 | Nút 3 | Nút 5 |
| --- | --- | --- | --- |
| Giáo | Chọc Giáo +1 dmg | Lá tấn công thứ 3 mỗi lượt +5 | Mở lá Thánh Giáo |
| Khiên | +5 giáp đầu trận | Giữ 3 giáp mỗi lượt | Mở Pháo Đài Gà |
| Sao | +10 Nộ đầu trận | Sao Rơi +10 dmg | Sao Rơi gây Dễ Vỡ |

## Thánh vật và thuốc

| Thánh vật | Độ hiếm | Hiệu ứng |
| --- | --- | --- |
| Lông Đuôi Phượng | Thường | Đầu trận +8 giáp |
| Hạt Ngô Vàng | Thường | +15 vàng mỗi trận |
| Mỏ Sắt | Thường | Lá tấn công đầu tiên mỗi trận +4 |
| Tổ Ấm | Thường | Nghỉ ngơi hồi thêm 10% |
| Chuông Bình Minh | Hiếm | Lượt 1 rút thêm 2 lá |
| Khiên Vỏ Ốc | Hiếm | Giữ tối đa 5 giáp sang lượt sau |
| Ngọc Nộ | Hiếm | Bắt đầu trận với 30 Nộ |
| Vương Miện Ếch | Boss | +1 năng lượng; rút ít hơn 1 lá |
| Mắt Diều Hâu | Boss | Thấy ý định 2 lượt tới của địch |
| Giáo Sao Băng | Boss | Sao Rơi đánh 2 lần, mỗi lần 60% |

Thuốc giữ tối đa 3:

| Thuốc | Hiệu ứng |
| --- | --- |
| Nước Cam Gà | Hồi 15 HP |
| Sữa Sắt | +10 giáp ngay |
| Rượu Nộ | +40 Nộ |
| Bùn Độc | Gây 8 Độc |
| Lọ Thời Gian | +2 năng lượng lượt này |
| Bột Lông | Rút 3 lá |

## Tiến trình và cân bằng

| Đối tượng | HP |
| --- | --- |
| Trùm cuối | 300 HP |
| Boss hồi 3 | 220 HP |
| Boss hồi 2 | 180 HP |
| Boss hồi 1 | 150 HP |
| Elite | 60–90 HP |
| Địch thường | 25–45 HP |

- Người chơi giữ 50 HP, 3 năng lượng, rút 5; bộ khởi đầu vẫn 10 lá.
- Sát thương địch thường 5–9/lượt, elite 9–14, boss 12–20 (có đòn báo trước 25–30).
- Vàng trung bình 300–400 mỗi hồi; lá ở cửa hàng 50/75/150, thánh vật 150–250.
- Độ khó Thử Thách 1–10: mỗi cấp thêm một bất lợi (địch +10% HP, ít hồi máu hơn...).
- Mục tiêu tỉ lệ thắng ở độ khó 0: 40–50% với người chơi mới.

## Kế hoạch kỹ thuật

Giữ nguyên nguyên tắc hiện tại: logic không phụ thuộc renderer, tốc độ animation không đổi kết quả RNG, mọi nội dung mới nằm trong JSON.

| Thành phần | Việc cần làm |
| --- | --- |
| `CardDef` | Thêm `type`, `rarity`, `keywords`, danh sách `effects` (op + giá trị) thay cho damage/block/heal cố định |
| `EffectSystem` (mới) | Thực thi effect: damage, block, heal, draw, energy, status, rage; dùng chung cho lá, thánh vật, thuốc |
| `StatusSet` (mới) | Độc, Yếu, Dễ Vỡ, Sức mạnh, Giữ giáp trên `Fighter`, tick đầu/cuối lượt |
| `EnemyDef` → `enemies.json` | Thêm `phases[]` (ngưỡng HP), `intentPattern` (chuỗi hoặc bảng trọng số), nhiều địch/trận |
| `Combat` | Hỗ trợ nhiều địch, chọn mục tiêu, hook sự kiện (OnTurnStart, OnCardPlayed, OnDamaged) cho thánh vật |
| `Run` (mới) | Deck, HP, vàng, thánh vật, thuốc, tầng hiện tại, seed; lưu `save.json` |
| `MapGen` (mới) | Sinh bản đồ phân nhánh theo seed, đọc tỉ lệ từ `map.json` |
| `EventDef` → `events.json` | Văn bản, lựa chọn, điều kiện, kết quả là danh sách effect |
| `relics.json`, `potions.json`, `classes.json` | Dữ liệu thánh vật, thuốc, lớp nhân vật, cây kỹ năng |
| `Ui` | Thêm màn: Bản đồ, Phần thưởng, Cửa hàng, Sự kiện, Nghỉ, Chọn lớp; Game chuyển thành state machine các màn |
| `CoreChecks` | Kiểm thử theo seed cho MapGen, effect, pha boss; bot tự chơi 1000 lượt để đo tỉ lệ thắng |

## Lộ trình

| Mốc | Nội dung |
| --- | --- |
| v0.2 – Nền tảng effect | EffectSystem, StatusSet, CardDef mới, 15 lá mới, Ếch Vương 2 pha, thưởng chọn lá sau trận |
| v0.3 – Một hồi roguelike | Run, MapGen, trận thường/elite/nghỉ/cửa hàng, 6 địch thường, 2 elite, 10 thánh vật, 6 thuốc, lưu game |
| v0.4 – Sự kiện & 3 hồi | 12 sự kiện, Rắn Bùn Mẹ, Diều Hâu Bão, Cáo Thầy Phù, Undergod, tổng 40+ lá |
| v0.5 – Lớp nhân vật & meta | Gà Pháp Sư, Gà Ninja, cây kỹ năng Lông Vũ, độ khó Thử Thách 1–10 |
| v1.0 – Hoàn thiện | Cân bằng bằng bot tự chơi, cinematic cho tuyệt kỹ mới, âm thanh, thành tựu, bản phát hành |
