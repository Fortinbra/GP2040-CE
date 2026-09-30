# Warning-Free Firmware and WebUI Builds

**Status:** Living draft; remediation requires review and approval.

**Date:** 2026-09-30

**Related:** [Roadmap to 1.0](roadmap-to-1.0.md),
[parity qualification](controller-parity-qualification.md).

## Goal

Produce reproducible firmware and embedded WebUI artifacts with zero errors and
zero warnings throughout supported build pipelines. Successful exit status alone
is insufficient. Fix the causes rather than hiding diagnostics, removing useful
checks, or reducing the supported feature set.

This document inventories current diagnostics and defines remediation work.
Writing it does not implement the fixes or certify the firmware as warning-free.

## Scope and Definition of Clean

The two build systems are CMake/Ninja for firmware and npm/Vite for the WebUI.
Include their prerequisites and generated artifacts: dependency installation,
CMake configure, C/C++/assembly compilation, linking, protobuf/GATT generation,
TypeScript generation, SCSS processing, bundling, and embedded filesystem output.
Also require explicit WebUI lint/type checks: bundling is not a substitute for them.

- Every required command exits successfully, and no compiler, linker, generator,
  package-manager, bundler, Sass, lint, or type diagnostic remains unresolved.
- Normal progress, memory-usage summaries, and informational funding/update notices
  are not warnings. Security advisories are tracked separately from compiler
  diagnostics but must not be hidden to create a clean installation log.
- Existing global warning suppressions and ignored files require review; absence
  of output under a disabled check does not establish correctness.
- Scope includes project, generated, vendored, and SDK code used by supported
  configurations. Third-party ownership changes the repair method, not the goal.
- A temporary documented baseline can prevent new debt while repairs proceed,
  but remaining exceptions mean the zero-warning goal is not yet achieved.

## Verified Build-Path Gaps

| ID | Evidence | Required change |
| --- | --- | --- |
| CLEAN01 | [CMakeLists.txt](../../CMakeLists.txt) applies `-Wall` and `-Wtype-limits`, but globally disables format and unused-function diagnostics. Debug adds separate diagnostic flags. | Inventory Release and Debug warnings, repair root causes, and review suppressions at the narrowest owning target without reducing useful checks. |
| CLEAN02 | [WebUI scripts](../../www/package.json) build protobuf types, run Vite, then make filesystem data; lint is a separate command and there is no explicit typecheck script. | Make lint and a suitable TypeScript check explicit required gates, covering intended hand-written and generated code without mistaking transpilation for type safety. |
| CLEAN03 | CMake runs npm installation/build only inside successful Node/npm discovery and path checks. | Fail clearly when web-inclusive prerequisites are missing instead of silently allowing stale or absent embedded assets. Preserve explicit firmware-only build behavior. |
| CLEAN04 | Dependency installation, generators, and third-party compiles have independent diagnostic surfaces. | Capture their full output and exit status; assign diagnostics to source owners and compatible dependency upgrades. |
| CLEAN05 | An incremental build may compile nothing, and Standard Pico omits wireless/RP2350 paths. | Require fresh configure plus cleaned outputs and a justified target/mode/toolchain matrix before claiming clean firmware. |
| CLEAN06 | Existing quality scripts are not equivalent to an enforced end-to-end release gate. | Add portable local/CI enforcement after repair, preserving logs and failing on reintroduced diagnostics. |

## Diagnostic Baseline

Measured on Windows on 2026-09-30 at source commit `e1ab3d9f623c`, before any
remediation. Firmware and dependency source files were not edited during the audit.
The package-lock SHA-256 was
`9FFDBDE29BBBB22A04A07AB7A4297D9DDDE00B9D2D363A1E67081A7B469A4F3F`.

Environment: SDK 2.3.1, GNU Arm 15.2.1, CMake 3.31.5, Ninja 1.12.1, Node 23.4.0,
npm 11.12.0. Installed frontend versions include Vite 4.5.14, Sass 1.98.0,
Bootstrap 5.3.8, TypeScript 5.9.3, ESLint 8.57.1, React 18.3.1,
`@types/react` 19.2.14, react-i18next 12.3.1, and i18next 23.16.8.
These are measured versions, not a recommendation to standardize on this Node
release. The existing Node workflow uses 20.x; host/toolchain alignment is required.

