# Undergod Card Quest — C++ / DxLib

Bản hiện tại thay gameplay và toàn bộ asset cũ của Undergod bằng Card Quest pastel: Gà Chiến Binh đấu Ếch Vương trên sân ngang. Chạy trực tiếp trong **Visual Studio 2022 / MSVC v143 / C++17 / DxLib / x64**. Không cần npm, Electron hoặc server web.

## Chạy và build

Mở `UndergodCard.sln`, chọn project `UndergodCard`, cấu hình `Release / x64`, nhấn F5. DxLib mặc định tại `C:\DxLib`.

```powershell
.\build.ps1 -Configuration Release
.\verify.ps1 -Configuration Release
.\verify.ps1 -Configuration Debug
.\package.ps1 -SkipBuild
```

EXE: `bin/Release/UndergodCard.exe`. Game tự đặt thư mục làm việc về thư mục EXE. Khi mang sang máy khác, dùng `release/UndergodCard-DxLib-Windows-x64.zip`, giải nén và giữ nguyên EXE cùng hai thư mục `assets`, `data`.

Cả ba script nhận `-DxLibDir 'D:\Libraries\DxLib'` để đổi vị trí SDK. Khi mở trực tiếp Visual Studio, đặt thuộc tính MSBuild `DxLibDir` tương ứng. nlohmann/json đã kèm trong `third_party`.

`package.ps1` mặc định build Release trước khi đóng gói; `-SkipBuild` dùng lại bản đã kiểm tra. ZIP có thư mục `licenses` kèm thông báo bên ngoài; không đưa PDB, object, test executable hoặc ảnh gốc vào ZIP. Các đầu ra build và smoke test được sinh lại, không commit lên Git.

## Cách chơi

- Chuột hoặc phím 1–5: chơi thẻ tương ứng. Tấn công tự nhắm Ếch Vương.
- E: kết thúc lượt. U: Sao Rơi khi đủ 100 Nộ.
- Nút x1/x2/x3 hoặc F1/F2/F3: đổi tốc độ. Mỗi run bắt đầu ở x1 chậm; vẫn đổi được khi đang đánh hoặc xem cinematic.
- Space / Esc: bỏ qua cinematic; sát thương chưa áp vẫn được xử lý đúng một lần.
- R / Enter hoặc nút Chơi lại: bắt đầu lại khi không bị khóa input.
- Đóng bằng nút X của cửa sổ. Tọa độ chuột được đổi từ client Windows về canvas để khớp nút khi hiển thị ở kích thước khác.

Gà Chiến Binh có 50 HP, 3 năng lượng và rút 5 lá mỗi lượt. Bộ khởi đầu gồm 5 Chọc Giáo (1 năng lượng/6 damage), 3 Khiên Gỗ (1/5 giáp), 1 Đâm Mạnh (2/12 damage), 1 Bánh Quy (2/hồi 8 HP). Ếch Vương có 120 HP, chọn đánh 8–13 hoặc nhận 6 giáp theo trọng số 70/30; mở đầu luôn tấn công và tối đa một lượt thủ liên tiếp. Giáp của mỗi bên xóa trước hành động/lượt kế tiếp của chính bên đó. Đánh tăng 8 Nộ, phòng thủ tăng 5, tối đa 100; Sao Rơi gây 40 damage, trừ giáp trước rồi trừ HP.

## Cấu trúc

