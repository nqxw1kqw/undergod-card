# undergod-card

Game thẻ bài theo lượt với **C++17, DxLib, Visual Studio 2022 và Windows x64**. Gà Chiến Binh dùng giáo, khiên và thẻ bài để đấu với Ếch Vương.

![Trận đấu Gà Chiến Binh và Ếch Vương](UndergodCard/docs/images/battle.png)

- Hành trình ngắn bốn trận, kết bằng Ếch Vương hai giai đoạn. HP và Nộ được giữ; giáp và trạng thái xóa khi sang trận mới.
- Bộ khởi đầu 10 lá, rút 5, 3 năng lượng. Sau mỗi trận trước trùm, chọn 1 trong 3 lá thưởng. Bộ thưởng có 15 lá, chia độ hiếm và từ khóa.
- Hiệu ứng đọc từ JSON: sát thương, giáp, hồi máu, Độc, Suy Yếu, Dễ Thương, Sức Mạnh, rút bài. Ý định địch hiện sát thương đã tính các trạng thái đó.
- Chiêu **Sao Rơi**: gà phóng lao lên trời, lao rơi xuống địch và gây sát thương lúc chạm.
- Nhịp x1 chậm mặc định; nút x1/x2/x3 hoặc F1/F2/F3 đổi tốc độ cả giữa đòn đánh.
- 50 PNG pastel, UI tiếng Việt; chạy trực tiếp bằng DxLib. Lá mới chưa có ảnh riêng thì hiện ký hiệu dự phòng.

## Chơi ngay

Tải `UndergodCard-DxLib-Windows-x64.zip` trong [Releases](https://github.com/nqxw1kqw/undergod-card/releases), giải nén và chạy `UndergodCard.exe`. Giữ các thư mục `assets`, `data`, `licenses` cạnh EXE.

Chuột hoặc **1–9** chơi thẻ, **E** hết lượt, **U** dùng Sao Rơi khi đủ Nộ. Sau trận, **1–3** hoặc chuột chọn một lá thưởng. **Space / Esc** bỏ qua cinematic. **R / Enter** chơi lại từ trận đầu.

## Mở và build

1. Cài Visual Studio 2022 với bộ công cụ C++ cho desktop, MSVC v143 và Windows SDK.
2. Cài bộ DxLib cho Visual C++ x64 tại `C:\DxLib`, hoặc chỉ định `DxLibDir` khi build.
3. Mở [UndergodCard.sln](UndergodCard/UndergodCard.sln), chọn `UndergodCard`, `Release | x64` và nhấn F5.

Từ thư mục gốc repo:

```powershell
.\UndergodCard\build.ps1 -Configuration Release
.\UndergodCard\verify.ps1 -Configuration Release
.\UndergodCard\package.ps1 -SkipBuild
```

Với SDK ở vị trí khác:

```powershell
.\UndergodCard\build.ps1 -DxLibDir 'D:\Libraries\DxLib'
```

EXE sinh tại `UndergodCard/bin/Release/UndergodCard.exe`; ZIP sinh tại `UndergodCard/release/`. SDK DxLib được cài riêng, không kèm trong repo.

## Cấu trúc và kiểm chứng

`UndergodCard/` chứa solution, source, JSON, 50 asset runtime và bộ kiểm tra. Core combat tách khỏi renderer; tốc độ và animation không đổi kết quả damage hay RNG. Logic chạy không cần DxLib:

```bash
make -C UndergodCard core
```

Bộ đó có **134 kiểm tra**, gồm seed 1337 tự đi hết bốn trận, giữ HP 47/40/40 rồi thắng trùm ở giai đoạn 2 với 21 HP. Trên Windows, `verify.ps1` build solution, chạy cùng bộ kiểm tra, rồi smoke DxLib tự chơi hành trình và yêu cầu `boss_phase=2`.

Xem [hướng dẫn gameplay và source](UndergodCard/README.md), [kiểm chứng](UndergodCard/docs/VALIDATION.md) và [thành phần bên ngoài](UndergodCard/THIRD_PARTY.md).

Repo chỉ chứa project native hiện tại. Bản web, kho RE, ảnh gốc sinh asset và công cụ hậu xử lý được lưu riêng tại máy phát triển; build và chạy game không phụ thuộc các bản tham khảo đó. File build, cache và gói EXE được bỏ qua bởi Git.
