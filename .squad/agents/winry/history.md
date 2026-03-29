## Project Context (Day 1)

**Project:** GP2040-CE — RP2040 firmware for gamepads and game controllers  
**Stack:** C/C++, CMake, Raspberry Pi Pico SDK 2.1.1, Protobuf, React (web configurator)  
**Goal:** Create feature documentation for the firmware  
**User:** Fortinbra  
**Repo root:** C:\ws\GP2040-CE

**Key directories:**
- src/ — firmware C++ implementation
- headers/ — data structures and interfaces
- lib/ — third-party libraries
- configs/ — board configurations (default: Pico)
- proto/ — protobuf config protocol definitions
- www/ — React web configurator
- docs/ — existing documentation

## Learnings

### 2026-03-28T024429: Round-3 Doc Fixes

Assigned to fix Riza's round-3 rejection issues (Mustang was locked out). Two fixes applied:
1. `cmake_minimum_required` version corrected to 3.10+ in both `rp2350-support.md` and `dependency-updates.md` — matched against actual `CMakeLists.txt` value
2. Second stale `pico_sdk_import.cmake` path reference corrected in `rp2350-support.md`

Commit `a23b72ee`. Both fixes survived all subsequent review rounds (rounds 4–6) without regression.

**Key file:** `pico_sdk_import.cmake` lives at repo root; build docs should reference it as `pico_sdk_import.cmake` (not a subdirectory path).

### 2026-03-28: Bluetooth Web UI Implementation — Phase 1

Implemented Bluetooth configuration UI in the React web configurator. Key findings about the architecture:

**Component Patterns:**
- Addons follow a consistent pattern: exported `{name}Scheme` (yup validation), `{name}State` (default values), and a React component
- Each addon is a Section component with a toggle switch at the bottom (reverse positioned)
- Addons are registered in AddonsConfigPage by importing and adding to ADDONS array, schema, and DEFAULT_VALUES
- FormControl, FormSelect, and FormCheck (from react-bootstrap) are the primary input components

**API Integration:**
- WebApi.getAddonsOptions() fetches all addon data from `/api/getAddonsOptions`
- WebApi.setAddonsOptions() saves changes to `/api/setAddonsOptions`
- Data is diff'd before saving (only changed fields are sent)
- Formik handles form state and validation

**Input Mode Selection:**
- INPUT_MODES and INPUT_BOOT_MODES arrays in SettingsPage.jsx define available modes
- Modes are grouped ('primary' or 'mini') and can have requirements (e.g., 'usb' peripheral)
- Localization uses labelKey pointing to 'input-mode-options' in locale files

**Build System:**
- `npm run build-proto` generates TypeScript types from proto files
- `npm run build` runs build-proto, vite build, and makefsdata
- enums.ts is auto-generated in src_gen/ — never edit manually
- Build output goes to www/build/

**Files Modified:**
- Created: `www/src/Addons/Bluetooth.tsx` — Bluetooth addon component
- Modified: `www/src/Pages/AddonsConfigPage.tsx` — registered Bluetooth addon
- Modified: `www/src/Pages/SettingsPage.jsx` — added Bluetooth to INPUT_MODES and INPUT_BOOT_MODES
- Modified: `www/src/Locales/en/AddonsConfig.jsx` — added Bluetooth localization strings
- Modified: `www/src/Locales/en/SettingsPage.jsx` — added Bluetooth input mode label

**Bluetooth UI Features:**
- Enable/disable toggle for Bluetooth addon
- Pairing mode toggle (makes controller discoverable)
- Read-only paired device name display
- Clear pairing button (sets bondedDeviceAddr and bondedDeviceName to empty)
- Info alert explaining Bluetooth requires CYW43-based boards

### 2026-03-29T02:00: Bluetooth Web UI Implementation — Phase 2 Complete

Web config UI delivered for Phase 2 integration:
- Bluetooth output mode selection in Settings (INPUT_MODE_BLUETOOTH = 17)
- Bluetooth addon configuration panel with all required controls
- Clean npm build verified
- Awaiting Riza code review

**Status:** Phase 2 complete — in review cycle

