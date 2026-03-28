# GitHub Copilot Instructions for GP2040-CE

This document guides GitHub Copilot when contributing to the GP2040-CE project. Follow these conventions for all code and documentation changes.

## Code Style and Formatting

### Indentation
- **Always use 4 spaces for indentation**
- **Never use tabs** — configure your editor to show and convert tabs to spaces
- Maintain consistent indentation across all file types (C/C++, CMake, JSON, headers, etc.)

**Correct example (C++):**
```cpp
void setupController() {
    if (initialized) {
        configureInputs();
        if (debugMode) {
            enableLogging();
        }
    }
}
```

**Incorrect example (tabs or mixed spacing):**
```cpp
void setupController() {
	if (initialized) {  // Tab used — WRONG
	  configureInputs();  // 2 spaces — WRONG
	}
}
```

### C/C++ Conventions
- Use standard RP2040 SDK conventions for hardware access
- Prefer explicit type names over `auto` for clarity
- Use header guards in `.h` files: `#ifndef HEADER_NAME_H` / `#define HEADER_NAME_H`
- Include necessary Pico SDK headers: `#include "pico/stdlib.h"`, etc.
- Keep lines reasonable length for readability

### CMake Formatting
- Use 4-space indentation for all CMake files
- Use lowercase commands: `target_sources()`, `target_include_directories()`, etc.
- Align multiline lists for readability
- Use descriptive variable names (not abbreviated)

**CMake example:**
```cmake
target_sources(gp2040_firmware PRIVATE
    main.cpp
    gamepad.cpp
    config.cpp
)

target_include_directories(gp2040_firmware PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/headers
)
```

### Commenting
- Only comment code that needs clarification — avoid over-commenting obvious logic
- Comments should explain *why* not *what*
- Use `//` for single-line comments, `/* */` for block comments when appropriate

## Platform and SDK Configuration

### Default Target Board
- **Default to Raspberry Pi Pico** unless explicitly specified otherwise
- Use environment variable: `PICO_BOARD=pico`
- Supported boards are listed in `configs/` directory
- Only suggest alternative boards (Pico W, Pico 2, etc.) when explicitly requested

### Pico SDK Version
- **Always use Pico SDK version 2.2.0** for all builds (RP2040 and RP2350)
- SDK path: `${env:USERPROFILE}/.pico-sdk/`
- SDK is imported via `pico_sdk_import.cmake`

### Build Tools
- **Ninja**: v1.12.1
- **Picotool**: 2.2.0 (for flashing and device operations)
- **OpenOCD**: 0.12.0+dev (for debugging)
- **CMake**: 3.10+

## Build System

### CMake Configuration
Standard Pico build command:
```bash
cmake -B build -S .
```

With environment variables:
```
PICO_BOARD=pico
SKIP_WEBBUILD=TRUE
GP2040_BOARDCONFIG=Pico
```

### Build Directory
- Always use `build/` directory for CMake builds
- Output files: `.uf2`, `.elf`, `.hex` go to `build/` subdirectories
- Do not commit build artifacts

### VS Code Tasks
When working in VS Code, prefer using the built-in tasks:
- **Configure CMake (Standard Pico)** — for standard Pico builds
- **Configure CMake (Pico W)** — only when WiFi features are needed
- **Compile Project** — to build after configuration
- **Run Project** — to flash via picotool

## Project Structure

### Directory Layout
- **`src/`** — Firmware C++ implementation
  - `main.cpp` — entry point
  - `gp2040.cpp` — core firmware logic
  - `gamepad.cpp` — gamepad interface
  - `drivers/` — hardware drivers (USB, input, LEDs, etc.)
  - `addons/` — optional feature implementations
  - `display/` — display/OLED support
  
- **`headers/`** — Data structures and interfaces
  - `.h` files defining the public API and data types
  - Always `#include` from this directory for external interfaces
  
- **`configs/`** — Board-specific configurations
  - `Pico/` — default Raspberry Pi Pico configuration
  - Other boards have their own subdirectories
  - Each contains a `.cmake` file with board-specific settings
  
- **`lib/`** — Third-party libraries
  - External dependencies integrated into the project
  - Do not modify without clear justification
  
- **`proto/`** — Protocol Buffer definitions
  - `.proto` files for configuration serialization
  - Compiled to C++ via `compile_proto.cmake`
  
- **`www/`** — React web configurator
  - TypeScript/React code for the web UI
  - Compiled as embedded resources in the firmware
  
