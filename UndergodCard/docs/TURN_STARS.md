# Turn Star và mở trận — v1.1.0-preview

Bản v1.2.0 giữ các luật dưới đây, sửa icon quay về điểm cũ trong FadeOut và tách 36 lá riêng của sáu boss. Cách chọn bài theo pha và nhịp tụ lực mới ở [BOSS_DECKS.md](BOSS_DECKS.md).

Thay đổi dành cho game gốc C++17 / DxLib / Visual Studio. Demo HTML và website được giữ riêng, không cập nhật theo thay đổi này.

## Bản đồ và mở trận

Avatar dùng đúng lớp gà đang chơi. Khi chọn một điểm hợp lệ, avatar di chuyển từ vị trí hiện tại đến điểm đó; trong lúc đi, thao tác chọn điểm tiếp và lưu/tải bị khóa. Chỉ khi màn hình đã đen mới xác nhận vào khu vực mới. Màn đen được giữ một nhịp ngắn rồi sáng lại, cả với shop, nghỉ và sự kiện.

Nếu khu vực là trận đấu, ảnh **FIGHT** đi từ ngoài mép phải, dừng ở giữa, rồi ra khỏi mép trái. Sau đó các lá bay lần lượt từ chồng rút vào tay; chỉ khi chia xong mới được chọn bài. Các vòng sau cũng có hiệu ứng chia bài, không lặp lại chữ FIGHT.

Ảnh gốc do **ImageGen tích hợp** tạo, giữ nguyên alpha, kích thước 2172×724: `assets/pastel/ui/fight.png`. Prompt đầy đủ và hash nằm trong mục `fight_banner` của `assets/provenance.json`. Không sửa hoặc thay các sprite cũ.

## Xếp ba lá rồi Attack

- Mỗi phe có đúng **3 Turn Star** mỗi vòng. Mỗi lá chiếm **1 sao**, thay giá năng lượng cũ.
- Bấm lá trên tay để đặt vào hàng chờ phía dưới. Chọn bài chưa gây sát thương, chưa buff/debuff và chưa tiêu hao lá.
- Bấm một lá trong hàng chờ để gỡ; các lá còn lại giữ thứ tự. Mỗi lá nhớ mục tiêu tại thời điểm chọn; nếu mục tiêu đã gục khi thực hiện thì chuyển sang địch còn sống.
- Đủ ba vị trí thì nút **Attack** sáng lên. Phím `E` cũng thực hiện Attack. Không thể sửa hàng chờ, dùng thuốc, dùng tuyệt kỹ hoặc lưu/tải giữa chuỗi hành động.
- Thứ tự cố định: **ta 1 → địch 1 → ta 2 → địch 2 → ta 3 → địch 3**. Lá hiện lớn ở giữa sân, sau đó áp hiệu ứng tại nhịp chạm. Nếu một phe đã bị hạ thì hủy phần chuỗi còn lại.
- Nút **Chờ 1 sao** dành cho trường hợp thiếu bài, dính lời nguyền hoặc bị khóa; không tạo thêm bài hay hiệu ứng. Cần lấp đủ ba vị trí trước khi Attack.
- Thuốc và tuyệt kỹ vẫn dùng riêng trong giai đoạn chuẩn bị. Bài rút bởi hiệu ứng trong chuỗi được giữ cho lần chọn tiếp theo.

## Hàng bài địch

Địch chốt đủ ba lá trước khi người chơi chọn. Các lá nằm phía trên địch; rê chuột lên từng lá để xem tên, sát thương, số hit, giáp và trạng thái. Chọn/gỡ bài, đổi mục tiêu hoặc rê chuột không làm địch đổi kế hoạch.

Phe địch dùng chung ba sao kể cả có viện binh. Khi có nhiều địch, ba vị trí được chia cho các đơn vị còn sống; đơn vị đã gục không thực hiện lá của nó. Chuyển pha không thay bí mật những lá đã công bố; hành vi pha mới bắt đầu từ kế hoạch vòng sau. Khóa bài mới của Ếch cũng chờ vòng chuẩn bị sau, không khóa một lá đã chốt.

Đòn thường của từng lá dùng 35% sát thương gốc, làm tròn lên, trước khi cộng Sức Mạnh và hệ số trạng thái; số tầng trạng thái thường được giảm một nửa, làm tròn lên. Đây là điều chỉnh cho việc địch có ba hành động. Đòn tích tụ giữ sát thương gốc, chỉ đếm một lần mỗi vòng, hai lá phụ là dựng giáp. Hổ vẫn có một vòng báo trước rồi mới vồ.

## Đổi các hiệu ứng năng lượng

Hiệu ứng nhận năng lượng trên bài/thuốc trở thành rút cùng số lá; nhận năng lượng vòng sau trở thành rút thêm bài vòng sau. Mỗi mức tăng năng lượng nền từ di vật/kỹ năng thành ba lựa chọn bài thêm đầu vòng; mức giảm cũng giảm lựa chọn tương ứng. Tay tối đa mười lá. Pháp Sư vẫn rút sáu lá; nâng giới hạn Mana cũ cho thêm lựa chọn bài, không tạo thanh năng lượng riêng.

Các nâng cấp trước đây chỉ giảm giá năng lượng được thay bằng **thêm 3 giáp khi dùng**, để dấu `+` vẫn có tác dụng trong hệ giá một sao. Tooltip và hiệu ứng thực dùng cùng định nghĩa; Ảnh Phân và Sao Chép cũng nhận đúng hiệu ứng này.

## Lưu và tương thích

Hàng bài ta, mục tiêu mỗi lá, kế hoạch địch và RNG được lưu trong session v2. Có thể lưu khi mới chọn một hoặc hai lá, rồi tiếp tục đúng hàng đó. Bản v2 cũ chưa có hàng chờ vẫn đọc được, giữ HP/bộ bài/tiến trình và tạo kế hoạch theo luật mới. Không lưu ở giữa di chuyển, chuyển cảnh, chia bài hay thực hiện hành động.

`tests/star_checks.cpp` kiểm tra thứ tự, không gây hiệu ứng khi chọn, gỡ/đổi bài, chờ khi thiếu bài, tích tụ, chuyển pha, trận liên tiếp, lưu/tải và kế hoạch địch. Chế độ `--star-smoke` chạy renderer DxLib thực và ghi các ảnh vào `artifacts/turn-stars`; `--smoke` giữ phép đối chiếu nền cũ. Mô phỏng luật mới dùng `CoreChecks.exe --simulate-stars N`.
