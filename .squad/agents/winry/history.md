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

### 2026-03-29T20:00: Hide Bluetooth Classic from Web Configurator

Removed Bluetooth Classic (INPUT_MODE_BLUETOOTH = 17) from web configurator UI at Fortinbra's request. Only BLE (INPUT_MODE_BLE = 18) should be visible during current BLE development phase.

**Files Modified:**
- `www/src/Pages/SettingsPage.jsx` — Removed Bluetooth Classic entries from INPUT_MODES array (line 205) and INPUT_BOOT_MODES array (line 252)

**Pattern:**
- Proto enum (enums.proto) and localization strings (SettingsPage locale) remain intact — only UI visibility affected
- This is a temporary UI-only change — Bluetooth Classic can be re-enabled by adding the entries back to the two arrays
- BLE (value: 18) entries remain untouched in both arrays

**Status:** Applied on `feature/ble-hid` branch. No build needed — UI-only change.


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

### 2026-03-29T17:52: Edward's BLE Audit — Cross-Agent Learning (Input Mode Pattern)

Edward's BLE HID audit identified a critical pattern for future InputMode additions:

**Rule:** Any new InputMode enum value added to `proto/enums.proto` MUST have a corresponding `#define INPUT_MODE_<NAME>_NAME "<Display>"` macro in `headers/display/ui/screens/MainMenuScreen.h`.

**Why:** MainMenuScreen.h uses macro expansion (InputMode_VALUELIST with INPUT_MODE_ENTRIES pattern) to generate the input mode menu. Missing display name macro causes compilation failure.

**Example:** When INPUT_MODE_BLE = 18 was added to enums.proto, INPUT_MODE_BLE_NAME "BLE" was missing from MainMenuScreen.h. This is now documented as a hard rule.

**Action for Winry:** If web configurator adds new input modes in future (e.g., future wireless modes), confirm firmware has corresponding display name macros before integration.

### 2026-03-29T18:30: BLE Web UI Debug Investigation

User reported BLE web UI "looks identical to Bluetooth Classic" — BLE was not appearing as a distinct input mode option.

**Root Cause:**
- `INPUT_MODE_BLE = 18` was never added to `proto/enums.proto` (only Bluetooth Classic = 17 exists)
- `INPUT_MODE_BLE_NAME "BLE"` macro missing from `MainMenuScreen.h`
- No BLE entry in `SettingsPage.jsx` INPUT_MODES or INPUT_BOOT_MODES arrays
- No BLE localization label in `SettingsPage.jsx` locale file

**Architecture Confirmed:**
- `enums.ts` is auto-generated from proto via `npm run build-proto` — always matches proto source
- Web UI reads INPUT_MODES array to render dropdown options (each has labelKey, value, group)
- Localization labels are in `www/src/Locales/en/SettingsPage.jsx` under 'input-mode-options'
- Firmware display menu macros (INPUT_MODE_*_NAME) must match proto enum values or compilation fails

**Required Fix (5 files):**
1. Add `INPUT_MODE_BLE = 18;` to `proto/enums.proto`
2. Add `#define INPUT_MODE_BLE_NAME "BLE"` to `MainMenuScreen.h`
3. Add BLE entry to INPUT_MODES array in `SettingsPage.jsx`
4. Add BLE entry to INPUT_BOOT_MODES array in `SettingsPage.jsx`
5. Add `ble: 'BLE'` to localization in `www/src/Locales/en/SettingsPage.jsx`
6. Rebuild: `npm run build-proto && npm run build`

**Pattern for Future Wireless Modes:**
Adding any new InputMode requires coordination across proto definition, firmware display macros, web UI arrays, and localization strings. Missing any piece causes UI display failure or compilation errors.

Full findings documented in `.squad/decisions/inbox/winry-ble-ui-debug.md`.

### 2026-03-29T19:00: BLE Web UI Fix Implementation

Applied all 5 identified fixes to add BLE as a distinct input mode (INPUT_MODE_BLE = 18) in the web configurator and firmware display system:

**Files Modified:**
1. `proto/enums.proto` — Added `INPUT_MODE_BLE = 18;` to InputMode enum
2. `headers/display/ui/screens/MainMenuScreen.h` — Added `#define INPUT_MODE_BLE_NAME "BLE"` macro
3. `www/src/Pages/SettingsPage.jsx` — Added BLE entry to INPUT_MODES array (line 206)
4. `www/src/Pages/SettingsPage.jsx` — Added BLE entry to INPUT_BOOT_MODES array (line 254)
5. `www/src/Locales/en/SettingsPage.jsx` — Added `ble: 'BLE'` localization label (line 29)

**Pattern Reinforced:**
All new InputMode additions require 5-part coordination: proto enum value, firmware display macro, web UI input modes array, boot modes array, and localization string. Missing any piece causes UI display failures or compilation errors.

**Status:** Applied on `feature/ble-hid` branch. Awaiting Edward's pairing fix before final npm build and integration testing.