- **`docs/`** — Documentation
  - Markdown files for feature documentation
  - Architecture notes and design documents
  
- **`modules/`** — Modular firmware components
  - Logically grouped functionality

### Header Inclusion
- Include paths are configured in CMakeLists.txt
- Prefer `#include "path/to/file.h"` for local headers
- Use `#include <pico/...>` for Pico SDK headers
- Follow the directory structure for intuitive includes

## Git and Branching Policy

### CRITICAL RULES
- **NEVER commit directly to `main`** — Always use a feature branch
- **NEVER commit to `upstream` under any circumstances**
- **All changes must be on a feature branch** before creating a pull request

### Branch Naming Convention
Use descriptive names with prefixes:
- `feature/` — New features (e.g., `feature/turbo-mode`)
- `fix/` — Bug fixes (e.g., `fix/socd-neutral`)
- `docs/` — Documentation changes (e.g., `docs/api-reference`)
- `chore/` — Build, refactor, dependencies (e.g., `chore/update-sdk`)
- `test/` — Tests and test improvements (e.g., `test/input-validation`)

### Workflow
1. Create a feature branch from `main`: `git checkout -b feature/your-feature`
2. Make commits with clear, descriptive messages
3. Reference related issues in commit messages: `Fixes #123`
4. When ready, push the branch and create a pull request
5. After approval and merging, the branch can be deleted

### Commit Messages
- Use imperative mood: "Add feature" not "Added feature"
- First line should be 50 characters or less, followed by blank line
- Reference issues: `Fixes #123` or `Related to #456`
- Example:
  ```
  Add SOCD neutral detection
  
  Implements the neutral input priority SOCD cleaning mode,
  allowing simultaneous opposite cardinal directions to output
  neutral input instead of last-input priority.
  
  Fixes #42
  ```

## Documentation

### Markdown Files
- Documentation belongs in `docs/` directory
- Use `.md` extension for Markdown files
- Follow standard Markdown formatting
- Include code blocks with language specification:
  ```markdown
  \`\`\`cpp
  // C++ code example
  void myFunction() { }
  \`\`\`
  ```

### Code Documentation
- Use doc comments for public APIs:
  ```cpp
  /**
   * Initialize the gamepad subsystem.
   * @param mode Input mode (e.g., XInput, PS4)
   * @return Status code
   */
  int initGamepad(uint8_t mode);
  ```

## Pull Requests and Code Review

### PR Guidelines
- Keep PRs focused and scoped to a single feature or fix
- Reference related issues in the PR description
- Include brief description of changes and rationale
- Add test coverage when adding new features
- Ensure CI passes before requesting review

### Review Expectations
- Code must follow the style guide in this document
- Indentation must be exactly 4 spaces (no exceptions)
- No hardcoded board assumptions — respect `PICO_BOARD` configuration
- Include comments only where code clarity genuinely benefits
- Performance matters — minimize latency and memory usage

## Testing

### Before Committing
- Verify code compiles with `cmake` and build commands
- Test with the default Pico board configuration
- Check for new compiler warnings with the standard build flags
- If changing core features, verify with physical hardware when possible

### Test Patterns
- Use assertions and debug output for validation
- Follow existing test patterns in the codebase
- Document expected behavior in test comments

## Additional Notes

### Performance Considerations
- GP2040-CE prioritizes **ultra-low input latency** (target: <1ms)
- Avoid blocking operations in interrupt handlers
- Prefer efficient data structures (arrays over linked lists when possible)
- Profile changes that affect the gamepad loop

### Hardware Constraints
- Target hardware: RP2040 microcontroller (limited RAM and flash)
- Be mindful of memory usage — the Pico has 264KB of SRAM
- Flash storage is limited — web UI and configs are stored here

### Dependencies
- Minimize external dependencies to reduce firmware size
- Third-party libraries should be in `lib/`
- Justify all new dependencies in the PR description

## Copilot-Specific Guidance

When contributing as GitHub Copilot:
1. **Always verify indentation** — set your editor to show tabs and use 4 spaces
2. **Maintain consistency** — match the style of surrounding code
3. **Test before suggesting** — ensure code compiles and works as intended
4. **Document your changes** — include comments explaining non-obvious decisions
5. **Respect constraints** — remember the RP2040's limited resources
6. **Follow branching rules** — create a feature branch, never commit to main or upstream

---

**Last updated:** 2025-03-28  
**Maintained by:** Roy Mustang, Project Lead
