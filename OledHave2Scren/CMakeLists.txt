# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.1)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(OledHave2Scren C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(OledHave2Scren
    OledHave2Scren.c
    core0.c
    src/app_hardware.c
    src/task0.c
)

option(PICO_INIT_ON_CORE1 "Chạy khởi tạo và kiểm tra DMA trên Core 1 vật lý" OFF)
if(PICO_INIT_ON_CORE1)
    target_compile_definitions(OledHave2Scren PRIVATE PICO_INIT_ON_CORE1=1)
endif()

option(PICO_PIO_TASKS "Nhận mã tác vụ từ PIO và chạy trên Core 1" ON)
if(PICO_PIO_TASKS OR PICO_INIT_ON_CORE1)
    target_sources(OledHave2Scren PRIVATE core1.c)
    target_link_libraries(OledHave2Scren hardware_watchdog pico_multicore)
endif()

if(PICO_PIO_TASKS)
    target_compile_definitions(OledHave2Scren PRIVATE PICO_PIO_TASKS=1)
    pico_generate_pio_header(OledHave2Scren ${CMAKE_CURRENT_LIST_DIR}/task_input.pio)
    target_link_libraries(OledHave2Scren hardware_pio)
endif()

option(PICO_OLED_UI_TEST "Bật kiểm thử ảnh SRAM qua DMA trên OLED 1" OFF)
if(PICO_OLED_UI_TEST)
    target_compile_definitions(OledHave2Scren PRIVATE PICO_OLED_UI_TEST=1)
endif()
if(PICO_OLED_UI_TEST OR PICO_PIO_TASKS)
    target_sources(OledHave2Scren PRIVATE
        src/app_tasks.c
        src/task1.c
        src/task2.c
        src/task3.c
        src/ssd1306.c
        src/display_dma.c
    )
endif()

pico_set_program_name(OledHave2Scren "OledHave2Scren")
pico_set_program_version(OledHave2Scren "0.1")

# Báo lỗi bằng LED nên tắt đầu ra chuẩn qua UART và USB.
pico_enable_stdio_uart(OledHave2Scren 0)
pico_enable_stdio_usb(OledHave2Scren 0)

# Add the standard library to the build
target_link_libraries(OledHave2Scren
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(OledHave2Scren PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/include
)

# Add any user requested libraries
target_link_libraries(OledHave2Scren 
        hardware_spi
        hardware_i2c
        hardware_uart
        hardware_dma
        pico_cyw43_arch_none
        )

pico_add_extra_outputs(OledHave2Scren)

