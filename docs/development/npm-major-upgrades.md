# npm Major Version Upgrades

## Overview

The web configurator (`www/` directory) has multiple npm dependencies with major version updates available that were **not** applied during the automated `npm update` run (which respects semver constraints). This document catalogs the deferred major version upgrades, breaks down the key breaking changes per package, groups them into logical upgrade phases, and provides a roadmap for completing the modernization of the web configurator's dependencies.

**Status:** Planned — deferred during automated dependency audit (requires staged rollout and thorough testing)

## Current State

The web configurator is a React + Vite + TypeScript SPA that communicates with the RP2040 firmware via Protocol Buffers. The following npm packages have major version upgrades available:

| Package | Current | Available | Key Breaking Changes | Phase |
|---|---|---|---|---|
| `react` | 18.3.1 | 19.2.4 | New JSX transform, hook behavior, `use()` API | 4 |
| `react-dom` | 18.3.1 | 19.2.4 | Async rendering improvements, hydration changes | 4 |
| `vite` | 4.5.14 | 8.0.3 | Skips v5, v6, v7 (3 major versions); config migration required | 2 |
| `@vitejs/plugin-react` | 4.7.0 | 6.0.1 | Tracks Vite major version | 2 |
| `react-router-dom` | 6.30.3 | 7.13.2 | New file-based routing paradigm, loaders/actions, createBrowserRouter required | 4 |
| `react-i18next` | 12.3.1 | 17.0.1 | Multiple breaking versions skipped | 4 |
| `i18next` | 23.16.8 | 26.0.1 | Multiple breaking versions skipped | 4 |
| `eslint` | 8.57.1 | 9.39.4 | **Flat config required** (`.eslintrc` → `eslint.config.js`); v9 is breaking | 1 |
| `@typescript-eslint/eslint-plugin` | 6.21.0 | 8.x | Requires ESLint v9; flat config adoption mandatory | 1 |
| `@typescript-eslint/parser` | 6.21.0 | 8.x | Requires ESLint v9; flat config adoption mandatory | 1 |
| `typescript` | 5.9.3 | 6.0.2 | Stricter type checking; may require type fixes in existing code | 3 |
| `zustand` | 4.5.7 | 5.0.12 | Store API changes (selector behavior, dispatch signatures) | 3 |
| `express` | 4.22.1 | 5.2.1 | Async error handling changes (used in dev server tooling) | 1 |
| `protobufjs-cli` | 1.2.0 | 2.0.0 | Proto generation output format may change; affects firmware communication | 5 |

**Additional findings:**
- **11 npm audit vulnerabilities** present in transitive dependencies (2 moderate, 9 high) — must be evaluated and resolved as part of upgrade phases
- **Sass deprecation warnings** in Bootstrap 5 SCSS (legacy JS API and `@import`) — should be resolved before or alongside Vite v8 upgrade (v8 has stricter Sass handling)

## Goals

1. Group major version upgrades into logical phases based on risk and dependencies
2. Provide clear action items for each phase
3. Minimize risk of blocking issues by separating high-risk upgrades (React, react-router-dom) into their own phases
4. Establish success criteria for each phase
5. Create a timeline and testing strategy for staged rollout

## Upgrade Phases

### Phase 1: Tooling Modernization (Low Risk, High Effort)
**Target upgrades:** ESLint v9, @typescript-eslint v8, express v5

**Scope:** Affect development and build tooling only; no impact on runtime functionality.

**Key breaking changes:**
- **ESLint v9:** Requires migration from `.eslintrc` config files to `eslint.config.js` (flat config format)
  - All ESLint plugins and rule definitions must be rewritten in JavaScript
  - Shareable configs (e.g., `eslint:recommended`) must be imported and wrapped in the new format
- **@typescript-eslint v8:** Requires ESLint v9; must be upgraded together
  - New plugin syntax required in flat config
- **express v5:** Async error handling changes (used in dev server/tooling, not production)

**Implementation approach:**
1. Migrate `.eslintrc.*` to `eslint.config.js` (follow ESLint v9 migration guide)
2. Update all ESLint plugin configurations to flat config syntax
3. Run `npm update eslint @typescript-eslint/eslint-plugin @typescript-eslint/parser express` for Phase 1 packages only
4. Test: `npm run lint` (entire codebase must pass without warnings)
5. Run full CI build to ensure no breakage

