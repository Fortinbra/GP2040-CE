# WebBuild.cmake
# Module for handling web frontend build process

function(setup_web_build)
    # Check if we should skip web build
    if(DEFINED ENV{SKIP_WEBBUILD})
        set(SKIP_WEBBUILD $ENV{SKIP_WEBBUILD})
    elseif(NOT DEFINED SKIP_WEBBUILD)
        set(SKIP_WEBBUILD FALSE)
    endif()

    if(SKIP_WEBBUILD)
        cmake_print_variables(SKIP_WEBBUILD)
        message(STATUS "Skipping web build")
        return()
    endif()

    message(STATUS "Setting up web build")
    
    # Include Node.js and NPM finder modules
    include(${CMAKE_SOURCE_DIR}/cmake/FindNodeJS.cmake)
    include(${CMAKE_SOURCE_DIR}/cmake/FindNPM.cmake)
    
    # Check if we have the necessary tools and files
    if(NOT NODEJS_FOUND)
        message(WARNING "Node.js not found, skipping web build")
        return()
    endif()
    
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/www")
        message(WARNING "www directory not found, skipping web build")
        return()
    endif()
    
    if(NOT NPM_FOUND)
        message(WARNING "NPM not found, skipping web build")
        return()
    endif()
    
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/www/package.json")
        message(WARNING "package.json not found in www directory, skipping web build")
        return()
    endif()
    
    # Run npm ci to install dependencies
    message(STATUS "Installing web dependencies with npm ci...")
    execute_process(
        COMMAND ${NPM_EXECUTABLE} ci
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/www
        RESULT_VARIABLE NPM_CI_RESULT
    )
    
    if(NOT NPM_CI_RESULT EQUAL "0")
        message(FATAL_ERROR "npm ci failed with ${NPM_CI_RESULT}")
    endif()
    
    # Run npm build
    message(STATUS "Building web frontend with npm run build...")
    execute_process(
        COMMAND ${NPM_EXECUTABLE} run build
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/www
        RESULT_VARIABLE NPM_BUILD_RESULT
    )
    
    if(NOT NPM_BUILD_RESULT EQUAL "0")
        message(FATAL_ERROR "npm run build failed with ${NPM_BUILD_RESULT}")
    endif()
    
    message(STATUS "Web build completed successfully")
endfunction()
