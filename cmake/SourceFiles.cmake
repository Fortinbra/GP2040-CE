# SourceFiles.cmake
# Module for organizing and collecting source files

function(setup_source_files target_name)
    # Core source files in src/
    set(CORE_SOURCES
        src/main.cpp
        src/gp2040.cpp
        src/gp2040aux.cpp
        src/gamepad.cpp
        src/addonmanager.cpp
        src/playerleds.cpp
        src/drivermanager.cpp
        src/eventmanager.cpp
        src/layoutmanager.cpp
        src/peripheralmanager.cpp
        src/storagemanager.cpp
        src/system.cpp
        src/usbdriver.cpp
        src/usbhostmanager.cpp
        src/config_legacy.cpp
        src/config_utils.cpp
        src/webconfig.cpp
    )
    
    # Gamepad sources
    file(GLOB_RECURSE GAMEPAD_SOURCES "src/gamepad/*.cpp")
    
    # Driver sources
    file(GLOB_RECURSE DRIVER_SOURCES "src/drivers/*.cpp" "src/drivers/*.c")
    
    # Interface sources
    file(GLOB_RECURSE INTERFACE_SOURCES "src/interfaces/*.cpp")
    
    # Display UI sources
    file(GLOB_RECURSE DISPLAY_SOURCES "src/display/*.cpp")
    
    # Addon sources
    file(GLOB_RECURSE ADDON_SOURCES "src/addons/*.cpp")
    
    # Animation station sources
    file(GLOB_RECURSE ANIMATION_SOURCES "src/animationstation/*.cpp")
    
    # Protocol buffer generated sources
    set(PROTO_SOURCES
        ${PROTO_OUTPUT_DIR}/enums.pb.c
        ${PROTO_OUTPUT_DIR}/config.pb.c
    )
    
    # Combine all sources
    set(ALL_SOURCES
        ${CORE_SOURCES}
        ${GAMEPAD_SOURCES}
        ${DRIVER_SOURCES}
        ${INTERFACE_SOURCES}
        ${DISPLAY_SOURCES}
        ${ADDON_SOURCES}
        ${ANIMATION_SOURCES}
        ${PROTO_SOURCES}
    )
    
    # Create the executable with all sources
    add_executable(${target_name} ${ALL_SOURCES})
    
    # Optional: Print source file count for debugging
    list(LENGTH ALL_SOURCES SOURCE_COUNT)
    message(STATUS "Added ${SOURCE_COUNT} source files to ${target_name}")
    
    # Optional: Organize sources in IDE (Visual Studio, Xcode, etc.)
    source_group("Core" FILES ${CORE_SOURCES})
    source_group("Gamepad" FILES ${GAMEPAD_SOURCES})
    source_group("Drivers" FILES ${DRIVER_SOURCES})
    source_group("Interfaces" FILES ${INTERFACE_SOURCES})
    source_group("Display" FILES ${DISPLAY_SOURCES})
    source_group("Addons" FILES ${ADDON_SOURCES})
    source_group("Animation" FILES ${ANIMATION_SOURCES})
    source_group("Protocol Buffers" FILES ${PROTO_SOURCES})
endfunction()
