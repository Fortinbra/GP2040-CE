# Driver Architecture V2 - Protocol/Transport Separation
# This file adds the new driver architecture to the build

# Add interface headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../headers/interfaces
)

# Core transport implementations (always included)
set(CORE_TRANSPORT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../src/interfaces/gpiotransport.cpp
)

# USB transport (mutually exclusive with legacy USB driver)
set(USB_TRANSPORT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../src/interfaces/usbtransport.cpp
)

# Optional Bluetooth transport (only if enabled)
set(BLUETOOTH_TRANSPORT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../src/interfaces/bluetoothtransport.cpp
)

# Protocol driver implementations
set(PROTOCOL_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../src/drivers/xinput/XInputProtocolDriver.cpp
    # Add other protocol drivers here as they are implemented
    # ${CMAKE_CURRENT_LIST_DIR}/../src/drivers/ps4/PS4ProtocolDriver.cpp
    # ${CMAKE_CURRENT_LIST_DIR}/../src/drivers/snes/SNESProtocolDriver.cpp
)

# Enhanced driver manager
set(MANAGER_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../src/drivermanager_v2.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../src/driverintegration.cpp
)

# NOTE: Options are defined in main CMakeLists.txt to be available during source selection

# Start with core transport sources
set(TRANSPORT_SOURCES ${CORE_TRANSPORT_SOURCES})

# Add USB transport when new architecture is enabled (mutually exclusive with legacy)
if(ENABLE_NEW_DRIVER_ARCHITECTURE)
    list(APPEND TRANSPORT_SOURCES ${USB_TRANSPORT_SOURCES})
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ENABLE_USB_TRANSPORT=1
    )
    message(STATUS "USB transport enabled (new architecture)")
endif()

# Add Bluetooth transport conditionally
if(ENABLE_BLUETOOTH_TRANSPORT)
    list(APPEND TRANSPORT_SOURCES ${BLUETOOTH_TRANSPORT_SOURCES})
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ENABLE_BLUETOOTH_TRANSPORT=1
    )
    message(STATUS "Bluetooth transport enabled")
    
    # TODO: Add BTStack dependency when available
    # find_package(BTStack REQUIRED)
    # target_link_libraries(${PROJECT_NAME} PRIVATE BTStack::BTStack)
else()
    message(STATUS "Bluetooth transport disabled")
endif()

# Add all architecture sources to the build
if(ENABLE_NEW_DRIVER_ARCHITECTURE)
    target_sources(${PROJECT_NAME} PRIVATE
        ${TRANSPORT_SOURCES}
        ${PROTOCOL_SOURCES}
        ${MANAGER_SOURCES}
    )
    message(STATUS "Added ${CMAKE_CURRENT_LIST_DIR}/../src/drivermanager_v2.cpp to build")
    message(STATUS "Added ${CMAKE_CURRENT_LIST_DIR}/../src/driverintegration.cpp to build")
else()
    message(STATUS "New driver architecture disabled - not adding source files")
endif()

if(ENABLE_NEW_DRIVER_ARCHITECTURE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ENABLE_NEW_DRIVER_ARCHITECTURE=1
    )
    message(STATUS "New driver architecture enabled")
else()
    message(STATUS "New driver architecture disabled - using legacy drivers only")
endif()

if(ENABLE_GPIO_TRANSPORT)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        ENABLE_GPIO_TRANSPORT=1
    )
    message(STATUS "GPIO transport enabled")
else()
    message(STATUS "GPIO transport disabled")
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
