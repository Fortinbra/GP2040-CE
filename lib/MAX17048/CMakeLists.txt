cmake_minimum_required(VERSION 3.13)

set(LIB_NAME MAX17048)

add_library(${LIB_NAME} STATIC)

target_sources(${LIB_NAME}
    PRIVATE MAX17048.cpp
)

target_include_directories(${LIB_NAME}
    PUBLIC ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${LIB_NAME}
    PUBLIC PicoPeripherals
    PUBLIC pico_stdlib
    PUBLIC hardware_i2c
)
