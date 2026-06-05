# Developer Documentation System — Feature Document

**Last updated:** 2026-06-05  
**Maintained by:** GP2040-CE core team  
**Status:** Planning + pre-implementation audit  
**Scope:** Developer-facing documentation workflow in this repository

---

## Overview

GP2040-CE has strong user-facing docs on the project website, but developer documentation is not yet managed as a first-class, release-aware system.  
This feature introduces a repository-native developer documentation system that:

- keeps developer docs versioned with source code,
- produces docs as build artifacts,
- and automatically proposes documentation refreshes when a release is cut.

---

## Goals

1. Establish a clear, maintained developer documentation source of truth.
2. Include developer docs in CI build outputs.
3. Automatically detect and apply release-time documentation updates.
4. Open an automated PR back to `develop` when release-driven doc changes are generated.
5. Keep workflow reviewable and auditable through normal PR process.

## Non-Goals

- Replacing existing end-user website documentation in this phase.
- Building a full external docs portal in this repository in phase 1.
- Auto-merging generated documentation PRs without review.

---

## Pre-Implementation Audit

## Current repository state

- Developer docs already exist under `/tmp/workspace/Fortinbra/GP2040-CE/docs/development/` as feature and architecture documents.
- Existing CI has:
  - Node workflow that runs `npm ci` and `CI=false npm run build --if-present` in `www/`.
  - CMake workflow that consumes `fsData` artifacts from the Node workflow.
- No dedicated developer-doc generation or publishing workflow currently exists.
- No release-triggered automated docs-sync PR flow currently exists in this repository workflows.

## Gaps identified

1. No canonical developer-doc index/governance document.
2. No CI artifact for developer docs.
3. No release automation for docs refresh.
4. No guardrails to detect stale generated docs in PRs.

---

## Options Analysis

## Option A: Repository-native docs (recommended)

Keep docs in `docs/development/`, generate artifacts in CI, and run release-time doc sync automation.

**Pros**
- Versioned with code and tags.
- Normal PR/review/history applies.
- Easy to tie docs to release commits.
- Works with existing repository contribution flow.

**Cons**
- Requires workflow automation maintenance.
- Requires clear ownership and update rules.

## Option B: GitHub Wiki as primary developer docs

Use Wiki for developer documentation and maintain separately from main repository.

**Pros**
- Fast editing UX.
- Low initial setup effort.

**Cons**
- Weaker coupling to code changes and tags.
- Harder to enforce PR review and consistency.
- Harder to automate release-accurate synchronization.

## Option C: Hybrid model

Repository-native source docs with optional Wiki mirror for stable/high-level pages.

**Pros**
- Keeps authoritative source in repo while allowing convenient read surface.

**Cons**
- Two-surface maintenance complexity if mirror drifts.

---

## Recommendation

Adopt **Option A** now, with optional Option C mirror later if needed.

Primary source of truth remains repository docs under:

- `/tmp/workspace/Fortinbra/GP2040-CE/docs/development/`

---

## Proposed Architecture

## 1) Source and structure

- Keep authored docs in `docs/development/`.
- Add a developer docs index page (phase 1) that maps key subsystems and feature docs.
- Define ownership/review expectations for this directory.

## 2) CI build output

- Add docs packaging/publishing step to CI.
- On `push` and `pull_request`, produce docs artifact (for review and archival).
- Keep this independent from firmware binary packaging.

## 3) Release-time sync automation

On release/tag events:

1. Checkout release commit.
2. Run doc generation/sync tasks.
3. If changes are produced, commit on an automation branch.
4. Open or update a PR targeting `develop`.
5. Include release tag and changed-doc summary in PR body.

## 4) Drift prevention

- Add a CI validation step that fails if generated docs are stale.
- Require docs impact acknowledgment in PR review flow for feature-level changes.

---

## Implementation Plan (Detailed)

## Branch strategy