- `include/quest_core.hpp`: combat thuần C++, damage/block, deck, RNG, tài nguyên, thắng/thua; không include DxLib và không dùng thời gian thực.
- `src/quest_data.cpp`: đọc/kiểm tra JSON bằng nlohmann/json đã kèm trong `third_party`, có giấy phép MIT.
- `include/quest_timeline.hpp`: trình phát timeline, bảo đảm impact/end chỉ một lần khi bỏ qua hoặc nhảy frame.
- `src/game.cpp`: khóa input, lunge/contact, chuyển lượt, popup, pose, camera và cinematic.
- `src/ui.cpp`: renderer/input DxLib, tiếng Việt UTF-8, sân parallax, thẻ và 50 PNG mới.
- `src/main.cpp`: vòng lặp Windows và chế độ `--smoke`.
- `data/*.json`: thông số, thẻ, nhân vật, địch và cinematic; build tự chép cạnh EXE.
- `tests/core_checks.cpp`: 93 kiểm tra độc lập renderer.
- `assets/pastel`: 50 PNG runtime; `assets/manifest.json` ghi kích thước và SHA-256.
- `docs`: cấu trúc, kiểm chứng và ảnh gameplay; `THIRD_PARTY.md` ghi nguồn asset và thành phần bên ngoài.

## Sửa dữ liệu

Thêm thẻ trong `data/cards.json` với ID duy nhất, tên, cost, description, copies và damage/block/heal. Đặt ảnh 256×256 tại `assets/pastel/cards/<id>.png`. Core hỗ trợ kết hợp các hiệu ứng này; hiệu ứng mới cần mở rộng CardDef/resolver. Build lại để chép dữ liệu/ảnh mới sang bin. Số lá tay trên UI mặc định thiết kế cho draw=5; draw có thể đặt tối đa 10.

`data/balance.json` điều khiển HP/energy/draw/Nộ/damage tuyệt chiêu. `enemies.json` điều khiển intent. `cinematic.json` khai báo mốc thời gian, dim, zoom, actor/pose, sheet/target, shake, impact và flash. Sao Rơi hiện là gà giơ giáo, phóng lao lên trời, lao rơi đúng boss rồi áp damage khi chạm. Quỹ đạo từ/to/duration nằm trong action moveSpear; loader yêu cầu mốc kết thúc rơi khớp impact. Hiệu ứng va chạm dùng sheet 4 frame; không dùng ngôi sao cũ.

RNG Mulberry32 và hai stream shuffle/intent giữ cùng cách seed với bản TypeScript. Giá trị RNG và tay bài mở đầu được đối chiếu golden test từ bản đó. Luật AI mới ép đòn mở đầu và chặn thủ liên tiếp nên chuỗi intent mặc định đã đổi; chuỗi cũ vẫn có golden test khi tắt chính sách mới. Core áp damage tại callback contact/impact; animation không quyết định giá trị damage.

## Asset và kiểm chứng

Renderer chỉ nạp `assets/pastel`: 44 frame nhân vật, 4 ảnh thẻ, 1 ảnh lao và 1 sheet va chạm. Khi thiếu PNG, actor/card dùng hình/ký hiệu dự phòng; bản chuẩn smoke test yêu cầu đủ 50/50. Ảnh nguồn, prompt, bản web và kho RE được lưu riêng ngoài repo, không cần để build hoặc chạy.

Debug và Release đạt 93 kiểm tra, renderer smoke PASS. Bài trận đầy đủ seed 1337 dùng API gameplay thật thắng ở lượt 7 với 27/50 HP. Chạy lại `verify.ps1` để sinh báo cáo và ảnh tại `bin/<cấu hình>/artifacts`. Xem `docs/VALIDATION.md` và ảnh minh họa tại `docs/images`.

Chưa có lưu/tải, âm thanh, phát MP4, map nhiều trận hoặc đánh giá cân bằng dài hạn. Cảnh báo C4010 trong header ngoài `C:\DxLib\DxFunctionWin.h` có thể xuất hiện. SDK DxLib được cài riêng, không kèm trong repo.

Nhịp mới: x1 dùng đồng hồ 0,65 lần thời gian thực, một đòn thẻ khoảng 1 giây và cinematic khoảng 5,54 giây. x2/x3 lần lượt nhanh gấp 2 và 3 so với x1, không đổi damage, tay bài hoặc RNG. Toast giữ thời gian thực để đọc được. Bộ throw/projectile/impact do công cụ sinh ảnh tạo, nguồn và công cụ hậu xử lý lưu riêng tại máy phát triển.
