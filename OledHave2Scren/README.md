# OledHave2Scren · PicoDualOLED V1.3

PicoDualOLED V1.3 là dự án firmware C11 cho Raspberry Pi Pico W (RP2040). Đặc tả yêu cầu Pico SDK **2.2.0** và đặt mục tiêu điều khiển hai màn SSD1306 độc lập, hiển thị đồ họa 1 bit và phát hoạt ảnh có lớp chữ, biểu tượng và giao diện phủ lên trên. Chương trình mẫu hiện cấu hình với Pico SDK **2.3.1**.

## Hiện trạng

`main()` trong [`OledHave2Scren.c`](OledHave2Scren.c) chạy trên **Core 0 vật lý** và gọi [`core0_entry()`](core0.c). Mặc định, Core 0 gọi [`app_prepare_hardware()`](src/app_hardware.c) để khởi tạo hai bus I²C, UART, SPI và kiểm tra DMA sao chép từ flash sang SRAM. Core 0 cũng báo lỗi bằng LED và chạy kiểm thử OLED nếu được bật. Cấu hình chân, tốc độ và mã lỗi nằm trong [`include/app_config.h`](include/app_config.h).

`PICO_PIO_TASKS` mặc định là `ON`. Core 0 đọc mã từ PIO và khởi chạy [`core1_entry()`](core1.c) trên **Core 1 vật lý** khi có yêu cầu đầu tiên. Core 1 nhận mã qua FIFO, gọi [`app_run_task()`](src/app_tasks.c) để chọn tệp `taskN.c` tương ứng rồi gửi kết quả về Core 0. Có thể tắt chế độ này bằng `-DPICO_PIO_TASKS=OFF`.

`PICO_INIT_ON_CORE1` mặc định là `OFF`. Nếu bật tùy chọn này, Core 1 được khởi chạy sớm để chạy `app_prepare_hardware()` rồi chờ tác vụ. Chế độ `PICO_OLED_UI_TEST` vẫn chạy một lượt thử trên Core 0 lúc khởi động, trước khi Core 0 gửi tác vụ PIO, để hai nhân dùng I²C0 theo lượt.

USB stdio và UART stdio đều tắt. Chế độ kiểm thử riêng đã có đường truyền DMA tới OLED 1. Display Engine và tích hợp thẻ SD đang chờ triển khai.

[`blink.pio`](blink.pio) là ví dụ PIO nháy LED. Target hiện dùng [`task_input.pio`](task_input.pio) cho tín hiệu chọn tác vụ. Build cũng chạy `pioasm` cho chương trình PIO của driver CYW43 trong Pico SDK.

[`PROJECT_TREE.map`](PROJECT_TREE.map) ghi cấu trúc hiện tại, đầu ra build và các mô-đun dự kiến. Build trong `build/` hiện có ELF, linker map và UF2. Việc nạp firmware và thử mã nháy trên Pico W vẫn cần xác minh.

## Phụ thuộc ngoài workspace

Workspace của repository là `D:\GIT\RBP_PicoW`. Trên Windows, `%USERPROFILE%` là thư mục người dùng. Các mục `[>]` trong cây tham chiếu những phụ thuộc chính sau đây.