- This feature is implemented and maintained on `develop`.
- Automation-generated documentation PRs from release events also target `develop`.
- Promotion from `develop` to `main` stays in the existing repository release flow.

## Planned workflow files

1. Extend `/tmp/workspace/Fortinbra/GP2040-CE/.github/workflows/node.js.yml`
   - Add a docs packaging step after existing web build.
   - Upload a developer-doc artifact for PR/push visibility.
2. Add `/tmp/workspace/Fortinbra/GP2040-CE/.github/workflows/docs-release-sync.yml`
   - Trigger on release/tag events.
   - Run doc sync/generation tasks.
   - Create/update a PR to `develop` when docs changes exist.
3. Add `/tmp/workspace/Fortinbra/GP2040-CE/.github/workflows/docs-drift-check.yml`
   - Trigger on pull requests.
   - Fail if generated docs are out of date relative to source inputs.

## Documentation artifacts and sources

- Authoritative source docs remain in `docs/development/`.
- Generated outputs should be deterministic and limited to maintainable targets, for example:
  - auto-generated index/navigation summaries,
  - release-linked documentation metadata,
  - generated reference tables sourced from code/config where applicable.
- Generated files should live in clearly marked paths so review scope is obvious.

## Automation behavior requirements

1. **No-op safety:** if release sync produces no file changes, workflow exits successfully without PR creation.
2. **Single PR policy:** one open docs-sync PR per release line/branch to avoid PR spam.
3. **Traceability:** PR body includes release tag, generator task summary, and changed file list.
4. **Review safety:** automation never bypasses branch protections or auto-merges by default.
5. **Repeatability:** rerunning the workflow on same inputs should produce identical output.

## Validation and rollout checklist

### Phase 1 validation (artifact path)
- Confirm docs artifact appears for PR and push runs.
- Confirm artifact content maps to expected `docs/development/` sources.

### Phase 2 validation (release sync path)
- Test with a dry-run tag/release event in a safe branch context.
- Confirm PR creation targets `develop`.
- Confirm PR updates (rather than duplicates) on rerun.

### Phase 3 validation (drift enforcement)
- Confirm drift check fails when generated docs are intentionally stale.
- Confirm drift check passes after regeneration.
- Confirm contributor guidance is clear in failure output.

## Operational ownership

- **Primary owners:** core maintainers responsible for release automation and docs governance.
- **Review expectations:** docs-sync PRs require maintainer review before merge.
- **Fallback procedure:** if automation fails, maintainers run doc sync locally and open a manual PR to `develop`.

---

## High-Level Rollout Plan

## Phase 1 — Foundation

- Add this feature document.
- Add developer docs index/governance page.
- Add CI docs artifact generation.

## Phase 2 — Release automation

- Add release-triggered docs sync workflow.
- Add automated PR creation/update behavior.

## Phase 3 — Enforcement and hardening

- Add stale-doc checks in CI.
- Add ownership/review guardrails.
- Monitor first release cycles and tune automation.

---

## Risks and Mitigations

1. **Noisy or low-value generated diffs**  
   Mitigation: constrain generators to deterministic outputs and scoped targets.

2. **Automation failures during release**  
   Mitigation: keep manual fallback path documented and preserve normal PR approval gate.

3. **Contributor confusion about source of truth**  
   Mitigation: explicitly document that `docs/development/` is canonical.

4. **Workflow runtime/cost growth**  
   Mitigation: run heavy doc-generation only on release/tag paths and lightweight checks on PRs.

---

## Acceptance Criteria

1. Developer docs have a canonical source path and index.
2. CI publishes developer docs artifacts on PRs/pushes.
3. Release event can generate docs updates and open/update a PR to `develop`.
4. Generated-doc drift is detectable in CI.
5. Process is documented for maintainers, including fallback manual steps.

---

## Audit Outcome

This feature is **ready for implementation planning**.  
Repository-native docs with release-time PR automation is the best fit for GP2040-CE’s existing workflow and review model.
