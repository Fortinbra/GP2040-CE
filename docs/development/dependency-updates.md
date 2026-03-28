# Dependency Management

## Overview

GP2040-CE uses a two-tier dependency system: **firmware dependencies** (C/C++ libraries for the RP2040 microcontroller) and **web configurator dependencies** (Node.js/React packages for the browser-based UI). Both tiers maintain stability by pinning specific versions while providing clear upgrade paths that preserve backward compatibility with existing setups.

The project prioritizes **no breaking changes to user configurations**. When dependencies are updated, comprehensive testing ensures that existing gamepad configurations and saved settings continue to work without migration.

## Firmware Dependencies (C/C++)

### Core SDK

| Dependency | Current Version | Source | Purpose |
|---|---|---|---|
| Raspberry Pi Pico SDK | 2.2.0 | `CMakeLists.txt` (`sdkVersion`), CI (`cmake.yml`) | Core microcontroller SDK — provides APIs for GPIO, USB, timers, and platform-specific functionality |

The `pico_sdk_import.cmake` file manages SDK sourcing. It supports three modes:
1. **PICO_SDK_PATH** (environment variable) — Use a locally installed SDK
2. **PICO_SDK_FETCH_FROM_GIT** — Fetch the SDK from GitHub at the specified tag
3. **Default behavior** — Look for SDK in the user's `.pico-sdk/` directory (VS Code extension standard)

**Minimum version requirement:** The `CMakeLists.txt` enforces Pico SDK 2.2.0 as the project-wide minimum for **all** builds — RP2040 and RP2350 alike. This version is pinned in `CMakeLists.txt` (`set(sdkVersion 2.2.0)`) and in CI. The build will halt with a fatal error if an older SDK is used:
```cmake
if (PICO_SDK_VERSION_STRING VERSION_LESS "2.2.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.2.0 (or later) required...")
endif()
```


### Verifying your SDK version

Check the SDK version file at `$PICO_SDK_PATH/pico_sdk_version.cmake`:
```bash
cat $PICO_SDK_PATH/pico_sdk_version.cmake
# Look for: set(PICO_SDK_VERSION_STRING "2.2.0")
```

Or let CMake confirm it during configuration — the output includes the detected SDK version, and a version below 2.2.0 will produce a `FATAL_ERROR`.

### FetchContent Libraries

| Dependency | Current Version | Source | Purpose |
|---|---|---|---|
| ArduinoJson | v6.21.2 | GitHub: `bblanchon/ArduinoJson` (FetchContent) | JSON parsing and serialization for configuration management |

ArduinoJson is fetched and built as part of the CMake build process. The version is pinned in `CMakeLists.txt`.

### Vendored Libraries (lib/ directory)

| Dependency | Location | Purpose |
|---|---|---|
| **pico-pio-usb** | `lib/pico_pio_usb` | Pico SDK extension — enables USB device operation via PIO (Programmable I/O) |
| **TinyUSB** | `lib/tinyusb` | USB stack — powers USB device communication |
| **nanopb** | `lib/nanopb` | Protocol Buffers C implementation — serializes/deserializes config protocol |
| **lwIP** | `lib/lwip-port` | TCP/IP stack port for Pico SDK |
| **rndis** | `lib/rndis` | RNDIS protocol support for network communication |
| **httpd** | `lib/httpd` | HTTP server for web configurator access |
| **ADS1219** | `lib/ADS1219` | I2C ADC driver for analog input |
| **ADS1256** | `lib/ADS1256` | SPI ADC driver for high-precision analog input |
| **CRC32** | `lib/CRC32` | CRC-32 checksum computation |
| **FlashPROM** | `lib/FlashPROM` | Flash memory management for non-volatile configuration storage |
| **NeoPico** | `lib/NeoPico` | NeoPixel RGB LED control |
| **OneBitDisplay** | `lib/OneBitDisplay` | OLED/LCD display driver (SSD1306 and others) |
| **PicoPeripherals** | `lib/PicoPeripherals` | GPIO and peripheral management utilities |
| **WiiExtension** | `lib/WiiExtension` | Wii controller extension interface |
| **SNESpad** | `lib/SNESpad` | SNES controller protocol handler |

**Vendored strategy:** Bundled libraries are either:
- Submodules (tracked in `.gitmodules`) that pull specific commits from upstream repositories
- Forks maintained by the GP2040-CE project with custom patches
- Third-party libraries used as-is, with source included for full transparency

Each vendored library has its own `CMakeLists.txt` and is included via `add_subdirectory()` in `lib/CMakeLists.txt`.

## Web Configurator Dependencies (Node.js/React)

The web configurator (`www/` directory) is a React application built with Vite and compiled into the firmware as a static asset. All npm dependencies are locked in `www/package-lock.json` for reproducible builds.

> **Version format note:** The "Current Version" column shows the caret ranges defined in `package.json` (e.g., `^18.2.0`), which allow compatible minor/patch updates. The exact pinned versions for each dependency are in `www/package-lock.json` and are what actually gets installed during a build. Always commit both files.

