# Pico2W Board Configuration with Bluetooth GATT Support

# This configuration is automatically included when:
# - PICO_BOARD=pico2_w AND GP2040_BOARDCONFIG=Pico2W

message(STATUS "Pico2W configuration loaded - GATT header generation handled by main CMakeLists.txt")

# Add Pico2W specific include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Define Pico2W specific compile definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    PICO2W_CONFIG_ENABLED=1
    BLUETOOTH_ENABLED=1
    BLUETOOTH_HID_ENABLED=1
)
