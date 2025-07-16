# PicoW Board Configuration with Bluetooth GATT Support

# This configuration is automatically included when:
# - PICO_BOARD=pico_w AND GP2040_BOARDCONFIG=PicoW

message(STATUS "PicoW configuration loaded - GATT header generation handled by main CMakeLists.txt")

# Add PicoW specific include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Define PicoW specific compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PICOW_CONFIG_ENABLED=1
    BLUETOOTH_ENABLED=1
    BLUETOOTH_HID_ENABLED=1
)