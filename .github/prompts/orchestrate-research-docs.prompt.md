---
name: Orchestrate Research And Docs
description: "Route a research-heavy or documentation-heavy task through the Orchestrator with a bias toward Research Audit first and Documentation Expert review."
argument-hint: "Describe the research, audit, or feature-document task"
agent: "Orchestrator"
---
Evaluate the incoming task as a research-heavy, audit-heavy, or documentation-heavy workflow and coordinate it through the specialist agents.

Task: ${input:Describe the research, audit, or feature-document task}

Routing policy:
- Prefer `Research Audit` as the primary specialist for code audits, feature feasibility, discrepancy investigation, and evidence gathering.
- Prefer `Documentation Expert` as the primary specialist for writing or restructuring feature documents, guides, or release-facing documentation.
- Always assign a second specialist for review.
- Prefer `Documentation Expert` as the reviewer when the outcome is a document, proposal, or user-facing explanation.
- Prefer `Research Audit` as the reviewer when the outcome depends on evidence quality, discrepancy findings, or technical feasibility.

Requirements:
- Do not implement directly as the orchestrator.
- If implementation is eventually needed, keep the current pass focused on research, audit, and documentation preparation first.
- If specialists disagree, arbitrate based on repository evidence, validation results, and documented constraints.
- Return a concise coordination summary with:
  - primary specialist
  - review specialist
  - reason for each routing decision
  - evidence or repository surfaces that should be examined first
  - any conflicts, risks, or open questions
  - final recommendation or next action