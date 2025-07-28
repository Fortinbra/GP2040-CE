# TargetConfig.cmake
# Module for configuring target properties, libraries, and includes

function(setup_target_properties target_name)
    # Set target output name with version and board config
    set_target_properties(${target_name} PROPERTIES 
        OUTPUT_NAME ${target_name}_${CMAKE_PROJECT_VERSION}_${GP2040_BOARDCONFIG}
    )
    
    # Set Pico program properties
    pico_set_program_name(${target_name} "GP2040-CE")
    pico_set_program_version(${target_name} ${GIT_REPO_VERSION})
    
    # Add extra outputs (UF2, hex, etc.)
    pico_add_extra_outputs(${target_name})
    
    message(STATUS "Target properties configured for ${target_name}")
endfunction()

function(setup_target_libraries target_name)
    # Define all required libraries
    set(TARGET_LIBRARIES
        # Pico SDK libraries
        pico_stdlib
        pico_bootsel_via_double_reset
        hardware_adc
        hardware_pwm
        pico_mbedtls
        
        # USB libraries
        tinyusb_host
        tinyusb_pico_pio_usb
        
        # Project libraries
        CRC32
        FlashPROM
        ADS1219
        ADS1256
        NeoPico
        OneBitDisplay
        PicoPeripherals
        WiiExtension
        SNESpad
        
        # External libraries
        ArduinoJson
        rndis
        nanopb
    )
    
    # Link all libraries to target
    target_link_libraries(${target_name} ${TARGET_LIBRARIES})
    
    message(STATUS "Linked libraries to ${target_name}")
endfunction()

function(setup_target_includes target_name)
    # Public include directories
    set(PUBLIC_INCLUDES
        headers
        headers/addons
        headers/configs
        headers/drivers
        headers/drivers/shared
        headers/events
        headers/interfaces
        headers/interfaces/i2c
        headers/interfaces/i2c/ads1219
        headers/interfaces/i2c/pcf8575
        headers/interfaces/i2c/ssd1306
        headers/interfaces/i2c/wiiextension
        headers/gamepad
        headers/display
        headers/display/fonts
        headers/display/ui
        headers/display/ui/static
        headers/display/ui/elements
        headers/display/ui/screens
        headers/animationstation
        headers/animationstation/effects
        configs/${GP2040_BOARDCONFIG}
        ${PROTO_OUTPUT_DIR}
        ${CMAKE_BINARY_DIR}/headers
    )
    
    # Private include directories
    set(PRIVATE_INCLUDES
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/.. # for common lwipopts or other standard includes
    )
    
    # Set include directories
    target_include_directories(${target_name} PUBLIC ${PUBLIC_INCLUDES})
    target_include_directories(${target_name} PRIVATE ${PRIVATE_INCLUDES})
    
    message(STATUS "Include directories configured for ${target_name}")
endfunction()

function(setup_target_definitions target_name)
    # Compile definitions
    target_compile_definitions(${target_name} PUBLIC
        PICO_XOSC_STARTUP_DELAY_MULTIPLIER=64
        BOARD_CONFIG_FILE_NAME="$<TARGET_FILE_BASE_NAME:${target_name}>"
        GP2040_BOARDCONFIG="${GP2040_BOARDCONFIG}"
    )
    
    message(STATUS "Compile definitions configured for ${target_name}")
endfunction()

function(setup_installation target_name)
    # Install files
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${target_name}_${CMAKE_PROJECT_VERSION}_${GP2040_BOARDCONFIG}.uf2
        ${CMAKE_CURRENT_LIST_DIR}/README.md
        DESTINATION .
    )
    
    message(STATUS "Installation configured for ${target_name}")
endfunction()

function(configure_target target_name)
    # Configure all target aspects
    setup_target_properties(${target_name})
    setup_target_libraries(${target_name})
    setup_target_includes(${target_name})
    setup_target_definitions(${target_name})
    setup_installation(${target_name})
    
    message(STATUS "Target ${target_name} fully configured")
endfunction()
