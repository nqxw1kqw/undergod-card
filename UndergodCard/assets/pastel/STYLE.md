# Art runtime — pastel mực tay

Dựa trên gà vàng Shin cung cấp: dáng tròn cute, mắt đen, khăn đỏ, khiên gỗ/giáo; pastel ấm, viền than mềm, highlight kem. Pháp Sư tím, Ninja chàm/đỏ; boss và địch có thiết kế riêng cùng nét vẽ.

190 PNG: 24 frame Chiến Binh, 20 Pháp Sư, 20 Ninja; 40 frame cho năm boss; chín ảnh địch thường/tinh anh/Nòng Nọc; 50 icon bài; 17 di vật; sáu thuốc; bốn VFX.

Nhân vật RGBA 512×512, chân y=456. Icon/feather/kunai RGBA 256×256. spear.png/impact 512×512; sheet impact 2×2 phát 8 fps. Mũi lao neo (256,56), xoay khi rơi. ult_throw Chiến Binh 4 fps, hai frame cuối tay trống.

Raw animation từng action là grid 2×2 nền magenta; icon atlas 4×4/3×3. Processor loại nền/alpha, tách cell, căn chân, chuẩn hóa body giữa action. Atlas gutter lệch được repack từ cell nguyên vẹn. Không vẽ bù sprite bằng code.

Địch thường một ảnh riêng, bob/lunge/recoil bằng renderer; không giả frame bằng nhân bản. Boss có idle/attack; thiếu hit/defend dùng idle kèm phản ứng/khiên.

tools/import_art.py dùng processor generate2dsprite ở máy phát triển, không chạy khi build. assets/provenance.json giữ tên/hash nguồn và prompt đã lưu; raw/QC ở artifacts/art-v1 ngoài Git. assets/manifest.json giữ kích thước/hash từng PNG và hash 12 WAV.

Nhịp x1 dùng đồng hồ 0,65 lần thời gian thực; x2/x3 nhân đồng hồ đồng bộ sprite/projectile/camera/popup, không đổi kết quả combat.
