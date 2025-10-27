cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(crypto C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_EXAMPLES_PATH ${PROEJECT_SOURCE_DIR})


pico_sdk_init()

add_executable(main devFiles/rasp/src/function/Main.c 
)

target_link_libraries(main pico_stdlib)

pico_add_extra_outputs(main)