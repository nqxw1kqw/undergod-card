# Hợp nhất draft và main — v1.4.0-preview

Nhánh `integrate/draft-main-optimizations` lấy **draft/v0.3-wip** làm nền. Hai đầu nguồn đã đối chiếu:

- Draft: `65b383b0f9befb5ec36862e801db9efb3e045839`.
- Main: `2a70e0277527e02768afc7431b4356966961d858`.
- Main phát triển tiếp từ prototype một boss/bốn lá; không có chiến dịch hoặc bộ Turn Star của draft. Vì vậy không chép nguyên core/UI cũ đè lên draft.

Merge commit giữ cả hai đầu lịch sử làm cha. Nội dung cây draft được giữ trước, sau đó chuyển từng cải tiến từ main sang controller/renderer hiện tại và kiểm tra lại. `main` và `draft/v0.3-wip` không bị thay đổi.

## Đối chiếu từng phần

Số mục tương ứng [đề xuất gốc từ main](../../IMPROVEMENT_PROPOSAL.md). [Phase 2](../../PHASE2_RESULTS.md), [Phase 3](../../PHASE3_RESULTS.md), [Phase 4](../../PHASE4_RESULTS.md) được giữ nguyên làm tài liệu lịch sử; số liệu prototype trong đó không phải số liệu của bản tích hợp.

| Mục từ main / commit | Cách tích hợp trên nền draft | Bằng chứng |
| --- | --- | --- |
| 1 — Seed và đo nhiều chính sách, `8e851c3` | Tách seed phiên khỏi RNG combat; N tạo seed mới, Shift+R chơi lại cùng seed/lớp/độ khó, hai nút tương ứng ở màn kết quả. Runner mới cho cả sáu boss, ba lớp, ba mức khó và ba chính sách. | `integration_checks.cpp`, `integration_balance.cpp`; lặp lại cùng seed so cả trace thứ tự và snapshot/RNG cuối. |
| 2 — Hiệu quả thực của lá, `d94c236` | Bản xem trước chạy resolver trên bản sao: HP thực, giáp hấp thụ, hồi giới hạn bởi HP tối đa, Nộ giới hạn. Mỗi lá vẫn dùng một sao. Giữ chỉ số 98 lá của draft. | Không đổi snapshot hoặc ba RNG; test giáp một phần, hồi gần đầy, Nộ 99, lá khóa. Main cũng đã quyết định giữ sát thương gốc sau đo thử. |
| 3 — Chuyển động bài, `db04d5c` | Mỗi bản bài trùng tên có ID hình ảnh riêng và snapshot sở hữu giá trị. Tay → hàng sao → ô thi triển → bỏ/tiêu hao; gỡ lá trở về tay, phần còn lại dịch vị trí; chia lần lượt sau FIGHT/hồi động tác. Vùng bấm đi theo hình đang vẽ. | Test ID trùng, remap chỉ số sau core xóa bài, thời điểm sát thương; ảnh DxLib queue/unqueue/play. |
| 4 — Phản hồi giới hạn, `6adc63b` | RNG hình ảnh riêng; tối đa 64 hạt/0,4 giây, tám vị trí popup mỗi đơn vị/0,95 giây, rung thường 2–4 px/0,18 giây. Tách thông báo chặn và mất HP. Nút FX nhẹ tắt rung/flash/zoom, lưu lựa chọn trong profile. | Test giới hạn/xóa FX, hai popup; smoke FX khi cinematic đang chạy. |
| 5 — Ranh giới lượt, `a5f4d4f` | Giữ thứ tự ta1/địch1/ta2/địch2/ta3/địch3 của draft. Lượt mới chỉ chia bài sau lá thứ sáu hồi xong. Hiện tên phe đang thi triển/hồi động tác; ẩn dự báo lúc đang chạy. Dự báo khi chuẩn bị mô phỏng chính hàng bài đã chọn trên bản sao. | Test Turn Star/hoạt ảnh cũ tiếp tục qua; 300 hành trình giữ nguyên kết quả. |
| 6 — Timing và động tác, `f5a4f44` | `Timing()` dùng cấu hình từng kỹ năng của draft, giữ 18 bộ cơ thể/17 FX và khiên theo giáp thật. Chuyển động hồi mượt; hiệu ứng va chạm nhận phần thời gian vượt mốc contact. | Test x1/x2/x3, contact đúng một lần, đòn kết liễu, projectile/khiên/Sao Chép và 98 binding. |
| 7 — Mức khó/báo trước, `8c6c392` | **Đấu thử** có Nhẹ/Chuẩn/Khó, sát thương địch −2/0/+4; HP, bộ bài boss, seed và thứ tự RNG giữ nguyên. D/nút mức khó chơi lại cùng seed, khóa khi đang hành động. Giữ tụ lực/vồ của Hổ và charge của Rắn, không gắn vòng charge 34 sát thương/60% HP của prototype lên sáu boss. Thử Thách 0–10 của chiến dịch giữ riêng. | Đo lại 3.240 trận và 3.240 replay; bảo toàn đa tập bài sau mỗi hành động, không kẹt. Các mức này vẫn là preview, không phải cân bằng đã xác nhận bằng người chơi. |
| 8 — Kết thúc trận, `c817ef0` | Core xác định chết ngay ở contact; hình ảnh giữ pose trúng đòn, dùng bộ lọc đơn sắc DxLib, nghiêng/mờ và bụi. Khóa 1,5 giây logic, fade kết quả từ 0,8–1,5 giây rồi mới thưởng. Recap đếm từ event thực, giữ ở thưởng/kết quả: lượt/lá/HP/giáp/hồi/Nộ tuyệt kỹ/seed. | Test lethal thường và ultimate bỏ qua hai lần, chỉ settle một lần; smoke bộ lọc, pose, fade, recap. |
| 9 — Thông tin/điều khiển, `ead958d` | Chi tiết đầy đủ khi rê/focus; ←/→ chọn lá và Space xếp hàng; 1–0 hỗ trợ tay 10 lá. P/O mở chồng rút/bỏ theo tên và số lượng, có phân trang, không lộ thứ tự. Modal chặn chọn bài/Attack/chờ/thuốc. | Resolver không đổi RNG; đa tập không đổi khi đảo thứ tự, test modal; ảnh UTF-8/focus/pile trong DxLib. |
| 11 — Zoom liên tục, `2de7157` | Mỗi cue lấy giá trị nội suy của segment trước tại đúng timestamp. Cue 3.000 ms đưa camera về 1 trong 550 ms; skip/reset/FX nhẹ về trung tính. | Test nhảy nhiều mốc trong một update, fine/coarse, skip và ảnh trước/sau FX nhẹ. |

