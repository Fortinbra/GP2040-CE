# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
if(DEFINED ENV{PICO_BOARD})
  set(PICO_BOARD $ENV{PICO_BOARD} CACHE STRING "Board type")
else()
  set(PICO_BOARD pico CACHE STRING "Board type")
endif()

cmake_minimum_required(VERSION 3.10...4.0)
include(CMakePrintHelpers)

# enable compile commands for use by IDE autocompletion
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(cmake/pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.1.1")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.1.1 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

# Setup Git version information
include(cmake/GitVersion.cmake)
setup_git_version()

# Uncomment the next line for an unomptimized build for debugging. Use in conjunction with the Debug build type.
# set(PICO_DEOPTIMIZED_DEBUG 1)

project(GP2040-CE LANGUAGES C CXX ASM VERSION ${CMAKE_GIT_REPO_VERSION})
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Setup board configuration
include(cmake/ProjectSetup.cmake)
setup_board_config()

# Setup Git submodules
include(cmake/GitSubmodules.cmake)
setup_git_submodules()

# Setup web build
include(cmake/WebBuild.cmake)
setup_web_build()

# Setup project dependencies and paths
setup_project_dependencies()

# Setup compiler options
setup_compiler_options()

# Compile protocol buffers
include(cmake/compile_proto.cmake)
compile_proto()

# Initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add lib subdirectory
add_subdirectory(lib)

# Create executable with organized source files
include(cmake/SourceFiles.cmake)
setup_source_files(${PROJECT_NAME})

# Configure target properties, libraries, includes, and definitions
include(cmake/TargetConfig.cmake)
configure_target(${PROJECT_NAME})

if (NOT (DEFINED ENV(CI)) AND (EXISTS ${CMAKE_SOURCE_DIR}/cmake/Custom.cmake))
  message(STATUS "Found custom script.")
	include(${CMAKE_SOURCE_DIR}/cmake/Custom.cmake)
endif()
