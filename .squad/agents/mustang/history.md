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

### 2026-03-28T024429: Round-2 Doc Fixes

Fixed 3 issues flagged by Riza in review round 2: wrong file reference in `rp2350-support.md`, a stale note carried over from Hughes's original draft, and an incorrect year in a document header. Commit `1c910869`. Note: Mustang is locked out from further revisions of these docs during this cycle (reviewer lockout protocol — Winry assigned for round-3 fixes).

### 2026-03-28T024429: Copilot Instructions "Maintained By" (cross-agent update from Scribe)

The `copilot-instructions.md` file that Mustang created originally included "Roy Mustang, Project Lead" in the Maintained-by field. Riza flagged this in round 5 as an internal AI agent name in a public document. Fortinbra corrected it to "GP2040-CE core team". **Pattern:** Never include Squad agent names in public-facing documentation — attribute to the project team instead.

### Copilot Instructions (2025-03-28)
- Created `.github/copilot-instructions.md` on branch `docs/copilot-instructions` (commit `6308f5c0`)
- Documented comprehensive guidance for GitHub Copilot contributions
- Locked Pico SDK version to 2.1.1 as standard
- **Critical branching policy:** Never commit directly to main or upstream; always use feature branches
- Indentation standard: 4 spaces only, never tabs — enforced across all file types
- Covers code style (C/C++, CMake), platform config, build system, project structure, git workflow, documentation, PR guidelines, testing, performance, and hardware constraints
