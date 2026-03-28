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
