# Driver Architecture V2 - Protocol/Transport Separation
# This file adds the new driver architecture to the build

# Add interface headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/headers/interfaces
)

# Transport implementations
set(TRANSPORT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/interfaces/usbtransport.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/interfaces/bluetoothtransport.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/interfaces/gpiotransport.cpp
)

# Protocol driver implementations
set(PROTOCOL_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/drivers/xinput/XInputProtocolDriver.cpp
    # Add other protocol drivers here as they are implemented
    # ${CMAKE_CURRENT_LIST_DIR}/src/drivers/ps4/PS4ProtocolDriver.cpp
    # ${CMAKE_CURRENT_LIST_DIR}/src/drivers/snes/SNESProtocolDriver.cpp
)

# Enhanced driver manager
set(MANAGER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/drivermanager_v2.cpp
    ${CMAKE_CURRENT_LIST_DIR}/src/driverintegration.cpp
)

# Add all new architecture sources
target_sources(${PROJECT_NAME} PRIVATE
    ${TRANSPORT_SOURCES}
    ${PROTOCOL_SOURCES}
    ${MANAGER_SOURCES}
)

# Conditional compilation flags for optional features
option(ENABLE_NEW_DRIVER_ARCHITECTURE "Enable new protocol/transport driver architecture" ON)
option(ENABLE_BLUETOOTH_TRANSPORT "Enable Bluetooth transport (requires BTStack)" OFF)
option(ENABLE_GPIO_TRANSPORT "Enable GPIO transport for retro consoles" ON)

if(ENABLE_NEW_DRIVER_ARCHITECTURE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ENABLE_NEW_DRIVER_ARCHITECTURE=1
    )
endif()

if(ENABLE_BLUETOOTH_TRANSPORT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ENABLE_BLUETOOTH_TRANSPORT=1
    )
    # Add BTStack dependency when available
    # find_package(BTStack REQUIRED)
    # target_link_libraries(${PROJECT_NAME} PRIVATE BTStack::BTStack)
endif()

if(ENABLE_GPIO_TRANSPORT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ENABLE_GPIO_TRANSPORT=1
    )
endif()

# Add protocol-specific feature flags
option(ENABLE_XINPUT_PROTOCOL_V2 "Enable XInput protocol driver v2" ON)
option(ENABLE_PS4_PROTOCOL_V2 "Enable PS4 protocol driver v2" OFF)
option(ENABLE_SNES_PROTOCOL "Enable SNES protocol driver" OFF)

if(ENABLE_XINPUT_PROTOCOL_V2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ENABLE_XINPUT_PROTOCOL_V2=1
    )
endif()

if(ENABLE_PS4_PROTOCOL_V2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ENABLE_PS4_PROTOCOL_V2=1
    )
endif()

if(ENABLE_SNES_PROTOCOL)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ENABLE_SNES_PROTOCOL=1
    )
endif()

# Documentation
install(FILES
    ${CMAKE_CURRENT_LIST_DIR}/docs/driver-architecture-v2.md
    DESTINATION docs
    COMPONENT documentation
    OPTIONAL
)