| Dependency | Current Version | Source | Purpose |
|---|---|---|---|
| **react** | ^18.2.0 | npm | UI framework |
| **react-dom** | ^18.2.0 | npm | React rendering for web |
| **react-router-dom** | ^6.10.0 | npm | Client-side routing |
| **react-bootstrap** | ^2.7.4 | npm | Bootstrap component library |
| **bootstrap** | ^5.3.0-alpha3 | npm | CSS framework |
| **formik** | ^2.2.9 | npm | Form state management |
| **yup** | ^1.1.1 | npm | Schema validation for forms |
| **react-select** | ^5.7.5 | npm | Accessible select component |
| **zustand** | ^4.5.5 | npm | Lightweight state management |
| **react-i18next** | ^12.3.1 | npm | Internationalization (i18n) support |
| **i18next** | ^23.1.0 | npm | i18n framework |
| **i18next-browser-languagedetector** | ^7.0.2 | npm | Auto-detect user language |
| **react-beautiful-dnd** | ^13.1.1 | npm | Drag-and-drop functionality |
| **lodash-es** | ^4.17.21 | npm | Utility functions (ES modules) |
| **javascript-color-gradient** | ^2.4.4 | npm | Color interpolation for animations |
| **jsencrypt** | ^3.3.2 | npm | RSA encryption for secure communication |
| **@hello-pangea/color-picker** | ^3.2.2 | npm | Color picker UI component |

**Dev Dependencies** (`npm ci`/`npm run build` only):
- **vite** (^4.3.9) — Fast build bundler
- **@vitejs/plugin-react** (^4.0.0) — React plugin for Vite
- **typescript** (^5.3.3) — Type checking
- **eslint** + plugins — Code linting
- **prettier** (^3.3.3) — Code formatting
- **nodemon** (^2.0.22) — Development server auto-reload
- **protobufjs-cli** (^1.1.3) — Protocol Buffers code generation
- **sass** (^1.62.1) — SCSS compilation

The web build process is integrated into CMake. By default in development builds, `SKIP_WEBBUILD=TRUE` is set (this is also the CI default), which skips the web build step entirely. When `SKIP_WEBBUILD` is not set or is set to `FALSE`, CMake:
1. Runs `npm ci` (clean install with lock file)
2. Runs `npm run build` (generates TypeScript from Protobuf, bundles React)
3. Runs `makefsdata.js` (packages static files into firmware)

> **When is `SKIP_WEBBUILD=FALSE` needed?** Only when you need to rebuild the web configurator UI from source — for example, when making changes to `www/` React code. Standard firmware-only development always uses `SKIP_WEBBUILD=TRUE`. The pre-built web assets are provided as build artifacts and downloaded by CI separately (the `fsData` artifact step in `cmake.yml`).

## Updating Dependencies

### Pico SDK

#### Where to update
**File:** `CMakeLists.txt`

#### What changes
```cmake
set(sdkVersion 2.2.0)  # Change this to the new version
```

Also update the minimum version check in `CMakeLists.txt` if appropriate:
```cmake
if (PICO_SDK_VERSION_STRING VERSION_LESS "2.2.0")
  # Update the version here too
endif()
```

#### Testing checklist
1. **Clean build:** `cmake -B build -S . && cmake --build build`
2. **Test on target hardware:** Flash the firmware and verify all input/output methods work (USB gamepad, display, LEDs, etc.)
3. **Verify configuration persistence:** Save a test config with the web configurator and power-cycle the device; settings should load automatically
4. **Test backward compatibility:** Load a configuration file from a prior firmware version and verify no data loss

### Firmware Libraries (FetchContent)

#### Where to update
**File:** `CMakeLists.txt`, lines ~146-150 (ArduinoJson example):

```cmake
FetchContent_Declare(ArduinoJson
    GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
    GIT_TAG        v6.21.2  # Change this
)
FetchContent_MakeAvailable(ArduinoJson)
```

#### Approach
- Always use **semantic versioning tags** (e.g., `v6.21.2`) rather than branches or commit SHAs for stability
- Pin exact versions in `GIT_TAG` — floating tags like `latest` can introduce breaking changes
- Check the upstream repository's changelog before updating

#### Testing checklist
1. **Clean build:** `cmake -B build -S . && cmake --build build` (FetchContent re-downloads the specified tag)
2. **Compilation:** Ensure no linker errors or symbol mismatches
3. **Feature test:** Test the subsystem that uses the updated library (e.g., JSON serialization if updating ArduinoJson)
4. **Flash and validate:** Test on actual hardware

### Vendored Libraries (lib/ subdirectories)

Vendored libraries are managed as **git submodules** or **included source code**. Update procedures vary:

#### Submodule updates
If the library is a submodule:
```powershell
cd lib/<library-name>
git fetch origin
git checkout v<new-version>  # Or the target commit
cd ../..
git add lib/<library-name>
git commit -m "chore: update <library> to v<version>"
```

#### Source code updates
If the library is bundled source:
1. Check the upstream repository for the new version
2. Replace the files in `lib/<library-name>/` with the new version
3. Review `lib/<library-name>/CMakeLists.txt` for any interface changes (include paths, compiler flags, etc.)
4. Test the subsystem