| Command / configuration | Outcome | Observed diagnostics |
| --- | --- | --- |
| Fresh Pico Release configure with WebUI, then cleaned full Ninja build | Exit 0; UF2 produced | 18 compiler warnings: 9 switch, 8 signedness, 1 unused variable; also 3 Python generator warning emissions. |
| Fresh Pico2W Release configure, then cleaned full Ninja build | Exit 0; UF2 produced | 17 compiler warnings: 8 switch, 8 signedness, 1 unused variable; also 3 Python generator warning emissions. No warning lines observed in its firmware-only configure log. |
| Fresh Pico Debug configure, then cleaned full Ninja build | Exit 0; UF2 produced | 57 compiler warnings: 26 stack usage, 10 alignment, 9 switch, 8 signedness, 3 fallthrough, 1 unused variable; also 3 Python generator warning emissions. |
| `npm ci` during Pico web-inclusive configure | Exit 0 | 7 package-deprecation warning lines and a 33-vulnerability summary. |
| `npm run build` during Pico web-inclusive configure | Exit 0; embedded assets generated | Sass legacy API/import/function deprecations, a summary omitting 312 repetitive deprecations, and the Vite chunk-size warning above 500 kB after minification. |
| `npm run lint` | Exit 1 | 225 errors and 51 warnings; also an unsupported TypeScript/parser-version warning outside the JSON diagnostic counts. |
| Local TypeScript compiler with `--project www/tsconfig.json --noEmit --pretty false` | Exit 2 | 166 errors: 158 in application files and 8 in dependency declarations. |
| `npm --prefix www audit --json` | Exit 1 | 33 reported vulnerabilities: 2 low, 8 moderate, 21 high, 2 critical. These are npm audit counts, not 33 independently proven exploitable firmware defects. |

Compiler counts are emitted occurrences in each build, not unique bugs and not
counts to add together across configurations. Python warnings are counted
separately. Sass abbreviates repeated output, so displayed warning lines are not
a complete occurrence count. No compile/link error occurred in these samples;
the failing quality checks still prevent a clean-build claim.

Raw logs are local, ignored artifacts under `build/board-validation/`:
`Pico-configure.log`, `Pico-build.log`, `Pico2W-configure.log`,
`Pico2W-build.log`, `Pico-Debug-configure.log`, `Pico-Debug-build.log`,
`web-lint.log`, `web-lint.json`, `web-types.log`, and `web-audit.json`.
They are not distributed with this document and will be replaced by later runs.
The audit left `build/` configured for Standard Pico Debug with web building skipped;
the embedded web assets came from the preceding successful web-inclusive build.

The initial attempt to pass formatter options through the local npm PowerShell
wrapper failed due to argument forwarding. That invocation is excluded from the
baseline: the unchanged `npm run lint` script and a direct equivalent ESLint
invocation both subsequently exited 1 with valid diagnostic results.

## Concrete Remediation Backlog

All rows below are open. Priority is correctness and build blockers first, then
warning removal and durable enforcement, not merely reducing diagnostic counts.

