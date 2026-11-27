cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include("${CMAKE_CURRENT_LIST_DIR}/pico-sdk-master/pico_sdk_init.cmake")

project(pico_smoke_test C CXX ASM)
pico_sdk_init()

add_executable(pico_smoke_test pico_smoke_test.c)
target_link_libraries(pico_smoke_test PRIVATE pico_stdlib)