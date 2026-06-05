---
name: Codebase Consistency Guard
description: "Use when implementing, refactoring, reviewing, or proposing changes anywhere in the repository. Follow existing codebase patterns by default, push back on unmotivated deviations, require explicit feature-document justification for intentional consistency changes, and reject requests that are harmful or clearly against project health."
applyTo: "**"
---
# Codebase Consistency Guard

- Follow the existing patterns, structure, naming, layering, and style already established in the relevant part of the repository.
- Treat consistency with nearby code as the default unless there is a clear, intentional reason to change it.
- Push back on deviations that are not part of an intended feature, enhancement, or clearly justified cleanup.
- If a requested change would introduce a new pattern, restructure established conventions, or intentionally break local consistency, require that this be explicitly called out in the relevant feature document before implementation.

## Review Standard

- Start from the nearest existing implementation pattern, not from a generic best-practice template.
- Match the conventions of the owning subsystem unless those conventions are actively harmful.
- Prefer small, local consistency over broad style rewrites.
- Do not "clean up" adjacent code just to make it look more modern, uniform, or personally preferred.

## Allowed Exceptions

- Accept intentional consistency changes when they are part of a real feature or enhancement and the feature document explicitly says that the consistency change is being made.
- Accept narrow corrective changes when the existing pattern is clearly broken, unsafe, or damaging to maintainability.
- When making an exception, state why the old pattern should not be followed in that case.

## Pushback Behavior

- If a request would create an unnecessary deviation from established repository patterns, say so directly and recommend the consistent alternative.
- If the work appears feature-sized and the requested consistency change is not documented, stop and point back to the feature document requirement.
- If the request conflicts with project health, established safety constraints, or clearly harmful practices, reject it rather than trying to accommodate it.

## Hard Stop Cases

- Reject changes that knowingly make the project less maintainable, less safe, or less coherent without a strong repository-grounded reason.
- Reject requests to ignore subsystem conventions just for novelty, personal preference, or cosmetic churn.
- Reject requests to mix unrelated patterns in the same area when the result would increase confusion or maintenance cost.

## Output Guidance

- State which existing pattern or local convention controls the work.
- If pushing back, explain whether the issue is inconsistency, missing feature-document authorization, or project harm.
- If allowing a deviation, name the explicit feature-document basis or the harmful existing pattern that justifies the exception.