| ID | Owner / observed problem | Required repair and discriminating validation |
| --- | --- | --- |
| FIX01 | [P5GeneralAuthUSBListener](../../src/drivers/p5general/P5GeneralAuthUSBListener.cpp#L43): four unhandled idle/wait states. | Specify explicit intended handling of each state and test transitions/timeouts. Do not hide missing states behind an unconditional default. This is maintenance of existing behavior, not the deferred new authentication feature. |
| FIX02 | [Legacy configuration](../../src/config_legacy.cpp#L546): R16/R16B enum members reported missing while case values 33/37 are outside the legacy enums. | Resolve legacy/current enum-name ambiguity and preserve historical serialized numeric meanings. Test legacy imports with affected layouts and invalid values; do not renumber schemas to silence warnings. |
| FIX03 | [Button layout display](../../src/display/ui/screens/ButtonLayoutScreen.cpp#L278): BLE enum unhandled in the non-wireless Pico build. | Define the intended unavailable-mode/display behavior under feature guards and test both wireless and non-wireless builds. |
| FIX04 | [Analog inputs](../../src/addons/analog.cpp#L24), [Hall triggers](../../src/addons/he_trigger.cpp#L24), [chase effect](../../src/animationstation/effects/chase.cpp), and [Web API](../../src/webconfig.cpp#L3251): eight signed/unsigned comparisons. | Validate negative/unassigned pin sentinels before unsigned bounds checks; use correct domain types for indices/counts. Test minimum/maximum/invalid pins and effect boundaries on both architectures rather than applying blanket casts. |
| FIX05 | [I2C ADC addon](../../src/addons/i2canalog1115.cpp#L46): unused local variable. | Remove the unused binding only after checking whether its initializer has required effects; preserve acquisition behavior. |
| FIX06 | Debug alignment warnings in [security helper](../../src/drivers/shared/xsm3/usbdsec.c), [PS4 driver](../../src/drivers/ps4/PS4Driver.cpp#L895), [event manager](../../src/eventmanager.cpp#L28), and [config utilities](../../src/config_utils.cpp). | Replace unsafe typed views of byte buffers with alignment-safe parsing/copying or prove and express alignment at allocation. Preserve endianness, packet layout, storage format, and lifetimes; test deliberately misaligned inputs. |
| FIX07 | Debug stack warnings across UI/config/Web API, drivers, [PIO USB](../../lib/pico_pio_usb/src/pio_usb_host.c), and SDK lwIP. | Use emitted stack-usage data plus call-depth/interrupt analysis. Refactor large temporaries and unbounded allocation with explicit lifetime/concurrency ownership; qualify memory and stack guards on hardware. Do not simply increase the threshold or move every buffer to shared static storage. |
| FIX08 | [Chase effect](../../src/animationstation/effects/chase.cpp#L329): three implicit fallthrough warnings. | Determine intended animation transitions; use an explicit supported fallthrough annotation only for intentional behavior, otherwise correct control flow. Test direction/effect transitions. |
| FIX09 | [nanopb generator package](../../lib/nanopb/generator/proto/__init__.py) uses deprecated `pkg_resources`; [generator](../../lib/nanopb/generator/nanopb_generator.py) uses deprecated `reflection.MakeClass()`. | Use the existing nanopb migration workstream to align generator/runtime/protobuf/setuptools versions or carry a reviewed minimal upstream fix. Regenerate and test configuration serialization/migrations; do not edit generated C or silence Python warnings. |
| FIX10 | Frontend tool/type dependency skew. | Align TypeScript with its ESLint parser's supported range; align React 18 runtime/types and react-i18next/i18next declarations using compatible reviewed versions. Verify lockfile installation, lint, types, and runtime before considering unrelated major upgrades. |
| FIX11 | 225 ESLint errors and 51 Fast Refresh warnings across hand-written source. | Remove genuine unused bindings, fix declarations/JSX/global references, localize literals, and separate component/helper exports where needed. Preserve addon registration and current module conventions; justify any necessary export-boundary change. Run lint with zero warning tolerance plus affected UI tests. |
| FIX12 | 158 application TypeScript errors plus 8 dependency declaration errors. | Type shared context/state/API boundaries, narrow optional data, correct callback/index/prop types, and repair declaration compatibility. Fix shared causes before leaf casts; keep strict checking and library checks enabled. |
| FIX13 | Sass legacy JS API and deprecated imports/functions in [styles](../../www/src/index.scss) and installed Bootstrap SCSS. | Move the bundler integration to a compatible modern Sass API and migrate project styling/dependency usage. Verify upstream Bootstrap compatibility or use an approved dependency strategy; a project-only `@use` replacement does not remove Bootstrap's internal deprecations. Compare rendered UI and compiled CSS. |
| FIX14 | Vite's large-chunk warning. | Analyze actual imports, routes, locale payloads, and embedded compressed size; remove unnecessary inclusion or split at meaningful load boundaries. Validate every lazy asset through the firmware HTTP filesystem, offline navigation, and flash budget. Do not merely increase the warning limit. |
| FIX15 | npm deprecations and security findings. | Trace lockfile dependency chains and update the owning direct packages with reviewed compatible releases. Re-run full-tree audit/install/build/generator checks; separately assess development-tool and shipped-code exposure. No blind `npm audit fix --force`. |
| FIX16 | Global warning suppressions, optional web discovery, and unenforced CI quality checks. | Review disabled diagnostics, fail closed for required tools/assets, introduce phase-appropriate strict gates, pin tested toolchains, and retain machine-readable diagnostics. Validate missing-tool and deliberately failing-check fixtures. |

### Debug Memory Risk

The build currently requests 4 KiB stacks per core. Debug reported a 3,272-byte
frame in the MainMenu screen constructor, a 1,456-byte Wii-extension frame,
1,272/1,312-byte splash-screen frames, and potentially unbounded SSD1306 stack
usage. A frame warning is not proof of runtime overflow, but these sizes justify
call-chain analysis before cosmetic warning work. SDK lwIP and PIO USB warnings
must be routed upstream or narrowly repaired without compromising their behavior.

### Frontend Diagnostic Groups

The 225 ESLint errors comprise 83 unused-variable, 43 literal-string/localization,
36 `prefer-const`, 23 link-target safety, 18 unknown JSX property, 5 `no-var`,
4 explicit-any, 3 empty-block, 3 undefined-name, 3 JSX-key, 2 banned-type, and
2 prototype-access diagnostics. All 51 warnings are
`react-refresh/only-export-components`. High-count files include
[Wii addon](../../www/src/Addons/Wii.tsx),
[settings](../../www/src/Pages/SettingsPage.jsx),
[boot-mode store](../../www/src/Store/useBootModesStore.ts), and
[Web API client](../../www/src/Services/WebApi.js).

TypeScript groups include 55 implicit callback parameter types, 42 missing
properties, 17 string-indexing errors, 14 non-numeric index errors, 10 implicit
destructured types, 8 assignment errors, and 7 missing `JSX` namespace errors;
the remaining 13 cover argument/shape/arithmetic/optional-data/declaration issues.
The context inferred as `null` across multiple addons is a shared typing problem,
not a reason to cast every caller. The parser explicitly supports TypeScript
`>=4.3.5 <5.4.0`, while the lockfile installs 5.9.3. React's runtime/types major
versions also differ; both require compatibility review.

The critical npm audit package entries are transitive `protobufjs` and
`shell-quote`, both reported with fixes available by this audit. Severity counts
and fix availability can change without a source edit, so refresh the audit when
implementation starts and verify the actual dependency path and version change.

## Reproduction and Coverage

The measured Release builds used the existing
[board matrix script](../../tests/build-board-matrix.cmake), which performs fresh
configure, Ninja output cleaning, full builds, hardware/storage assertions, and
artifact checks. From the repository root in the supported PowerShell environment:

```powershell
$env:PICO_SDK_PATH = "$env:USERPROFILE/.pico-sdk/sdk/2.3.1"
cmake -G Ninja '-DGP2040_BUILD_TARGETS=Pico;Pico2W' -DGP2040_BUILD_WITH_WEB=TRUE -P tests/build-board-matrix.cmake
```

For the Debug sample, clear inherited board-selector/legacy variables, set
`SKIP_WEBBUILD=TRUE` after generating web assets, then run:

```powershell
cmake -G Ninja -S . -B build --fresh -DGP2040_BOARD=Pico -DCMAKE_BUILD_TYPE=Debug -DSKIP_WEBBUILD=TRUE
ninja -C build -t clean
ninja -C build
```

Check every command's exit status and capture stdout/stderr independently. Run
the normal `npm run lint` from `www`; for JSON diagnostics in this Windows setup,
invoke installed ESLint directly with the same flags rather than forwarding new
arguments through the npm wrapper:

```powershell
node node_modules/eslint/bin/eslint.js src --ext js,jsx,ts,tsx --report-unused-disable-directives --max-warnings 0 --format json --output-file ../build/board-validation/web-lint.json
.\node_modules\.bin\tsc.cmd --project tsconfig.json --noEmit --pretty false
npm audit --json
```

Use installed binaries, not an `npx` invocation that may fetch an unpinned tool.
The existing typecheck includes `src` and `env.d.ts`, with generated types checked
when imported; review coverage of JS, tooling, server, generated code, and Vite
configuration rather than assuming all source is checked. The
[locale script](../../www/scripts/checklocale.js) is a Git-history comparison
helper, not an automated translation-completeness validation gate.

| Coverage | Status / required extension |
| --- | --- |
| Windows Pico Release with full web build | Measured above; warning-bearing success. |
| Windows Pico2W Release | Measured above; covers RP2350/CYW43 compilation, not all wireless boards. |
| Windows Pico Debug | Measured above; warning-bearing success, not hardware stack qualification. |
| Other registered targets and external packages | Not rerun in this audit. Expand from the repository's eight reference targets to the release registry, recording exceptions as incomplete work. |
| Wireless Debug and other capability-specific configurations | Not measured; required to expose conditional warning paths. |
| Hosted Linux CI / supported Node and compiler matrix | Not run here. Reproduce on pinned supported versions and resolve differences before completion. |
| Additional warnings currently suppressed | Not measured with suppressions removed; a required follow-up inventory, not assumed zero. |

## Enforcement Design

The [Node workflow](../../.github/workflows/node.js.yml) currently installs and
builds with `CI=false`; it does not run lint or typecheck. The
[firmware workflow](../../.github/workflows/cmake.yml) builds Release targets and
consumes shared web assets, but lacks a warning-free gate and Debug coverage.
`CI=true` alone is not a substitute for explicit tools and fail-on-warning policy.

- After fixes, enable project-target `-Werror` and appropriate CMake warning/error
   policies without allowing imported targets to inherit arbitrary flags blindly.
   Audit third-party targets separately so target scoping does not hide their debt.
- Review `-Wno-format` with portable integer formatting and checked format types;
   inventory `-Wno-unused-function` before narrowing/removing it. Review existing
   disabled lint rules as deliberate policies, not automatic evidence of cleanliness.
- Retain the existing ESLint zero-warning threshold and add an explicit installed
   TypeScript command. Select compatible tool versions and predictable dependency
   resolution before enabling new CI gates.
- Use supported warning/fatal-deprecation hooks for Sass/Vite/generators where
   available; otherwise add tested diagnostic capture that preserves full output,
   exit codes, and unknown warnings. No brittle single `grep warning` as the sole gate.
- Fail web-inclusive configure if Node/npm, the web source, or required generated
   assets are unavailable. Check freshness/provenance of the shared CI filesystem
   artifact as well as its existence. Do not remove supported firmware-only builds.
- Run a separate full dependency-security gate. For this feature's strict completion
   claim, known unresolved audit findings remain debt even if a risk exception is
   temporarily accepted elsewhere; avoid hiding dev dependencies with audit filters.
- Retain raw logs, tool/lockfile provenance, deduplicated issue inventories, counts,
   and artifacts in CI. Temporary baselines must reject new warnings and shrink as
   fixes land; only an empty debt baseline meets the final goal.

## Remediation Rules

Repair project code at the owning abstraction with behavior-scoped tests. For
generated code, fix generator inputs/tool versions rather than hand-editing output.
For external dependencies, prefer a compatible upstream fix or reviewed version
update; a minimal documented local patch is a last resort with an upstream/removal
reference. Do not reformat or modernize unrelated code during warning cleanup.

Do not use global `-w`, new broad `-Wno-*` options, blanket lint disables, unchecked
casts, `any`, `skipLibCheck`, raised bundle thresholds, quiet modes, or filtered logs
to make the build look clean. Validate any narrowly justified diagnostic policy
change against the actual contract; waived diagnostics remain visible debt.

Dependency security remediation must follow [dependency management](dependency-updates.md)
and [npm upgrade planning](npm-major-upgrades.md), not an unreviewed force upgrade.
Reuse the [nanopb](nanopb-stable-migration.md),
[TinyUSB](tinyusb-upstream-port.md), and [PIO USB](pico-pio-usb-upstream-port.md)
workstreams when their changes are genuinely necessary; do not make every planned
major migration a prerequisite without evidence.

## Delivery Sequence

1. Capture raw diagnostics and tool versions from clean builds and separate quality
   checks; group duplicate messages by root cause, owner, phase, and affected target.
2. Fix build blockers and correctness warnings first, then project warning debt,
   generated/dependency deprecations, WebUI checks, and bundle/asset warnings.
3. Validate each repair with narrow tests, then clean firmware/WebUI builds and
   behavior checks for the affected subsystem. Preserve existing features and
   hardware safety limits throughout dependency changes.
4. Add strict target-scoped compiler and configure policies plus web quality gates
   only after the corresponding diagnostics have been repaired. During migration,
   use explicit tracked debt rather than a false zero-warning claim.
5. Qualify the complete supported matrix, publish evidence, and enforce regression
   gates locally and in CI. Keep dependency/toolchain upgrades separately reviewable.

## Acceptance

1. Every baseline diagnostic has a recorded fix and validation; unresolved warnings,
   failures, or suppressed debt prevent completion of this feature.
2. Required firmware, WebUI, generator, lint, type, and dependency checks succeed
   without warnings from a clean checkout/install/configure/build state.
3. Generated files and embedded assets match their inputs; no stale output can make
   a skipped or failed web/generator step appear successful.
4. Relevant clean builds use SDK 2.3.1, Ninja, registered `GP2040_BOARD` targets, and
   the repository's existing workflow. Include Standard Pico and web-inclusive
   validation plus additional architecture/wireless/Debug coverage.
5. Compiler-warning fixes preserve protocol layout, latency, memory, storage, and
   hardware behavior; frontend changes preserve configuration workflows and locales.
6. Regression enforcement is tested by deliberately introducing representative
   diagnostics in isolated fixtures, proving the gate fails without changing
   production code or leaving test warnings in release artifacts.

## Open Decisions

Define supported host/toolchain versions, exhaustive target coverage, TypeScript
ownership boundaries, security-advisory policy, numerical bundle budgets, and CI
artifact retention before implementation signoff. A quiet build on one developer
machine is not proof of a warning-free release matrix.
