# RBP_PicoW

Dự án firmware cho Raspberry Pi Pico W (RP2040), sử dụng C/C++ và Pico SDK. Tệp chương trình chính là `main.c`. Firmware đầu ra là `build-offline\main.uf2`.

## Sử dụng hằng ngày

Mở PowerShell tại thư mục chứa `main.c`, lưu các thay đổi rồi chọn một lệnh:

| Mục đích | Lệnh |
|---|---|
| Kiểm tra biên dịch và liên kết | `.\build-offline.cmd check` |
| Build và xuất `build-offline\main.uf2` | `.\build-offline.cmd` |

Chế độ `check` tạo `main.elf`. Nếu đã có `main.uf2` từ lần trước, tệp đó vẫn chứa firmware cũ cho đến khi bạn chạy lệnh xuất UF2 thành công.

## 1. Chuẩn bị dự án trên máy Windows mới

Bộ công cụ hiện tại dành cho **Windows x64 (Intel hoặc AMD)**. Bạn có thể đặt repo trên bất kỳ ổ đĩa nào. Script lấy thư mục chứa chính nó làm gốc và thiết lập `PATH` riêng cho mỗi lần chạy.

Trên máy mới, chuẩn bị cấu trúc sau. Với CMake và Arm GNU Toolchain, hãy chép **toàn bộ thư mục công cụ** để có đủ thư viện đi kèm.

```text
RBP_PicoW/
├── main.c
├── CMakeLists.txt
├── build-offline.cmd
├── lib/                              # Nếu dự án có thư viện riêng
├── pico-sdk-master/
│   └── pico_sdk_init.cmake
└── .tool/
    ├── cmake 4.5.1/bin/cmake.exe
    ├── ninja v1.13.2/ninja.exe
    ├── arm-gnu 15.3.rel2/bin/arm-none-eabi-gcc.exe
    └── picotool/picotool.exe
```

- Nếu lấy dự án bằng `git clone`, hãy chép thêm từ máy cũ những công cụ hoặc thư viện bị bỏ qua bởi `.gitignore`.
- Chuẩn bị **Python 3** trong `PATH` hoặc tại `.tool\python\python.exe`. Nếu dùng bản cài trên Windows, kiểm tra bằng `python --version`.
- Để script tạo `build-offline\` mới trên máy mới. Cache CMake trong các thư mục build cũ có chứa đường dẫn tuyệt đối của máy trước.
- Sau khi đã có SDK và đủ công cụ trên máy, bạn có thể ngắt mạng và thực hiện cả lần build đầu tiên.

### Nếu thiếu `picotool.exe`

Script dùng `picotool.exe` để chuyển `main.elf` sang `main.uf2`. Nếu đã từng build thành công trong `build-manual`, chạy đoạn sau **trên máy cũ**, từ gốc repo, để lấy lại bản đã có:

```powershell
$p = Get-ChildItem '.\build-manual\_deps' -Recurse -File -Filter picotool.exe |
    Select-Object -First 1

if (-not $p) { throw 'Không tìm thấy picotool.exe trong thư mục build cũ.' }

New-Item -ItemType Directory -Force '.\.tool\picotool' | Out-Null
Copy-Item -LiteralPath $p.FullName -Destination '.\.tool\picotool\picotool.exe'
& '.\.tool\picotool\picotool.exe' version
```

Mang thư mục `.tool\picotool\` sang máy mới cùng các công cụ khác. Chế độ `check` vẫn dùng được khi chưa có `picotool.exe`.

## 2. Tạo `build-offline.cmd` nếu repo chưa có

Lưu nội dung sau thành `build-offline.cmd` cạnh `main.c`. Nếu dùng Notepad, chọn **Save as type → All files** để tránh đuôi `.cmd.txt`.

```bat
@echo off
setlocal
cd /d "%~dp0" || exit /b 1

set "PATH=%CD%\.tool\cmake 4.5.1\bin;%CD%\.tool\ninja v1.13.2;%CD%\.tool\arm-gnu 15.3.rel2\bin;%CD%\.tool\python;%PATH%"

python -c "import sys; sys.exit(sys.version_info.major != 3)" || exit /b 1

cmake -S . -B build-offline -G Ninja -DPICO_BOARD=pico_w -DPICO_NO_PICOTOOL=ON -DFETCHCONTENT_FULLY_DISCONNECTED=ON || exit /b 1
cmake --build build-offline --target main || exit /b 1

if /i "%~1"=="check" (
    echo Build OK: build-offline\main.elf
    exit /b 0
)

set "PICOEXE=.tool\picotool\picotool.exe"
if not exist "%PICOEXE%" (
    echo Missing: %PICOEXE%
    exit /b 1
)

"%PICOEXE%" uf2 convert "build-offline\main.elf" "build-offline\main-new.uf2" --family rp2040 --platform rp2040 || exit /b 1
move /y "build-offline\main-new.uf2" "build-offline\main.uf2" >nul || exit /b 1
for %%F in ("build-offline\main.uf2") do echo UF2: %%~fF [%%~zF bytes]
```

Sau khi xuất firmware thành công, dòng cuối hiển thị đường dẫn đầy đủ tới `build-offline\main.uf2` cùng kích thước tệp.

## 3. Các tệp đưa lên Git

Commit `main.c`, `CMakeLists.txt`, `build-offline.cmd`, `README.md` và các thư viện do dự án quản lý. Thêm các dòng sau vào `.gitignore` ở gốc repo để bỏ qua kết quả build cục bộ:

```gitignore
/build-check/
/build-manual/
/build-offline/
```

Nếu muốn quản lý bộ công cụ riêng ngoài Git, thêm `/.tool/` vào `.gitignore` và giữ một bản sao thư mục này để dùng trên máy mới.