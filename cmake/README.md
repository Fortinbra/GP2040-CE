# CMake Modules Documentation

This directory contains modular CMake files that organize the GP2040-CE build system into logical, reusable components.

## Module Overview

### Core Modules

- **`pico_sdk_import.cmake`** - Standard Pico SDK import script (required by Pico SDK)
- **`compile_proto.cmake`** - Protocol buffer compilation setup using nanopb

### Project Setup Modules

- **`GitVersion.cmake`** - Extracts version information from Git repository
  - Sets `GIT_REPO_VERSION`, `CMAKE_GIT_REPO_VERSION`, `GIT_REPO_BUILD_ID`
  - Generates version header file
  - Function: `setup_git_version()`

- **`GitSubmodules.cmake`** - Handles Git submodule initialization
  - Respects `SKIP_SUBMODULES` environment variable
  - Automatically updates submodules if in Git repository
  - Function: `setup_git_submodules()`

- **`ProjectSetup.cmake`** - Project configuration and dependencies
  - Functions:
    - `setup_board_config()` - Configures GP2040 board settings
    - `setup_project_dependencies()` - Sets up external dependencies (ArduinoJson, Pico-PIO-USB)
    - `setup_compiler_options()` - Configures compiler flags and optimization settings

# CMake Modules Documentation

This directory contains modular CMake files that organize the GP2040-CE build system into logical, reusable components.

## Module Overview

### Core Modules

- **`pico_sdk_import.cmake`** - Standard Pico SDK import script (required by Pico SDK)
- **`compile_proto.cmake`** - Protocol buffer compilation setup using nanopb

### Project Setup Modules

- **`GitVersion.cmake`** - Extracts version information from Git repository
  - Sets `GIT_REPO_VERSION`, `CMAKE_GIT_REPO_VERSION`, `GIT_REPO_BUILD_ID`
  - Generates version header file
  - Function: `setup_git_version()`

- **`GitSubmodules.cmake`** - Handles Git submodule initialization
  - Respects `SKIP_SUBMODULES` environment variable
  - Automatically updates submodules if in Git repository
  - Function: `setup_git_submodules()`

- **`ProjectSetup.cmake`** - Project configuration and dependencies
  - Functions:
    - `setup_board_config()` - Configures GP2040 board settings
    - `setup_project_dependencies()` - Sets up external dependencies (ArduinoJson, Pico-PIO-USB)
    - `setup_compiler_options()` - Configures compiler flags and optimization settings

### Build System Modules

- **`WebBuild.cmake`** - Frontend build process
  - Respects `SKIP_WEBBUILD` environment variable  
  - Handles Node.js/NPM detection and web frontend building
  - Function: `setup_web_build()`

- **`SourceFiles.cmake`** - Automatic source file discovery and organization
  - Automatically discovers source files in organized directory structure
  - Groups files by category (Core, Drivers, Display, Addons, etc.)
  - Function: `setup_source_files(target_name)`

- **`TargetConfig.cmake`** - Complete target configuration
  - Functions:
    - `setup_target_properties()` - Sets target name, version, and Pico properties
    - `setup_target_libraries()` - Links all required libraries
    - `setup_target_includes()` - Configures include directories
    - `setup_target_definitions()` - Sets compile definitions
    - `setup_installation()` - Configures installation rules
    - `configure_target()` - One-stop function that calls all the above

### Utility Modules

- **`FindNodeJS.cmake`** - Locates Node.js installation
- **`FindNPM.cmake`** - Locates NPM package manager

## Usage Pattern

The main `CMakeLists.txt` includes these modules and calls their setup functions:

```cmake
# Include and call setup functions
include(cmake/GitVersion.cmake)
setup_git_version()

include(cmake/ProjectSetup.cmake)
setup_board_config()
setup_project_dependencies()
setup_compiler_options()

include(cmake/SourceFiles.cmake)
setup_source_files(${PROJECT_NAME})

include(cmake/TargetConfig.cmake)
configure_target(${PROJECT_NAME})
```

## Benefits

1. **Extreme Modularity** - Each concern is completely separated
2. **Automatic Discovery** - Source files are found automatically
3. **Maintainability** - Easy to find and modify specific functionality
4. **Clarity** - Main CMakeLists.txt shows only high-level flow
5. **IDE Integration** - Source files are organized in folders for IDEs
6. **Reusability** - Functions can be used for multiple targets
7. **Scalability** - Easy to add new source files without CMake changes
