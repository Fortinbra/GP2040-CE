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

### Copilot Instructions (2025-03-28)
- Created `.github/copilot-instructions.md` on branch `docs/copilot-instructions` (commit `6308f5c0`)
- Documented comprehensive guidance for GitHub Copilot contributions
- Locked Pico SDK version to 2.1.1 as standard
- **Critical branching policy:** Never commit directly to main or upstream; always use feature branches
- Indentation standard: 4 spaces only, never tabs — enforced across all file types
- Covers code style (C/C++, CMake), platform config, build system, project structure, git workflow, documentation, PR guidelines, testing, performance, and hardware constraints