**Duration:** 2–3 days (most effort is in flat config migration, not code fixes)

**Success criteria:**
- ✓ `.eslintrc*` files removed; `eslint.config.js` created and functional
- ✓ `npm run lint` passes with zero errors/warnings on entire www/ directory
- ✓ CI build completes successfully
- ✓ No changes to production code needed (tooling-only upgrade)

---

### Phase 2: Build Pipeline Modernization (Medium Risk, Medium Effort)
**Target upgrades:** Vite v8, @vitejs/plugin-react v6

**Scope:** Affects build pipeline and dev server. No runtime code changes needed, but build config must be updated.

**Key breaking changes:**
- **Vite v8:** Skips three major versions (v5, v6, v7). Each introduced deprecations and removals:
  - v5: Removed legacy `renderBuiltUrl` API, deprecated `ssrLoadModule`
  - v6: Changed manifest format, removed `isProduction` (use `mode` instead)
  - v7: Deprecation removals, Rollup API changes
  - v8: Continued API refinements, potential SSR changes
- **Pre-existing Sass warnings:** Vite v8 may enforce stricter Sass import handling
  - Bootstrap 5 SCSS uses deprecated `@import` syntax and legacy JS API
  - May need to migrate to `@use` syntax or configure Sass compatibility layer

**Implementation approach:**
1. Review Vite v5→v8 migration guide (official docs: https://vitejs.dev/guide/migration.html)
2. Update `vite.config.ts`:
   - Remove deprecated `renderBuiltUrl` if present
   - Update `createViteConfig` or equivalent to v8 syntax
   - Enable Sass compatibility layer if needed (e.g., `sass.legacy: true` in vite.config.ts)
3. Update `@vitejs/plugin-react` to v6 (must track Vite major version)
4. Run `npm update vite @vitejs/plugin-react`
5. Test: `npm run build` (production build must complete without warnings)
6. Test: `npm run dev` (dev server must start and hot reload must work)
7. Manual testing: load configurator in browser, verify all pages load and respond

**Duration:** 2–3 days (config updates + testing)

**Success criteria:**
- ✓ `vite.config.ts` migrated to v8 syntax without errors
- ✓ Sass warnings resolved (if any)
- ✓ `npm run build` completes successfully with zero warnings
- ✓ `npm run dev` starts without errors; hot reload works
- ✓ Web configurator loads in browser and all UI pages respond

**Dependency:** Must complete Phase 1 first (ESLint v9 + TypeScript may affect this)

---

### Phase 3: Language & State Upgrade (Low-Medium Risk, Low Effort)
**Target upgrades:** TypeScript v6, Zustand v5

**Scope:** Runtime code may need minor type fixes. Zustand store API changes are localized.

**Key breaking changes:**
- **TypeScript v6:** Stricter type-checking rules
  - May emit new errors on existing code (e.g., stricter null checks, stricter function overloads)
  - Recommended fix: enable `strict: true` gradually or address reported errors incrementally
- **Zustand v5:** Store selector and dispatch API changes
  - Selector behavior changed (more performant but requires explicit memoization in some cases)
  - Dispatch signatures may change (unlikely to be breaking in GP2040-CE context, but verify)

**Implementation approach:**
1. Run `npm update typescript`
2. Attempt to rebuild: `npm run build`
3. If TypeScript errors appear, assess each:
   - If fixable easily (e.g., add explicit types), fix inline
   - If widespread, may need staged rollout (enable strict mode gradually)
4. Run `npm update zustand`
5. Test all state-managed UI pages (configurator settings, profile management, etc.)
6. Verify: `npm run build` and `npm run dev` both work

**Duration:** 1–2 days (mostly TypeScript type fixes if needed)

**Success criteria:**
- ✓ TypeScript v6 compiles with zero errors (or errors are deliberately addressed)
- ✓ Zustand v5 store API confirmed compatible with existing code (may need minor refactor)
- ✓ `npm run build` successful
- ✓ All state-managed pages tested and responsive

**Dependency:** Perform after Phase 2 (Vite upgrade may affect TypeScript compilation)

---

### Phase 4: React & Routing Upgrade (High Risk, High Effort)
**Target upgrades:** React 19, react-dom 19, react-router-dom 7, react-i18next 17, i18next 26

**Scope:** Core framework and routing. Requires UI testing and potential component refactoring.

**Key breaking changes:**
- **React 19:** New JSX transform (no need to import React in files), hook behavior changes, new `use()` API for promises/context
- **react-dom 19:** Async rendering improvements; hydration behavior changes (affects SSR if used)
- **react-router-dom 7:** Fundamental routing paradigm shift
  - File-based routing is new standard (automatic route generation from file structure)
  - Loaders/actions replace older data-fetching patterns
  - `createBrowserRouter` or `RouterProvider` required (breaking from `BrowserRouter` wrapper)
  - Data mutations now use `Form` component and loader actions, not manual `fetch()`
- **react-i18next 17:** Multiple breaking versions skipped; may include namespace or hook API changes
- **i18next 26:** Breaking changes in plugin or language file format (verify with react-i18next compatibility)

**⚠️ WARNING:** This phase has the highest risk. React 19 and react-router-dom 7 are major refactors. Do NOT combine with other phases.

**Implementation approach:**
1. **Create a feature branch:** `feature/react-19-upgrade`
2. **Upgrade one library at a time:**
   - First: `npm update react react-dom`
   - Test: Can existing components render? Are hook behaviors correct?
   - Then: `npm update react-router-dom`
   - Test: Do all routes still work? Verify loaders/actions (may need refactor)
   - Then: `npm update react-i18next i18next`
   - Test: Are translations loading? Are language switches working?
3. **Code migration:** Expect to refactor:
   - Remove `import React` from files (unless using legacy JSX)
   - Update `useContext` calls to use `use()` API where applicable
   - Migrate route definitions to loaders/actions pattern
   - Update i18n hooks if API changed
4. **Testing:** Comprehensive UI regression testing required
   - Load all pages in configurator
   - Test form submissions (profile save, button bindings, etc.)
   - Test language switching on every page
   - Test on multiple browsers (Chrome, Firefox, Safari)

**Duration:** 5–7 days (significant refactoring + testing)

**Success criteria:**
- ✓ React 19 compiles and renders without errors
- ✓ All routing works correctly; loaders/actions configured
- ✓ Form submissions and data mutations work
- ✓ All pages load and respond to user input
- ✓ Language switching works on all pages
- ✓ Comprehensive browser testing (Chrome, Firefox, Safari) passes
- ✓ No console errors or warnings in browser dev tools

**Dependency:** Perform after Phase 3 (TypeScript and Zustand should be stable first)

---

### Phase 5: Proto Compiler Upgrade (Medium-High Risk, Medium Effort)
**Target upgrades:** protobufjs-cli v2

**Scope:** Affects firmware ↔ web configurator communication. Changes to proto output must be validated against firmware.

**Key breaking changes:**
- **protobufjs-cli v2:** Proto generation output format may change
  - Generated JavaScript code structure could differ (e.g., class vs. factory functions)
  - Message definitions, field accessors, and serialization APIs may change
  - **Critical:** The firmware expects specific protobuf messages; format changes could break communication

**⚠️ WARNING:** This upgrade affects the contract between firmware and web configurator. Must be carefully validated. Do NOT combine with other phases.

**Implementation approach:**
1. **Review protobufjs-cli v2 changelog** for output format changes
2. **Generate test proto files** with both v1 and v2, compare output:
   - Message class/factory structure
   - Field accessor names and types
   - Serialization/deserialization method signatures
3. **Update `proto/` build step** in `www/package.json` or build script to use v2
4. **Regenerate all proto files:** `npm run build-proto` (or equivalent)
5. **Verify no API breaks** in the generated code:
   - Check TypeScript type definitions
   - Ensure field accessors match expected names
6. **Test with firmware:**
   - Flash firmware to RP2040
   - Load configurator in browser
   - Save a configuration (exercise serialization)
   - Reload configuration (exercise deserialization)
   - Verify settings persist correctly
7. **Backward compatibility check:** Can the firmware (old proto version) deserialize messages from the web configurator (new proto version)?

**Duration:** 2–3 days (format comparison + integration testing with firmware)

**Success criteria:**
- ✓ protobufjs-cli v2 generates valid JavaScript/TypeScript code
- ✓ Generated API is compatible with or adapters are in place for web configurator code
- ✓ Proto regeneration step in build process works correctly
- ✓ Web configurator saves and loads configurations without errors
- ✓ Firmware and web configurator successfully exchange messages
- ✓ Settings persist correctly in flash after firmware reboot

**Dependency:** Perform LAST. Should not be combined with React upgrade (Phase 4) to isolate communication issues from UI issues.

---

## Pre-Phase Checklist

Before starting Phase 1:

1. ✓ Back up current `www/package-lock.json` (version control or separate file)
2. ✓ Review current npm audit results: `npm audit` (document all 11 vulnerabilities)
3. ✓ Create a branch per phase (`feature/eslint-v9`, `feature/vite-v8`, etc.) — **never main**
4. ✓ Ensure CI/CD pipeline is green before starting (all existing tests pass)

## npm Audit Findings

Run `npm audit` before Phase 1 and evaluate all vulnerabilities:

**Expected vulnerabilities:** 2 moderate, 9 high in transitive dependencies

**Action:** Create an audit suppression strategy:
- Identify which vulnerabilities can be resolved by upgrading direct dependencies (Phases 1–5 may fix some)
- For remaining vulnerabilities, document whether they affect runtime (production code) or dev-only tools
- If unresolved after all phases, create a separate issue to track remediation

## Testing Strategy

**Per-phase testing:**
- Unit tests (if any exist): `npm run test` must pass
- Linting: `npm run lint` must pass with zero warnings
- Build: `npm run build` must complete without errors or warnings
- Dev server: `npm run dev` must start and serve without errors
- Manual UI testing: navigate all pages, fill forms, test language switching, test settings save/load

**Cross-phase testing:**
- After each phase, run the full build and ensure the next phase is not blocked
- Maintain a test matrix (browser × page) for manual testing at the end of Phase 4

**Hardware integration testing (Phase 5 only):**
- Flash firmware to RP2040
- Boot device and connect web configurator
- Perform full config save/load cycle
- Verify data integrity in firmware flash

## Timeline

- **Phase 1:** 2–3 days (ESLint, TypeScript ESLint, express)
- **Phase 2:** 2–3 days (Vite, @vitejs/plugin-react)
- **Phase 3:** 1–2 days (TypeScript, Zustand)
- **Phase 4:** 5–7 days (React, react-router-dom, i18next) — largest phase
- **Phase 5:** 2–3 days (protobufjs-cli)

**Total estimated effort:** 2–3 weeks for the complete upgrade path, assuming no blocking issues.

## Success Criteria (Overall)

1. ✓ All five phases completed
2. ✓ npm audit vulnerabilities resolved or explicitly documented
3. ✓ `npm run build` and `npm run dev` both work without errors
4. ✓ All pages in web configurator load and respond correctly
5. ✓ Language switching works on all pages
6. ✓ Form submissions (settings save) work correctly
7. ✓ Firmware and web configurator communicate without errors
8. ✓ Comprehensive test coverage (browser × page matrix) passes
9. ✓ Documentation updated in `docs/development/dependency-updates.md`
10. ✓ All PRs merged to main branch with full test results

## References

- npm audit: Run `npm audit` in `www/` directory to see current vulnerabilities
- Official migration guides:
  - ESLint v9: https://eslint.org/docs/latest/use/configure/migration-guide
  - Vite v5→v8: https://vitejs.dev/guide/migration.html
  - React 19: https://react.dev/blog/2024/12/05/react-19
  - react-router-dom v7: https://reactrouter.com/upgrading/v6
  - Zustand v5: https://github.com/pmndrs/zustand/releases

---

**Maintained by:** GP2040-CE core team  
**Last updated:** 2026-03-28  
**References:** `www/package.json`, `www/package-lock.json`, `docs/development/dependency-updates.md`
