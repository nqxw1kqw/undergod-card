# Art runtime — pastel mực tay

Dựa trên gà vàng Shin cung cấp: dáng tròn cute, mắt đen, khăn đỏ, khiên gỗ/giáo; pastel ấm, viền than mềm, highlight kem. Pháp Sư tím, Ninja chàm/đỏ; boss và địch có thiết kế riêng cùng nét vẽ.

379 PNG: giữ nguyên 239 ảnh đã duyệt (24 frame Chiến Binh, 20 Pháp Sư, 20 Ninja; 52 frame boss; chín địch; 50 bài player; 36 bài boss; 17 di vật; sáu thuốc; bốn VFX; một FIGHT), thêm 72 frame cơ thể và 68 frame hiệu ứng theo kỹ năng.

Banner `ui/fight.png` là ảnh ImageGen RGBA8 2172×724, nền trong suốt, chữ kem/vàng viền xanh; giữ nguyên đầu ra và render theo tỉ lệ. Banner vào từ phải, dừng giữa rồi ra trái trước khi chia bài. Icon `boss_cards/boss_<boss>_<chiêu>.png` là RGBA8 256×256, sáu ảnh/boss, tạo từ sáu pack 2×3 theo đúng sprite tham chiếu. Không thay các asset cũ.

Nhân vật RGBA 512×512, chân y=456. Icon/feather/kunai RGBA 256×256. spear.png/impact 512×512; sheet impact 2×2 phát 8 fps. Mũi lao neo (256,56), xoay khi rơi. ult_throw Chiến Binh 4 fps, hai frame cuối tay trống.

Raw animation từng action là grid 2×2 nền magenta; icon atlas 4×4/3×3. Processor loại nền/alpha, tách cell, căn chân, chuẩn hóa body giữa action. Atlas gutter lệch được repack từ cell nguyên vẹn. Không vẽ bù sprite bằng code.

Địch thường một ảnh riêng, bob/lunge/recoil bằng renderer; không giả frame bằng nhân bản. Player có đâm/mổ/giậm/niệm/đỡ hoặc chém/ném theo lớp; boss có niệm, Hổ có đỡ và vồ. Mỗi bộ bốn frame khác nhau. Phần đạn/va chạm tạo riêng khỏi cơ thể để không bị cắt khi bay xa.

17 bộ `skill_fx` có cầu lửa/phép/độc, vệt chém/vuốt, bốn lớp khiên rỗng ở giữa, hồi phục, dấu phép, khói, lưỡi, sóng giậm, sét, sao băng và triệu hồi. 35 sheet 2×2 được tách/chroma key/khử viền magenta; các cơ thể dùng cùng hệ số trong một bộ và chân y=456. Manifest có biên alpha cho FX cần kéo dài. Khiên dùng alpha để vẫn nhìn rõ cơ thể, giữ theo số giáp của đơn vị.

tools/import_art.py dùng processor generate2dsprite ở máy phát triển, không chạy khi build. assets/provenance.json giữ tên/hash nguồn và prompt đã lưu; raw/QC ở artifacts/art-v1 ngoài Git. assets/manifest.json giữ kích thước/hash từng PNG và hash 12 WAV.

`tools/import_skill_art.py` và `artifacts/skill-art` dành cho 140 frame mới; đủ prompt/hash/QC ở `assets/provenance.json` → `skill_animations`. Chi tiết chuyển động và thời điểm chạm ở [SKILL_ANIMATIONS](../../docs/SKILL_ANIMATIONS.md).

Nhịp x1 dùng đồng hồ 0,65 lần thời gian thực; x2/x3 nhân đồng hồ đồng bộ sprite/projectile/camera/popup, không đổi kết quả combat.
