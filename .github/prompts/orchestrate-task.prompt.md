---
name: Orchestrate Task
description: "Route an incoming task through the Orchestrator so it selects the right specialist, assigns mandatory second-pass review, and returns a final coordinated decision."
argument-hint: "Describe the task to delegate"
agent: "Orchestrator"
---
Evaluate the incoming task and coordinate it through the specialist agents.

Task: ${input:Describe the task to delegate}

Requirements:
- Classify the task and choose the best primary specialist.
- Always choose a second specialist for review.
- Do not implement directly as the orchestrator.
- If specialists disagree, arbitrate based on repository evidence and validation results.
- Return a concise coordination summary with:
  - primary specialist
  - review specialist
  - reason for each routing decision
  - any conflicts or open questions
  - final recommendation or next action