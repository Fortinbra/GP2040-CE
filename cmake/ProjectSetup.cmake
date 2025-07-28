# ProjectSetup.cmake
# Module for setting up project dependencies and configuration

function(setup_project_dependencies)
    # Set board configuration header directory
    set(PICO_BOARD_HEADER_DIRS ${CMAKE_SOURCE_DIR}/configs/${GP2040_BOARDCONFIG} PARENT_SCOPE)
    
    # Include FetchContent for external dependencies
    include(FetchContent)
    
    # Fetch ArduinoJson library
    message(STATUS "Fetching ArduinoJson library...")
    FetchContent_Declare(ArduinoJson
        GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
        GIT_TAG        v6.21.2
    )
    FetchContent_MakeAvailable(ArduinoJson)
    
    # Configure Pico-PIO-USB path
    if(DEFINED ENV{PICO_PIO_USB_PATH})
        message(STATUS "Found custom Pico-PIO-USB path, using it.")
        set(PICO_PIO_USB_PATH $ENV{PICO_PIO_USB_PATH} PARENT_SCOPE)
    elseif(NOT DEFINED PICO_PIO_USB_PATH)
        message(STATUS "Using default Pico-PIO-USB.")
        set(PICO_PIO_USB_PATH ${CMAKE_SOURCE_DIR}/lib/pico_pio_usb PARENT_SCOPE)
    endif()
    
    # Set TinyUSB path
    set(PICO_TINYUSB_PATH "${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb" PARENT_SCOPE)
    
    message(STATUS "Project dependencies configured successfully")
endfunction()

function(setup_compiler_options)
    # Add compiler warning options
    add_compile_options(-Wall
        -Wtype-limits
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
    )
    
    # Remove unused code and data
    add_compile_options(-fdata-sections -ffunction-sections)
    add_link_options(-Wl,--gc-sections)
    
    # Debug build specific options
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Activate some compiler / linker options to aid us with diagnosing stack space issues in Debug builds
        add_compile_options(-fstack-usage -Wstack-usage=500)
        add_compile_definitions(PICO_USE_STACK_GUARDS=1)
        message(STATUS "Debug build options enabled")
    endif()
    
    # Set larger stack size (4kb per core instead of default 2kb)
    add_compile_definitions(PICO_STACK_SIZE=0x1000)
    
    message(STATUS "Compiler options configured")
endfunction()

function(setup_board_config)
    # Set GP2040 board configuration
    if(DEFINED ENV{GP2040_BOARDCONFIG})
        set(GP2040_BOARDCONFIG $ENV{GP2040_BOARDCONFIG} PARENT_SCOPE)
    elseif(NOT DEFINED GP2040_BOARDCONFIG)
        set(GP2040_BOARDCONFIG Pico PARENT_SCOPE)
    endif()
    
    message(STATUS "Board configuration: ${GP2040_BOARDCONFIG}")
endfunction()