Mục **10** (vòng thưởng giữa trận) và **12** (âm thanh) là phần main còn hoãn, không phải code bị bỏ sót khi hợp nhất. Draft đã có ba hồi, thưởng/bộ bài/di vật/meta và 12 WAV cùng mute; các hệ thống đó được giữ. Không thêm hit-pause hay nội suy thanh HUD vốn mới nằm trong đề xuất.

## Nền draft được giữ

- C++17 / DxLib / MSVC v143 / Visual Studio 2022, framebuffer và client **1600×900**.
- Ba hồi, ba lớp, 50 lá gốc +48 nâng cấp, sáu boss/36 lá riêng, 45 kỹ năng meta, lưu/tải và ba RNG độc lập.
- Avatar tới đúng điểm map, chuyển cảnh đen, FIGHT và chia bài; ba Turn Star luân phiên; hoạt ảnh theo skill cho cả hai phe.
- Toàn bộ **379 PNG và 12 WAV** giữ nguyên hash so với draft. Không dùng lại 50 asset cũ của prototype main. Trong 18 JSON runtime, chỉ `cinematic.json` đổi cue camera.
- Profile v1 đọc được như cũ; `reducedFeedback` là trường tùy chọn mặc định false. Session v2 giữ nguyên. Đấu thử không ghi đè save hoặc nhận thưởng meta.
- HTML/website ngoài phạm vi; thư mục sibling hiện không có trong workspace này, không tuyên bố kiểm tra lại web.

## Chạy lại

```powershell
.\verify.ps1 -Configuration Release
.\verify.ps1 -Configuration Debug
.\bin\Release\CoreChecks.exe --simulate-stars 300
.\bin\Release\CoreChecks.exe --integrate-balance 20
.\package.ps1 -SkipBuild
python tools/validate_package.py
```

Mỗi cấu hình verify chạy core và bốn smoke DxLib: legacy, Turn Star, hoạt ảnh skill, tích hợp. `tests/run_portable.sh` dùng Makefile để build chung một binary core và tái sử dụng nó cho loạt nhiều chính sách; lệnh Linux được cung cấp nhưng chưa chạy trên Linux ở mốc này.

Kết quả, ảnh renderer và giới hạn kiểm chứng hiện tại ở [VALIDATION.md](VALIDATION.md). Bot và handler UI không thay thế một lượt chơi tay toàn chiến dịch hoặc xác nhận cảm giác x3.
