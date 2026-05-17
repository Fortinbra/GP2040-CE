# Pimoroni SDK Board Definition Option for Pico LiPo 2 XL W

**Status:** Recommended implementation strategy (documentation-only; no code changes in this pass)  
**Scope type:** Build/configuration capability expansion (default behavior preserved)  
**Target board:** Pimoroni Pico LiPo 2 XL W (`GP2040_BOARDCONFIG=PimoroniPicoLipo2XLW`)

---

## Evidence Baseline (Current State)

Repository-grounded facts:

- `configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake` currently sets `PICO_BOARD=pico2_w` and `PICO_PLATFORM=rp2350-arm-s`.
- Root `CMakeLists.txt` loads `GP2040_BOARDCONFIG` first, sets `PICO_BOARD_HEADER_DIRS` to `configs/${GP2040_BOARDCONFIG}`, then includes the board overlay cmake before SDK init.
- Build and runtime metadata expose `GP2040_BOARDCONFIG` as a compile definition (used in config defaults and Stats UI), while SDK board identity remains determined by `PICO_BOARD`.

External board-definition reference provided for this feature:

- Pimoroni board header: [github.com/pimoroni/pico-lipo/.../pimoroni_pico_lipo2xl_w.h](https://github.com/pimoroni/pico-lipo/blob/main/boards/pimoroni_pico_lipo2xl_w/pimoroni_pico_lipo2xl_w.h)
- Raw header reviewed for macro evidence: [raw.githubusercontent.com/pimoroni/pico-lipo/.../pimoroni_pico_lipo2xl_w.h](https://raw.githubusercontent.com/pimoroni/pico-lipo/main/boards/pimoroni_pico_lipo2xl_w/pimoroni_pico_lipo2xl_w.h)

Verified external macro examples from the Pimoroni header (not yet consumed by GP2040-CE build flow):

- `pico_cmake_set PICO_PLATFORM=rp2350`
- `pico_cmake_set PICO_CYW43_SUPPORTED = 1`
- `PIMORONI_PICO_LIPO2_RP2350`
- `CYW43_DEFAULT_PIN_WL_REG_ON=23`, `CYW43_DEFAULT_PIN_WL_DATA_OUT=24`, `CYW43_DEFAULT_PIN_WL_CLOCK=29`, `CYW43_DEFAULT_PIN_WL_CS=25`
- `CYW43_WL_GPIO_VBUS_PIN=2`
- Default peripheral pin macros for UART/I2C/SPI and `PICO_FLASH_SIZE_BYTES` default to 16 MiB (with comment showing a 4 MiB cmake default).

Open question from evidence: `NUM_BANK0_GPIOS` was not observed in the Pimoroni header excerpt and may be inherited via SDK includes rather than declared directly in this file.

---

## Audit Findings (Repository, May 2026)

This section captures implementation-relevant mismatches and current-state behavior verified in the repository.

### A. Selection and Mapping Surfaces

- Root selection flow is currently split across environment and overlay files:
  - `CMakeLists.txt` resolves `GP2040_BOARDCONFIG`, includes `configs/<name>/<name>.cmake`, and then initializes the SDK.
  - Board overlays (`configs/*/*.cmake`) set `PICO_BOARD` and `PICO_PLATFORM`.
- Missing-board behavior is currently soft-fail: the `FATAL_ERROR` for missing `GP2040_BOARDCONFIG` file is commented out in `CMakeLists.txt`.
- `configs/<board>/BoardConfig.h` is selected through include paths (`PICO_BOARD_HEADER_DIRS` and target include dirs), not through a global shared `headers/BoardConfig.h` file.

### B. Runtime Macro Consumption Contract

- Board pin defaults (`GPIO_PIN_xx`) are consumed in `src/config_utils.cpp`.
- Default macro declarations in `src/config_utils.cpp` currently exist for `GPIO_PIN_00` through `GPIO_PIN_29` only.
- `boardConfig` default array initialization in `src/config_utils.cpp` is also explicit `GPIO_PIN_00..GPIO_PIN_29`, even though runtime loops use `NUM_BANK0_GPIOS`.
- Battery macros are consumed only by BLE path today:
  - `BATTERY_ADC_GPIO` and `BATTERY_ADC_CHANNEL` are read in `src/BLEHIDManager.cpp`.
  - If `BATTERY_ADC_GPIO` is missing, BLE battery reports default to 100%.
- `BATTERY_VBUS_GPIO` currently has no verified runtime consumer in `src/`.

### C. Verified Mismatch Requiring Reconciliation

- `configs/PimoroniPicoLipo2XLW/BoardConfig.h` documents and defines battery/VBUS on RP GPIOs (`BATTERY_ADC_GPIO=29`, `BATTERY_VBUS_GPIO=24`).
- `docs/development/pimoroni-pico-lipo-2xl-w-support.md` and `docs/development/bluetooth-support.md` document Pimoroni behavior as:
  - battery sense on GP43 / ADC3
  - VBUS via `CYW43_WL_GPIO_VBUS_PIN` through `cyw43_arch_gpio_get(...)`
- Pimoroni external board header indicates CYW43 VBUS GPIO path (`CYW43_WL_GPIO_VBUS_PIN=2`) and CYW43 SPI pin assignments on 23/24/25/29.

Implication: the repository needs one explicit, test-backed contract for this board before enabling external-board auto-matching as a stable feature.

---

## Recommended Primary Mapping Mechanism

### Naming

- Keep existing board identity as the canonical GP2040 selector: `GP2040_BOARDCONFIG`.
- Introduce one new source-selector variable:
  - `GP2040_BOARD_DEFINITION_SOURCE` with allowed values: `sdk`, `external`, `auto`.
- Optional explicit external board target override:
  - `GP2040_EXTERNAL_PICO_BOARD` (example: `pimoroni_pico_lipo2xl_w`).

### Lookup

For each GP2040 board config, define a mapping record with:

- `default_sdk_board` (current path, e.g. `pico2_w`)
- `external_board_candidates` (ordered list, e.g. `pimoroni_pico_lipo2xl_w`)
- `required_external_header_hint` (expected board header file stem)

Initial location recommendation: add a small mapping table adjacent to each board overlay (or a centralized table keyed by `GP2040_BOARDCONFIG`).

### Precedence

Resolution order should be deterministic:

1. If `PICO_BOARD` is explicitly provided by environment/user, use it unchanged (highest precedence, current behavior compatibility).
2. Else if `GP2040_BOARD_DEFINITION_SOURCE=external` and `GP2040_EXTERNAL_PICO_BOARD` is set, use that exact external board target.
3. Else if `GP2040_BOARD_DEFINITION_SOURCE=external`, resolve first valid candidate from `external_board_candidates`.
4. Else if `GP2040_BOARD_DEFINITION_SOURCE=auto`, attempt external candidate resolution first.
5. Else use `default_sdk_board` (`sdk` path and fallback default).

Conflict rule for diagnosability:

- If `PICO_BOARD` is set while any external selector (`GP2040_BOARD_DEFINITION_SOURCE=external|auto` or `GP2040_EXTERNAL_PICO_BOARD`) is also set, configure output should print one explicit precedence notice that `PICO_BOARD` won and external selection was bypassed.

### Fallback

- `external` mode: fail configure with a clear error if no external board candidate resolves.
- `auto` mode: warn once, then fall back to `default_sdk_board`.
- `sdk` mode: no external lookup; always use current behavior.

This preserves existing builds while making external board usage explicit and inspectable.

---

## Data Contract: GP2040 Board Config <-> External Board Headers

Contract boundary:

- GP2040 overlay remains owner of GP2040-specific behavior (button mapping, addon defaults, user-visible board label).
- External board header remains owner of SDK-level board/chip definitions (platform macro intent, CYW43 board pins, default peripheral pins, flash-size defaults).

Required contract fields for this board family:

1. `resolved_pico_board` (string): final board target passed to SDK.
2. `board_definition_source` (`sdk|external|auto-resolved-sdk|auto-resolved-external`).
3. `resolved_board_header` (path or inferred board stem for diagnostics).
4. `macro_expectations` (validation set):
   - CYW43 pin macros expected by wireless stack path.
   - `CYW43_WL_GPIO_VBUS_PIN` expectation for VBUS-read strategy.
   - Platform/chip indicator macros (at minimum, RP2350-family evidence).
5. `pin_ownership_rules`:
   - CYW43 reserved pins are never assignable by GP2040 button maps.
   - Battery/VBUS mapping must be explicitly documented and validated for this board.

Validation mechanism recommendation:

- Add a configure-time macro check target (preprocess/compile probe) that records pass/fail for required macros and writes a concise configure summary.
- Treat missing required macros as hard errors in `external` mode and warnings in `auto` mode when falling back to `sdk`.

---

## CMake and Workflow Integration Points

### CMake Integration

Recommended insertion points in existing flow:

1. Resolve `GP2040_BOARDCONFIG` (already present).
2. Before `include(pico_sdk_import.cmake)`, resolve board-definition source and final `PICO_BOARD` according to the precedence above.
3. Set/append board definition search path only when `external` or `auto` uses external.
4. Keep Ninja-only build requirement unchanged.
5. Emit configure summary lines:
   - selected GP2040 board config
   - selected board-definition source
   - final `PICO_BOARD`
   - external header root (if used)

### VS Code Task Integration

Recommended task model:

- Keep existing tasks unchanged.
- Add explicit Pimoroni external tasks (configure + clean configure), for example:
  - `Configure CMake (Pimoroni Pico LiPo 2 XL W External Board Def)`
  - `Clean and Configure (Pimoroni Pico LiPo 2 XL W External Board Def)`
- Each new task should set:
  - `GP2040_BOARDCONFIG=PimoroniPicoLipo2XLW`
  - `GP2040_BOARD_DEFINITION_SOURCE=external`
  - external board search path variable (name to be finalized in implementation)
  - optional `GP2040_EXTERNAL_PICO_BOARD=pimoroni_pico_lipo2xl_w` when selecting a non-default candidate
  - `-G Ninja` path consistency (or task equivalent)

Task authoring constraint:

- External-mode tasks should not set `PICO_BOARD` directly. Setting `PICO_BOARD` in task environment bypasses external resolution by design (precedence rule #1).

### CI Integration

Current CI matrix compiles by `GP2040_BOARDCONFIG` and does not include `PimoroniPicoLipo2XLW` in the default matrix list.

Recommended CI additions:

- Add one non-blocking job first for `PimoroniPicoLipo2XLW` in `sdk` mode as baseline visibility.
- Add second non-blocking job for `external` mode once dependency acquisition is deterministic.
- Promote to blocking only after repeated green runs and documented version pinning.

### Acceptance Test and Build Validation Plan

Use this plan as the implementation definition-of-done for board-definition matching.

1. Clean configure/build baseline (`sdk` source)
- Run a fresh configure and build using current default Pimoroni path.
- Confirm configure summary reports expected source mode and resolved `PICO_BOARD`.

2. Clean configure/build external opt-in (`external` source)
- Run a fresh configure and build with external board definition enabled.
- Confirm resolved board header identity in configure diagnostics.
- Confirm no fallback occurred in strict `external` mode.

3. Auto mode fallback behavior (`auto` source)
- Simulate missing external definition path.
- Confirm warning + deterministic fallback to `default_sdk_board`.
- Confirm build still succeeds and summary reflects fallback mode.

4. Negative mismatch tests
- Force intentional mismatch between `GP2040_BOARDCONFIG` and selected external board.
- Confirm configure fails with actionable diagnostics naming both identifiers.
- Force missing required macro expectation and confirm strict-mode failure.

5. Pin-contract validation tests
- Validate CYW43 reserved pins (23/24/25/29) are reported as reserved for CYW43 targets.
- Validate board overlay does not assign reserved pins to user mappings when strict checks are enabled.
- Validate battery signal contract check reports which source is active (RP GPIO ADC path vs CYW43 VBUS path).

6. Existing-config regression matrix (minimum)
- Clean configure/build for representative existing configs:
  - `Pico` (RP2040 baseline)
  - `PicoW` (CYW43 baseline)
  - `Pico2` (RP2350A baseline)
  - `Pico2W` (RP2350A + CYW43 baseline)
  - `SparkFunProMicroRP2350` (RP2350B baseline)
- Confirm no behavior change when source mode remains default (`sdk`).

---

## Risk Controls

### Pin Mismatch Controls

- Generate a reserved-pin report from resolved CYW43 pin macros and compare against board overlay assumptions.
- Hard-fail when overlay assigns any reserved CYW43 pins to user inputs.
- Keep battery and VBUS mapping checks explicit and board-specific.

### Macro Drift Controls

- Record expected macro signature for each supported external board target.
- On configure, compare current macro probe result against known signature and show drift warnings.
- Require explicit update of signature docs when intentional upstream changes occur.

### SDK Compatibility Controls

- Pin and document tested tuple: GP2040 Pico SDK version + external board-definition source commit/tag.
- In external mode, error if minimum compatible tuple is not met (or mark as unsupported experimental mode).

### Build-Path Clarity Controls

- Always print source mode and resolved board target during configure.
- Embed source mode and resolved board target into build metadata for post-build diagnostics.

---

## Migration Plan from Current Behavior

Phase 0 (now, documentation):

- Keep default behavior unchanged (`PICO_BOARD=pico2_w` via overlay).
- Publish this strategy as the recommended implementation path.

Phase 1 (internal plumbing, no behavior change):

- Add source-selector variable parsing and configure summary output.
- Implement mapping table with Pimoroni entry but default `sdk` path.

Phase 2 (opt-in external support):

- Enable `external` mode for `PimoroniPicoLipo2XLW` behind explicit environment/task selection.
- Add macro probe and reserved-pin validation gates.

Phase 3 (stabilization):

- Add CI coverage for both `sdk` and `external` modes.
- Decide whether long-term default should remain `sdk` or move to `auto`/`external`.

Rollback plan:

- Any external-mode regression can be isolated by setting `GP2040_BOARD_DEFINITION_SOURCE=sdk` and rebuilding without changing `GP2040_BOARDCONFIG`.

---

## Open Questions (Do Not Treat as Facts)

1. What variable name should be standardized for external board-header roots in this repository's CMake flow?
2. Should external support be delivered via submodule, documented prerequisite path, package fetch, or a hybrid model?
3. What exact compatibility tuple (Pico SDK 2.2.0 + Pimoroni repo commit/tag) should be considered supported for first release?
4. Is the Pico LiPo 2 XL W battery sense canonical mapping GPIO29/ADC3, GP43/ADC3, or mode-dependent by board revision? Evidence is currently mixed and requires hardware-verified reconciliation before hard enforcement.
5. Should CI include `PimoroniPicoLipo2XLW` in the default matrix, or only in dedicated optional jobs initially?

---

## Cross-References

- `docs/development/pimoroni-pico-lipo-2xl-w-support.md`
- `docs/development/rp2350-support.md`
- `docs/development/bluetooth-support.md`
- `configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake`
- `configs/PimoroniPicoLipo2XLW/BoardConfig.h`
- `CMakeLists.txt`
