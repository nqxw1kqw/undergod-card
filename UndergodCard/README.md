# Undergod Card Quest — v0.3 / C++17 / DxLib

Một hồi roguelike 13 tầng, 3 cột phân nhánh. Gà Chiến Binh khởi đầu với 50 HP, 3 năng lượng, rút 5 lá; vượt trận thường, tinh anh, sự kiện, cửa hàng, nghỉ rồi Ếch Vương có hai thanh máu 80 + 70 HP.

## Chạy, build và đóng gói

Mở UndergodCard.sln trong Visual Studio 2022, chọn UndergodCard, Release | x64. MSVC v143 và Windows SDK cần được cài. DxLib mặc định ở C:\DxLib.

    .\build.ps1 -Configuration Release
    .\verify.ps1 -Configuration Release
    .\verify.ps1 -Configuration Debug
    .\bin\Release\CoreChecks.exe --simulate 1000
    .\package.ps1 -SkipBuild

Cả ba script nhận -DxLibDir để đổi vị trí SDK. EXE sinh tại bin/Release/UndergodCard.exe; ZIP tại release/UndergodCard-DxLib-Windows-x64.zip. Giữ assets, data và licenses cạnh EXE. Bản đã build không cần cài SDK để chơi.

Core không dùng DxLib; Makefile được giữ để build bằng g++ với C++17. Phần lưu dùng thay file nguyên tử theo nền tảng. Kiểm chứng của đợt này chạy bằng MSVC trên Windows.

## Điều khiển

- Chuột hoặc 1–9: chọn lối đang mở / chơi lá bài. 1–3 chọn thưởng.
- E hết lượt; U Sao Rơi đủ 100 Nộ; Space/Esc bỏ qua cinematic.
- F1/F2/F3 hoặc x1/x2/x3 đổi tốc độ. x1 mặc định chậm, khoảng 1 giây mỗi đòn thẻ.
- Bản đồ: nút cuộn, bánh xe chuột hoặc Page Up/Page Down; tự cuộn tới vị trí hiện tại.
- Cửa hàng: 1–7 mua, bấm lá để gỡ với giá 75 vàng; E/Esc rời cửa hàng.
- Nghỉ: H hồi HP (đã tính di vật và giới hạn HP), hoặc 1–8 nâng cấp lá trên trang. Chỉ được chọn một việc.
- Bộ bài trong cửa hàng/nghỉ có 8 lá mỗi trang; mọi lá đều thao tác được qua nút trang, bánh xe hoặc Page Up/Page Down.
- Z/X/C dùng ba ô thuốc; V dùng ô thứ tư khi di vật mở thêm ô.
- F6 / Lưu: ghi checkpoint. F9 / Tiếp tục: tải checkpoint. Khi mở lại game, tự tiếp tục save.json.
- R/Enter/Chơi lại bắt đầu hành trình mới với seed mới. Chạy EXE với --seed 1337 để bắt đầu seed cụ thể.

## Gameplay và dữ liệu

Bộ khởi đầu: 5 Chọc Giáo, 3 Khiên Gỗ, 1 Đâm Mạnh, 1 Bánh Quy. Có 15 lá thưởng thêm, cùng bản nâng cấp sinh từ dữ liệu. Sức Mạnh cộng từng nhát, Suy Yếu nhân 3/4, Dễ Vỡ nhân 3/2, làm tròn xuống. Độc xuyên giáp đầu lượt rồi giảm một tầng. HP và Nộ giữ giữa các trận; giáp và trạng thái trận được xóa.

- map.json: số tầng/cột, tầng sớm nhất có tinh anh, trọng số Trận/Tinh anh/Sự kiện/Shop/Nghỉ 45/12/22/8/13, khoảng vàng. Tầng đầu luôn là trận thường; tầng ngay trước trùm luôn nghỉ. Hai hàng bắt buộc và hạn chế tinh anh sớm là ngoại lệ có chủ ý của trọng số.
- run.json: các pool địch thường/tinh anh, boss, vàng đầu, số ô thuốc, giá gỡ và mức hồi khi nghỉ.
- enemies.json: 6 địch thường, 2 tinh anh, boss hai giai đoạn, minFloor (đánh số từ 0), bảng ý định. Mỗi trận vẫn đối đầu một địch.
- cards.json: effects damage/block/heal/apply/draw/energy/rage, rarity và keywords. Ảnh cards/id.png là tùy chọn; chưa có ảnh dùng ký hiệu. Thêm hiệu ứng mới cần mở rộng resolver.
- relics.json / potions.json: 12 di vật và 6 thuốc. Di vật tinh anh chỉ thưởng một lần, không trùng; khi đã sở hữu hết thì đổi thành vàng.
- events.json: 10 sự kiện, mỗi lựa chọn có kết quả. Lựa chọn không đủ vàng giữ nguyên màn và tài nguyên.
- cinematic.json: pose, quỹ đạo lao và impact tại 2600ms của đồng hồ gameplay; speed không đổi damage/RNG.

## Lưu game

save.json nằm cạnh EXE, tự ghi sau hành động ổn định và chuyển màn. Có version, đường đi, HP/Nộ/vàng, bộ bài, di vật, thuốc, hàng đã mua, phần thưởng chưa nhận, trạng thái combat, phase boss và cả hai RNG.

Trong animation/cinematic, lưu và tải bị khóa để tránh nửa impact. Ghi ra save.json.tmp rồi thay file; lỗi ghi không phá checkpoint trước. Tải qua đối tượng tạm; file sai, khác layout map hay phiên bản không được áp vào run hiện tại. Nếu bản lưu tự mở bị hỏng, game giữ một bản save.json.invalid.* trước khi tạo run mới.

Muốn mang hành trình sang máy khác, chép save.json cạnh EXE cùng phiên bản dữ liệu. Không đưa bản lưu vào Git hoặc ZIP phát hành.

## Cấu trúc

- quest_core / quest_effects / quest_status: luật combat và snapshot; không phụ thuộc renderer.
- quest_run: map, chuyển nút, sự kiện, shop, nghỉ, di vật/thuốc và bot.
- quest_save: JSON checkpoint, kiểm tra dữ liệu và thay file.
- game: animation/contact, khóa input, autosave khi ổn định.
- ui: DxLib, hit-test cùng hình vẽ, phân trang, cuộn và chữ Việt.
- tests/core_checks.cpp và tests/run_checks.cpp: logic cũ, map theo seed, lưu/tải, tệp lỗi và hành trình.
- assets/manifest.json: SHA-256 của 50 PNG; third_party giữ giấy phép.

## Kiểm chứng và giới hạn

Xem docs/VALIDATION.md để lấy số liệu lần chạy hiện tại. verify.ps1 yêu cầu đủ 50 PNG, chiến thắng cả hồi và boss_phase=2; ảnh/báo cáo sinh tại bin/<cấu hình>/artifacts.

Chưa có ba hồi, lớp Pháp Sư/Ninja, cây kỹ năng, nhiều địch cùng trận, âm thanh, save tương thích mọi phiên bản hoặc cân bằng theo người chơi thật. Nhiều địch dùng chung sprite và 15 lá thưởng chưa có ảnh riêng. Header ngoài C:\DxLib có thể báo C4010.
