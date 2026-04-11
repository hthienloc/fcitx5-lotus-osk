# Lotus OSK (On-Screen Keyboard)

Bàn phím ảo (OSK) độc lập cho bộ gõ Lotus (`fcitx5-lotus`).

Dự án này tách biệt phần giao diện bàn phím (Qt/LayerShell) ra khỏi bộ gõ chính để dễ dàng quản lý và nâng cấp, đồng thời giữ nguyên khả năng tương thích hoàn hảo với phần xử lý phím (backend) bên trong `fcitx5-lotus`.

## Tính năng nổi bật

- **Full Functional Keys**: Hỗ trợ đầy đủ phím `Esc`, `Delete`, `Ctrl`, `Alt`, `Shift`, `Super` và các phím mũi tên.
- **One-shot Modifiers**: Hỗ trợ đặc biệt cho màn hình cảm ứng - nhấn `Ctrl` hoặc `Alt` để kích hoạt trạng thái "chờ", sau đó nhấn phím tiếp theo để gửi tổ hợp phím (ví dụ: `Ctrl+C`, `Alt+F4`).
- **KDE Integration & Tray Icon**: Cung cấp biểu tượng khay hệ thống giúp người dùng dễ dàng bật/tắt bàn phím, đặc biệt hữu ích trên môi trường KDE.
- **D-Bus Activated**: Tự động hiển thị khi backend yêu cầu (ví dụ: khi focus vào ô nhập liệu).
- **Giao diện hiện đại**: Hỗ trợ giao diện Sáng/Tối (Light/Dark mode) và điều chỉnh kích thước linh hoạt.

## Yêu cầu hệ thống

- **Qt6**: Core, Gui, Widgets, DBus.
- **LayerShellQt**: (Không bắt buộc) Khuyên dùng trên môi trường Plasma Wayland để có trải nghiệm overlay tốt nhất.
- **Fcitx5-Lotus**: Cần cài đặt bản backend có hỗ trợ OSK (nhánh `osk-support`).

## Hướng dẫn cài đặt

Lotus OSK hoạt động như một module mở rộng. Để sử dụng, hệ thống của bạn cần có `fcitx5-lotus` (phiên bản có hỗ trợ OSK).

### 1. Kiểm tra Backend (`fcitx5-lotus`)

Nếu bạn đang sử dụng phiên bản `fcitx5-lotus` chính thức (đã được merge PR hỗ trợ OSK), bạn có thể bỏ qua bước này.

Nếu chưa, bạn cần biên dịch backend từ nhánh `osk-support`:

```bash
# Chỉ thực hiện nếu bản fcitx5-lotus hiện tại chưa hỗ trợ OSK
git clone https://github.com/LotusInputMethod/fcitx5-lotus.git
cd fcitx5-lotus
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
sudo cmake --install build
```

Khởi động lại Fcitx5 sau khi cài đặt.

### 2. Cài đặt Lotus OSK (Frontend)

```bash
git clone https://github.com/LotusInputMethod/fcitx5-lotus-osk.git
cd fcitx5-lotus-osk
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build -j$(nproc)
sudo cmake --install build
```

## Cách sử dụng

1. **Khởi chạy**: Bạn có thể tìm thấy "Lotus OSK" trong menu ứng dụng hoặc chạy lệnh `fcitx5-lotus-osk` trong terminal.
2. **Biểu tượng khay hệ thống**: Sau khi chạy, một biểu tượng bàn phím sẽ hiện ở khay hệ thống.
    - **Click chuột trái**: Để hiện/ẩn nhanh bàn phím.
    - **Click chuột phải**: Để mở menu cài đặt nhanh hoặc thoát ứng dụng.
3. **Tự động hóa**: Bộ gõ Lotus sẽ tự động điều khiển việc hiển thị bàn phím này khi bạn tương tác với các ô nhập liệu (nếu tính năng OSK được bật trong cấu hình Fcitx5).

## Thông tin kỹ thuật

- **D-Bus Service**: `app.lotus.Osk`
- **Object Path**: `/app/lotus/Osk/Controller`
- **Giao thức Socket**: Sử dụng `LotusKeyCommand` qua Unix Domain Socket để giao tiếp với `lotus-server` phục vụ việc mô phỏng phím phần cứng.
