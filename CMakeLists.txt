cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include("${CMAKE_CURRENT_LIST_DIR}/pico-sdk-master/pico_sdk_init.cmake")

project(main C CXX ASM)
pico_sdk_init()

add_executable(main main.c)

target_sources(main PRIVATE
    .lib/SSD1308/ssd1308.c
)

target_include_directories(main PRIVATE
    .lib/SSD1308
)

target_link_libraries(main PRIVATE
    pico_stdlib
    hardware_i2c
)

pico_add_extra_outputs(main)