#### Testing checklist
1. **Verify CMakeLists.txt compatibility:** Check if the updated library has a compatible CMake interface
2. **Compilation:** `cmake --build build 2>&1 | grep -E "error|warning"`
3. **Link verification:** Ensure all expected symbols are resolved
4. **Integration test:** Test the subsystem (e.g., OLED display, NeoPixels, flash storage) on hardware

### Web Configurator (npm)

#### Automated updates
```bash
cd www
npm update                    # Updates all packages to compatible versions per package.json semver ranges
npm ci                        # Installs exact versions from package-lock.json (reproducible)
```

#### Manual updates
To update a specific package:
```bash
cd www
npm install package-name@version   # E.g., npm install react@18.3.0
```

This updates `package.json` and `package-lock.json`.

#### Where to update
- `www/package.json` — Defines the version ranges (caret/tilde ranges)
- `www/package-lock.json` — Pinned exact versions (auto-generated by npm)

**Always commit both files** to ensure reproducible builds across environments.

#### Testing checklist
1. **Local dev server:** `npm start` in `www/` and test configurator UI in a browser
   - Check all pages load
   - Test form submission (save config)
   - Verify drag-and-drop (button remapping)
   - Test i18n language switching
2. **Build:** `npm run build` and verify no errors
3. **Lint:** `npm run lint` and ensure no new warnings
4. **Firmware integration:** Re-run the full CMake build (which runs `npm ci && npm run build`)
5. **On device:** Flash to hardware and access the web configurator via `http://<device-ip>/` or `http://192.168.7.1`

#### Breaking change detection
Watch for:
- Major version updates (e.g., `^5.0.0` → `^6.0.0`)
- Component API changes in React or React Bootstrap
- Protobuf changes (if updating protobufjs-cli)
- Build tool changes (Vite configuration)

## Compatibility Notes

### Version Matrix (Known Working Combinations)
| Pico SDK | ArduinoJson | TinyUSB | nanopb | Vite | React |
|---|---|---|---|---|---|
| 2.2.0 | v6.21.2 | (vendored) | (vendored) | ^4.3.9 | ^18.2.0 |

### Breaking change risks

**Pico SDK 2.x updates:**
- May introduce new CMake variables (usually backward compatible)
- Hardware API changes are rare but possible in patch versions
- Risk level: **Low** for patch updates, **Medium** for minor updates

**ArduinoJson 6.x → 7.x:**
- API breaking changes likely (namespace, class names)
- Risk level: **High** — requires code refactor

**React 18 → 19:**
- Hook behavior changes
- Concurrent rendering impacts
- Risk level: **Medium** — usually compatible with existing code, may need config updates

**TinyUSB (vendored):**
- Changes tracked through submodule commits
- Risk level: **Medium** — test all USB modes thoroughly (device, host, dual-role)

### Testing checklist after any dependency update

1. **Compilation:** No errors, warnings only if pre-existing
2. **Binary size:** Check `.uf2` file size hasn't grown unexpectedly (may indicate dependency bloat)
3. **Flash memory:** Test on-device configuration storage (save/load)
4. **USB communication:**
   - Connect to host PC and test gamepad detection
   - Test web configurator access
   - Test firmware update via web UI
5. **Display/LEDs (if applicable):** Test any add-on features that depend on the library
6. **Configuration persistence:** Save config, power-cycle, verify it loads
7. **Edge cases:** Test with multiple connected devices, long button presses, rapid config changes

## Version Lock Policy

### Why versions are pinned

1. **Reproducibility:** Anyone building GP2040-CE at the same commit gets identical binaries
2. **Stability:** Floating versions like `^6.0.0` can pull breaking changes unexpectedly
3. **Support:** Pinned versions make it easier to diagnose user issues ("you're on SDK 2.2.0, here's the fix for that")
4. **Backward compatibility:** Users upgrading firmware expect their configs to work unchanged

### How to propose a version bump

1. **Open an issue** describing:
   - Which dependency you want to update
   - Why (new features, bug fixes, security patch)
   - Any known breaking changes
   - Your testing plan

2. **Create a pull request** (branch: `deps/update-<lib-name>`) with:
   - Version pin updates in `CMakeLists.txt`, `pico_sdk_import.cmake`, or `www/package.json`
   - Updated `www/package-lock.json` (if npm changes)
   - Comprehensive test results (compile log, hardware tests)
   - Changelog entry documenting compatibility implications

3. **Code review** by the team (especially Edward for firmware, Winry for web UI, Riza for stability)

4. **Merge to develop branch first** — avoid direct main commits

5. **Test release candidate** — verify update didn't break anything in a full build

### Dependency deprecation

If a dependency becomes unmaintained or deprecated:
1. Assess replacement options
2. Plan migration (may take several releases)
3. Communicate deprecation to users in release notes
4. Provide a transition period before removal (at least 2 minor releases)

## See Also

- [Building GP2040-CE](../README.md) — Build instructions
- [Protobuf Configuration Protocol](./protobuf-config.md) — Details on configuration serialization (nanopb)
- [Web Configurator](./web-configurator.md) — UI development guide
- [Hardware Support](../hardware/) — Board-specific dependencies
