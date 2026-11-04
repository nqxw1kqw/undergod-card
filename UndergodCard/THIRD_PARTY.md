# Thành phần bên ngoài và asset

- **nlohmann/json**: dùng để đọc dữ liệu JSON; mã nguồn ở `third_party/nlohmann/json.hpp`, giấy phép MIT nguyên bản ở `third_party/nlohmann/LICENSE.MIT`.
- **DxLib**: SDK cài riêng để build, không đưa header hoặc thư viện SDK vào repo. Thông tin bản quyền từ SDK đang dùng được giữ nguyên ở `third_party/DxLib-notice.txt`. Chức năng đọc PNG có các ghi nhận trong tài liệu đó:

  libpng Copyright (C) 2004, 2006-2012 Glenn Randers-Pehrson.

  zlib Copyright (C) 1995-2012 Jean-loup Gailly and Mark Adler.

- **Asset runtime**: 50 PNG được tạo bằng công cụ sinh ảnh và hậu xử lý alpha/căn frame. Nhân vật gà dựa theo ảnh tham khảo Shin cung cấp. Manifest kích thước/SHA-256 ở `assets/manifest.json`; quy cách ở `assets/pastel/STYLE.md`. Ảnh gốc, prompt và công cụ xuất ảnh được lưu riêng tại máy phát triển.

Gói EXE chép các thông báo và giấy phép trên vào thư mục `licenses/`. Repo không kèm mã nguồn hoặc tài nguyên trích xuất từ Slay the Spire.
