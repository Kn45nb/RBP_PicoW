# OledHave2Scren · PicoDualOLED V1.3

PicoDualOLED V1.3 là dự án firmware C11 cho Raspberry Pi Pico W (RP2040), dùng Pico SDK **2.2.0**. Mục tiêu là điều khiển hai màn SSD1306 độc lập, hiển thị đồ họa 1 bit và phát hoạt ảnh có lớp chữ, biểu tượng và giao diện phủ lên trên.

## Hiện trạng

Tệp [`OledHave2Scren.c`](OledHave2Scren.c) chứa chương trình mẫu Pico SDK để thử SPI, I²C, DMA sao chép RAM, PIO và UART. Driver SSD1306, Display Engine và DMA truyền qua I²C đang chờ triển khai. CMake và cấu hình VS Code hiện trỏ tới SDK 2.3.1. Build theo SDK 2.2.0, FPS thực tế và mức sử dụng heap vẫn cần xác minh.

Xem [`PROJECT_TREE.map`](PROJECT_TREE.map) để biết cấu trúc hiện tại và vị trí dự kiến của từng mô-đun.

## Kết nối OLED theo đặc tả

| Màn hình | Bus | SDA | SCL | Địa chỉ I²C 7 bit |
| --- | --- | --- | --- | --- |
| OLED 1 | I2C0 | GP4 | GP5 | `0x3C` |
| OLED 2 | I2C1 | GP6 | GP7 | `0x3C` |

Hai màn đều có độ phân giải 128 × 64, dùng nguồn 3V3 và GND. Mỗi màn dùng một bus riêng ở tần số mục tiêu 400 kHz. Pinout, module và hệ thống tệp của thẻ SD sẽ được xác nhận trước khi tích hợp.

## Kiến trúc firmware

- Core 0 chạy bộ lập lịch, phát hoạt ảnh và ghép nền, chữ, giao diện vào backbuffer của mỗi màn. Core 1 ở trạng thái nghỉ.
- Mỗi màn dùng hai framebuffer kích thước cố định. Hai kênh DMA TX phục vụ hai bus độc lập. Firmware theo dõi lỗi I²C và xác nhận giao dịch kết thúc trên bus trước khi báo hoàn tất.
- GIF được chuyển đổi trên máy tính thành các frame bitmap 1 bit. Firmware phát từ RAM trước, sau đó có thể bổ sung bộ đệm nạp frame từ SD.
- Tổng ngân sách sơ bộ là 40 KiB cho hai cặp framebuffer, hai vùng đệm lệnh DMA và cache hoạt ảnh đề xuất. Chính sách bộ nhớ yêu cầu cấp phát tĩnh trong mã ứng dụng và kiểm toán RAM, stack, heap qua ELF, linker map và đo lúc chạy.

## Lộ trình

1. Kiểm tra repository và xác lập bản build cơ sở với SDK 2.2.0.
2. Khởi tạo hai OLED và hiển thị ảnh tĩnh.
3. Xây dựng framebuffer, đồ họa, bộ render chữ và ghép lớp.
4. Tích hợp DMA bất đồng bộ và xử lý lỗi cho hai bus I²C.
5. Phát hoạt ảnh từ RAM, phủ chữ và lập lịch theo deadline.
6. Tích hợp SD sau khi chốt phần cứng và hệ thống tệp.
7. Kiểm toán ELF, MAP, UF2, stack và các vùng SRAM.
8. Đo hiệu năng trên phần cứng với mục tiêu 30 FPS.
