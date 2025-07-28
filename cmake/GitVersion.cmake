# GitVersion.cmake
# Module for extracting version information from Git repository

function(setup_git_version)
    # Find Git package
    find_package(Git)
    
    if(NOT GIT_FOUND)
        message(WARNING "Git not found, using default version values")
        set(GIT_REPO_VERSION "0.0.0" PARENT_SCOPE)
        set(CMAKE_GIT_REPO_VERSION "0.0.0" PARENT_SCOPE)
        set(GIT_REPO_BUILD_ID "unknown" PARENT_SCOPE)
        return()
    endif()
    
    # Get full version string with tags
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --always --dirty --abbrev=7
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_REPO_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # Get build ID (short commit hash)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always --abbrev=7
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_REPO_BUILD_ID
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # Extract semantic version from tag (e.g., v1.2.3 -> 1.2.3)
    string(REGEX REPLACE "v([0-9]+\\.[0-9]+\\.[0-9]+).*" "\\1" CMAKE_GIT_REPO_VERSION ${GIT_REPO_VERSION})
    
    # If we don't have a proper version tag, fall back to 0.0.0
    string(REGEX REPLACE "^(.......-.*)|(.......)$" "0.0.0" CMAKE_GIT_REPO_VERSION ${CMAKE_GIT_REPO_VERSION})
    
    # Make variables available to parent scope
    set(GIT_REPO_VERSION ${GIT_REPO_VERSION} PARENT_SCOPE)
    set(CMAKE_GIT_REPO_VERSION ${CMAKE_GIT_REPO_VERSION} PARENT_SCOPE)
    set(GIT_REPO_BUILD_ID ${GIT_REPO_BUILD_ID} PARENT_SCOPE)
    
    # Generate version header
    configure_file("${CMAKE_SOURCE_DIR}/headers/version.h.in" "${CMAKE_BINARY_DIR}/headers/version.h")
    
    # Output version information
    message(STATUS "Git version information:")
    message(STATUS "  GIT_REPO_VERSION: ${GIT_REPO_VERSION}")
    message(STATUS "  CMAKE_GIT_REPO_VERSION: ${CMAKE_GIT_REPO_VERSION}")
    message(STATUS "  GIT_REPO_BUILD_ID: ${GIT_REPO_BUILD_ID}")
endfunction()
