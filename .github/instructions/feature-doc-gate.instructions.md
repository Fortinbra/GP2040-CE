---
name: Feature Document Gate
description: "Use when handling new features, feature requests, feature implementation planning, audits that may expand scope into a feature, or work that changes user-visible behavior. Require an existing feature document before implementation, push back pragmatically when one does not exist, and do not implement while feature documents are still being written."
---
# Feature Document Gate

- Treat net-new features, major feature expansions, and user-visible behavior changes as requiring an existing feature document before implementation starts.
- If no feature document exists, push back and steer the work toward creating or refining the feature document first.
- While a feature document is being written, revised, or researched, do not implement the feature in the same pass.
- Prefer feature documents under `docs/development/` unless the repository already has a better established location for that feature area.

## Pragmatic Exceptions

- Do not block small bug fixes, narrow refactors, typo-level UI adjustments, documentation-only work, or clearly scoped maintenance tasks just because they lack a feature document.
- Do not invent process friction for minor follow-up work that is already covered by an existing feature document or an accepted implementation plan.
- If the work is borderline, call that out explicitly and explain whether it should proceed immediately or be documented first.

## Expected Behavior

- Ask or check whether a feature document already exists before starting implementation for feature-level work.
- If one exists, ground the implementation or review in that document.
- If one does not exist, recommend creating the document first and help produce or refine that document instead of coding.
- When pushing back, be direct and pragmatic: explain why the work appears feature-sized and what document is missing.

## Output Guidance

- State whether the requested work is feature-sized or small enough to proceed without a new feature document.
- If blocked on documentation, name the likely document location and the next documentation step.
- If proceeding without a new document, briefly justify why the exception is appropriate.