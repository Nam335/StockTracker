# StockTracker

Ứng dụng theo dõi giá chứng khoán thời gian thực, xây dựng bằng C++ và Qt Widgets. Client và Server giao tiếp qua TCP Socket, dữ liệu được lưu bằng SQLite.

## 1. Chức năng

### Client
- Kết nối Server bằng địa chỉ IP và cổng TCP.
- Đăng ký, đăng nhập tài khoản.
- Nhận và hiển thị bảng giá chứng khoán theo thời gian thực.
- Xem biểu đồ lịch sử giá theo các khoảng 1 giờ (1h), 24 giờ (24h), 7 ngày (7d).
- Ghim mã cổ phiếu vào Favorites, xem danh sách và bỏ ghim.

### Admin
- Quản lý tài khoản người dùng.
- Xem, thêm, xóa và sắp xếp danh mục cổ phiếu.
- Sử dụng các chức năng theo dõi giá và Favorites.

### Server
- Tiếp nhận nhiều kết nối TCP Client.
- Gửi dữ liệu giá mô phỏng.
- Xử lý đăng ký, đăng nhập và phân quyền.
- Quản lý danh mục cổ phiếu, Favorites và lịch sử giá.
- Lưu dữ liệu bằng SQLite.

## 2. Công nghệ

- C++ và Qt 6 Widgets.
- Qt Network, Qt Charts, Qt SQL.
- SQLite.
- CMake và Visual Studio 2022 / MSVC 2022.

## 3. Cấu trúc dự án

- `Code/Client/StockClient`: mã nguồn Client.
- `Code/Sever/StockServer`: mã nguồn Server.
- `Docs/TestCase.xlsx`: tài liệu kiểm thử.

Lưu ý: tên thư mục Server trong repository là `Sever`.

## 4. Yêu cầu môi trường

Môi trường tham khảo:
- Windows 10/11 64-bit.
- Qt 6.12.0 MSVC 2022 64-bit.
- Visual Studio 2022, workload Desktop development with C++.
- CMake 3.19 trở lên.

Các module Qt:
- Client: Core, Widgets, Network, Charts.
- Server: Core, Network, Sql.

## 5. Build bằng CMake

Điều chỉnh đường dẫn `$qt` nếu Qt được cài ở vị trí khác.

```powershell
$repo = "C:\Users\Admin\Documents\GitHub\StockTracker"
$qt = "C:\Qt\6.12.0\msvc2022_64"
$cmake = "C:\Qt\Tools\CMake_64\bin\cmake.exe"

# Build Server
$server = "$repo\Code\Sever\StockServer"
& $cmake -S $server -B "$server\build" `
    -G "Visual Studio 17 2022" -A x64 `
    "-DCMAKE_PREFIX_PATH=$qt"
& $cmake --build "$server\build" --config Debug --parallel 4

# Build Client
$client = "$repo\Code\Client\StockClient"
& $cmake -S $client -B "$client\build" `
    -G "Visual Studio 17 2022" -A x64 `
    "-DCMAKE_PREFIX_PATH=$qt"
& $cmake --build "$client\build" --config Debug --parallel 4
```

## 6. Cách chạy ứng dụng

Mở PowerShell tại thư mục repository:

```powershell
$repo = "C:\Users\Admin\Documents\GitHub\StockTracker"
$env:PATH = "C:\Qt\6.12.0\msvc2022_64\bin;$env:PATH"
```

**Bước 1 – Chạy Server**

```powershell
& "$repo\Code\Sever\StockServer\build\Debug\StockServer.exe"
```

Giữ cửa sổ Server mở trong suốt quá trình sử dụng Client.

**Bước 2 – Chạy Client**

Mở một cửa sổ PowerShell khác:

```powershell
$repo = "C:\Users\Admin\Documents\GitHub\StockTracker"
$env:PATH = "C:\Qt\6.12.0\msvc2022_64\bin;$env:PATH"
& "$repo\Code\Client\StockClient\build\Debug\StockClient.exe"
```

**Bước 3 – Kết nối**

- Nhập địa chỉ IP của máy chạy Server.
- Cổng mặc định: `5000`.
- Nếu Client và Server chạy cùng máy, có thể dùng `127.0.0.1`.
- Nhấn nút kết nối để bắt đầu nhận dữ liệu.

## 7. Hướng dẫn sử dụng

### Người dùng
1. Đăng ký tài khoản hoặc đăng nhập.
2. Theo dõi giá cổ phiếu trên bảng giá.
3. Chọn mã cổ phiếu để xem biểu đồ lịch sử.
4. Chọn khoảng thời gian 1h, 24h hoặc 7d.
5. Chọn mã cổ phiếu và nhấn thêm vào Favorites để ghim.
6. Chọn mã trong danh sách Favorites để bỏ ghim khi không cần nữa.

### Quản trị viên
1. Đăng nhập bằng tài khoản có quyền Admin.
2. Sử dụng khu vực quản lý tài khoản để xem và quản lý người dùng.
3. Sử dụng khu vực quản lý danh mục cổ phiếu để thêm, xóa hoặc sắp xếp mã cổ phiếu.
4. Theo dõi bảng giá, biểu đồ và Favorites như người dùng thông thường.

## 8. Dữ liệu

Server sử dụng SQLite để lưu thông tin tài khoản, danh mục cổ phiếu, Favorites và lịch sử giá. Cơ sở dữ liệu được tạo hoặc sử dụng trong thư mục chạy Server.

Không nên xóa hoặc chỉnh sửa file cơ sở dữ liệu khi Server đang hoạt động.

## 9. Kiểm thử

Các trường hợp kiểm thử và kết quả được ghi trong `Docs/TestCase.xlsx`.

Nên chạy Server trước Client và kiểm tra các tình huống kết nối, ngắt kết nối, đăng nhập, cập nhật giá, biểu đồ, Favorites và chức năng quản trị.

## 10. Xử lý lỗi thường gặp

- **Không kết nối được Server:** kiểm tra Server đã chạy chưa, địa chỉ IP và cổng `5000`.
- **Port 5000 đang được sử dụng:** kiểm tra có tiến trình StockServer khác đang chạy hay không.
- **Không tìm thấy Qt DLL:** bổ sung thư mục `C:\Qt\6.12.0\msvc2022_64\bin` vào `PATH`.
- **Database bị khóa:** đóng các tiến trình Server đang chạy trùng và khởi động lại một tiến trình Server.

## 11. Ghi chú

Đây là dự án học tập. Dữ liệu giá cổ phiếu được mô phỏng, không phải dữ liệu giao dịch chứng khoán thực tế.

Các thư mục build và cấu hình Qt Creator cục bộ không cần đưa lên Git.
