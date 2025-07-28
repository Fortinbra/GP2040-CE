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

# etc.
```

## Benefits

1. **Modularity** - Each concern is separated into its own file
2. **Reusability** - Functions can be called multiple times if needed
3. **Maintainability** - Easier to find and modify specific functionality
4. **Clarity** - Main CMakeLists.txt shows high-level build flow
5. **Testing** - Individual modules can be tested in isolation