| Nơi tham chiếu | Đường dẫn hoặc công cụ bên ngoài | Mục đích |
| --- | --- | --- |
| `CMakeLists.txt` | `%USERPROFILE%\.pico-sdk\cmake\pico-vscode.cmake` | Nạp cấu hình SDK và toolchain của Pico extension |
| `pico_sdk_import.cmake`, mã C, IntelliSense và debug | `%USERPROFILE%\.pico-sdk\sdk\2.3.1\` | `pico_sdk_init.cmake`, header, thư viện, driver, linker script và tệp SVD của RP2040 |
| CMake, `build.ninja`, cấu hình VS Code | `%USERPROFILE%\.pico-sdk\toolchain\15_2_Rel1\bin\` | ARM GCC/G++, `objdump`, `objcopy` và GDB của toolchain |
| CMake và VS Code | `%USERPROFILE%\.pico-sdk\cmake\v4.5.0\bin\cmake.exe` và `%USERPROFILE%\.pico-sdk\ninja\v1.13.2\ninja.exe` | Configure và build |
| `build.ninja` | `%USERPROFILE%\.pico-sdk\tools\2.3.1\pioasm\pioasm.exe` | Biên dịch chương trình PIO chọn tác vụ và chương trình PIO của driver CYW43 |
| `build.ninja`, tác vụ `Run Project` | `%USERPROFILE%\.pico-sdk\picotool\2.3.1\picotool\picotool.exe` | Tạo UF2 và nạp firmware |
| CMake, `cmake-kits.json` | Python 3 từ Pico extension hoặc do CMake tìm thấy. Cache hiện ghi `%LOCALAPPDATA%\Programs\Python\Python313\python.exe`. | Chạy công cụ build của SDK |
| `tasks.json`, `launch.json` | `openocd` do môi trường hoặc extension cung cấp và script ở `%USERPROFILE%\.pico-sdk\openocd\0.12.0+dev\scripts\` | Nạp và debug qua SWD |
| `.vscode/` | Extension Raspberry Pi Pico, C/C++ và Cortex-Debug cài trong VS Code. Cấu hình còn đề xuất Serial Monitor. | Tác vụ IDE, IntelliSense và debug |

Các lệnh do Ninja sinh trên Windows cũng gọi `cmd.exe` của hệ điều hành. Nhánh tải SDK từ Git trong `pico_sdk_import.cmake` chỉ chạy khi bật `PICO_SDK_FETCH_FROM_GIT`. Cache hiện dùng SDK cài sẵn. Các mục `[~]` trong cây là kế hoạch, chưa có phụ thuộc build tương ứng.

## Mã nháy lỗi

Đếm số nháy ngắn liên tiếp trong một cụm. Các cụm lặp lại sau khoảng nghỉ 1,2 giây. Khi kiểm tra thành công, LED tắt.

| Số nháy | Trạng thái | LED |
| --- | --- | --- |
| 1 | Khởi tạo CYW43 hoặc lần ghi đầu tiên vào LED tích hợp thất bại | LED rời trên GP22, qua điện trở xuống GND |
| 2 | Hết kênh DMA trống | LED tích hợp |
| 3 | DMA sao chép quá thời hạn | LED tích hợp |
| 4 | Dữ liệu DMA sao chép sai | LED tích hợp |
| 5 | Core 1 khởi chạy hoặc phản hồi quá hạn, hoặc trả mã không hợp lệ | LED tích hợp |
| 6 | Kiểm thử OLED 1 thất bại | LED tích hợp |
| 7 | Không khởi tạo được state machine PIO đọc tác vụ | LED tích hợp |

Watchdog khởi động lại Pico W nếu bước khởi chạy Core 1 bị kẹt. Firmware báo 5 nháy sau khi khởi động lại. Với lỗi do một tác vụ trả về, Core 0 nháy một cụm rồi tiếp tục nhận lệnh. Nếu LED tích hợp lỗi, nhóm nháy chuyển sang LED rời.

Nếu đã mắc LED dự phòng vào GP6 theo sơ đồ cũ, chuyển LED sang GP22 trước khi dùng I²C1.

## Cổng ngoại vi đã khởi tạo

| Cổng | Chân GPIO | Cấu hình |
| --- | --- | --- |
| I²C0 | GP4 SDA, GP5 SCL | 400 kHz |
| I²C1 | GP6 SDA, GP7 SCL | 400 kHz |
| UART1 | GP8 TX, GP9 RX | 115200 baud, 8N1, không điều khiển luồng |
| SPI0 | GP16 MISO, GP17 CS, GP18 SCK, GP19 MOSI | 1 MHz, CS điều khiển bằng GPIO và ở mức cao khi rảnh |

Các cổng mới chỉ được khởi tạo. UART1 dành cho trao đổi dữ liệu và firmware chưa gửi ký tự khởi động. Kiểm tra điện trở kéo lên của từng bus I²C trước khi chạy ở 400 kHz.

## Chọn tác vụ bằng PIO

Nối chung GND và dùng tín hiệu logic 3,3 V. **GP10** là bit thấp `D0`, **GP11** là bit cao `D1`, **GP12** là `STROBE`. Giữ `STROBE` thấp khi rảnh, đặt ổn định `D1:D0` rồi đưa `STROBE` lên cao để chốt mã. Hạ `STROBE` trước khi gửi lệnh tiếp theo. Firmware chạy mỗi lần một tác vụ. Hãy gửi từng xung và đợi tác vụ trước xử lý xong; FIFO PIO có dung lượng hữu hạn.

| D1:D0 | Hàm trên Core 1 | Tác vụ |
| --- | --- | --- |
| `00` | [`task0_run()`](src/task0.c) | Kiểm tra DMA sao chép vào SRAM |
| `01` | [`task1_run()`](src/task1.c) | Hiện bàn cờ, đợi 1,2 giây rồi hiện sọc |
| `10` | [`task2_run()`](src/task2.c) | Hiện riêng ảnh bàn cờ |
| `11` | [`task3_run()`](src/task3.c) | Hiện riêng ảnh sọc lệch pha |

Ba tác vụ hiển thị dùng OLED 1 trên I²C0. Muốn bổ sung hoặc đổi tác vụ, sửa bảng mã tại [`app_tasks.h`](include/app_tasks.h) và bộ phân lệnh trong [`app_tasks.c`](src/app_tasks.c). Bộ chọn hai bit hiện có bốn mã.

## Kết nối OLED theo đặc tả

| Màn hình | Bus | SDA | SCL | Địa chỉ I²C 7 bit |
| --- | --- | --- | --- | --- |
| OLED 1 | I2C0 | GP4 | GP5 | `0x3C` |
| OLED 2 | I2C1 | GP6 | GP7 | `0x3C` |

Hai màn đều có độ phân giải 128 × 64, dùng nguồn 3V3 và GND. Mỗi màn dùng một bus riêng ở tần số mục tiêu 400 kHz. Pinout, module và hệ thống tệp của thẻ SD sẽ được xác nhận trước khi tích hợp.

## UX/UI kiểm thử ảnh qua DMA

Thêm `-DPICO_OLED_UI_TEST=ON` khi configure CMake để bật kiểm thử. Tùy chọn mặc định là `OFF`. Có thể dùng một thư mục build riêng cho chế độ này.

Cắm OLED SSD1306 128 × 64, địa chỉ `0x3C`, vào I²C0 trên GP4/GP5. Sau khi khởi động, OLED 1 hiện bàn cờ trong khoảng 1,2 giây rồi giữ mẫu sọc lệch pha. Hai mẫu được tạo trong framebuffer SRAM 1.024 byte. DMA đưa dữ liệu vào FIFO TX của I²C0. Test kiểm tra DMA hoàn tất, STOP trên bus và lỗi I²C. Nếu lỗi, LED phát 6 nháy mỗi cụm.

Task 1 điều phối chuỗi ảnh. Task 2 và Task 3 giữ hàm dựng mẫu tương ứng. Bộ phân lệnh sở hữu framebuffer dùng chung. [`ssd1306.c`](src/ssd1306.c) nhận framebuffer và chuẩn bị lệnh cho màn hình, với một `ssd1306_t` giữ bus, địa chỉ và bộ đệm lệnh DMA. [`display_dma.c`](src/display_dma.c) thực hiện giao dịch I²C và xử lý timeout. Phần OLED được biên dịch khi bật `PICO_OLED_UI_TEST` hoặc `PICO_PIO_TASKS`.

Ảnh kiểm thử hiện được tạo trên Pico W. Để bổ sung bước đọc thẻ vào cùng framebuffer, cần xác nhận module, pinout, hệ thống tệp và định dạng ảnh trên thẻ.

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
