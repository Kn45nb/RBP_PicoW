# OledHave2Scren · PicoDualOLED V1.3

PicoDualOLED V1.3 là dự án firmware C11 cho Raspberry Pi Pico W (RP2040). Đặc tả yêu cầu Pico SDK **2.2.0** và đặt mục tiêu điều khiển hai màn SSD1306 độc lập, hiển thị đồ họa 1 bit và phát hoạt ảnh có lớp chữ, biểu tượng và giao diện phủ lên trên. Chương trình mẫu hiện cấu hình với Pico SDK **2.3.1**.

## Hiện trạng

Tệp [`OledHave2Scren.c`](OledHave2Scren.c) dùng Core 1 để khởi tạo SPI0 (GP16–GP19), I²C0 (SDA GP8, SCL GP9) và kiểm tra DMA sao chép dữ liệu từ flash sang SRAM. Core 0 báo lỗi bằng LED. USB stdio và UART stdio đều tắt. Driver SSD1306, Display Engine và DMA truyền qua I²C đang chờ triển khai.

[`blink.pio`](blink.pio) là ví dụ PIO nháy LED có sẵn trong Git. Target hiện chưa sinh `blink.pio.h` từ tệp này. Build có chạy `pioasm` cho chương trình PIO của driver CYW43 trong Pico SDK. GP10 và GP11 chưa được mã mẫu cấu hình. Có thể dùng GP10 để thử PIO với LED rời, còn GP6 dành cho báo lỗi dự phòng trong chương trình mẫu.

[`PROJECT_TREE.map`](PROJECT_TREE.map) ghi cấu trúc hiện tại, đầu ra build và các mô-đun dự kiến. Build trong `build/` hiện có ELF, linker map và UF2. Việc nạp firmware và thử mã nháy trên Pico W vẫn cần xác minh.

## Phụ thuộc ngoài workspace

Workspace của repository là `D:\GIT\RBP_PicoW`. Trên Windows, `%USERPROFILE%` là thư mục người dùng. Các mục `[>]` trong cây tham chiếu những phụ thuộc chính sau đây.

| Nơi tham chiếu | Đường dẫn hoặc công cụ bên ngoài | Mục đích |
| --- | --- | --- |
| `CMakeLists.txt` | `%USERPROFILE%\.pico-sdk\cmake\pico-vscode.cmake` | Nạp cấu hình SDK và toolchain của Pico extension |
| `pico_sdk_import.cmake`, mã C, IntelliSense và debug | `%USERPROFILE%\.pico-sdk\sdk\2.3.1\` | `pico_sdk_init.cmake`, header, thư viện, driver, linker script và tệp SVD của RP2040 |
| CMake, `build.ninja`, cấu hình VS Code | `%USERPROFILE%\.pico-sdk\toolchain\15_2_Rel1\bin\` | ARM GCC/G++, `objdump`, `objcopy` và GDB của toolchain |
| CMake và VS Code | `%USERPROFILE%\.pico-sdk\cmake\v4.5.0\bin\cmake.exe` và `%USERPROFILE%\.pico-sdk\ninja\v1.13.2\ninja.exe` | Configure và build |
| `build.ninja` | `%USERPROFILE%\.pico-sdk\tools\2.3.1\pioasm\pioasm.exe` | Biên dịch chương trình PIO của driver CYW43 |
| `build.ninja`, tác vụ `Run Project` | `%USERPROFILE%\.pico-sdk\picotool\2.3.1\picotool\picotool.exe` | Tạo UF2 và nạp firmware |
| CMake, `cmake-kits.json` | Python 3 từ Pico extension hoặc do CMake tìm thấy. Cache hiện ghi `%LOCALAPPDATA%\Programs\Python\Python313\python.exe`. | Chạy công cụ build của SDK |
| `tasks.json`, `launch.json` | `openocd` do môi trường hoặc extension cung cấp và script ở `%USERPROFILE%\.pico-sdk\openocd\0.12.0+dev\scripts\` | Nạp và debug qua SWD |
| `.vscode/` | Extension Raspberry Pi Pico, C/C++ và Cortex-Debug cài trong VS Code. Cấu hình còn đề xuất Serial Monitor. | Tác vụ IDE, IntelliSense và debug |

Các lệnh do Ninja sinh trên Windows cũng gọi `cmd.exe` của hệ điều hành. Nhánh tải SDK từ Git trong `pico_sdk_import.cmake` chỉ chạy khi bật `PICO_SDK_FETCH_FROM_GIT`. Cache hiện dùng SDK cài sẵn. Các mục `[~]` trong cây là kế hoạch, chưa có phụ thuộc build tương ứng.

## Mã nháy lỗi

Đếm số nháy ngắn liên tiếp trong một cụm. Các cụm lặp lại sau khoảng nghỉ 1,2 giây. Khi kiểm tra thành công, LED tắt.

| Số nháy | Trạng thái | LED |
| --- | --- | --- |
| 1 | Khởi tạo CYW43 hoặc lần ghi đầu tiên vào LED tích hợp thất bại | LED rời trên GP6, qua điện trở xuống GND |
| 2 | Hết kênh DMA trống | LED tích hợp |
| 3 | DMA sao chép quá thời hạn | LED tích hợp |
| 4 | Dữ liệu DMA sao chép sai | LED tích hợp |
| 5 | Core 1 khởi chạy quá hạn hoặc thiếu kết quả hợp lệ | LED tích hợp |

Watchdog khởi động lại Pico W nếu bước khởi chạy Core 1 bị kẹt. Firmware báo 5 nháy sau khi khởi động lại. Nếu LED tích hợp lỗi trong lúc báo mã 2–5, nhóm nháy chuyển sang LED rời.

GP6 là chân báo lỗi dự phòng của chương trình mẫu. Đặc tả OLED bên dưới dành GP6 cho I²C1 ở giai đoạn tích hợp màn hình.

## Kết nối OLED theo đặc tả

| Màn hình | Bus | SDA | SCL | Địa chỉ I²C 7 bit |
| --- | --- | --- | --- | --- |
| OLED 1 | I2C0 | GP4 | GP5 | `0x3C` |
| OLED 2 | I2C1 | GP6 | GP7 | `0x3C` |

Hai màn đều có độ phân giải 128 × 64, dùng nguồn 3V3 và GND. Mỗi màn dùng một bus riêng ở tần số mục tiêu 400 kHz. Pinout, module và hệ thống tệp của thẻ SD sẽ được xác nhận trước khi tích hợp.

## Kiến trúc Display Engine dự kiến theo đặc tả V1.3

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
