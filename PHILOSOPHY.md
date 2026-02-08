# Software Philosophy

This document describes my programming philosophy. Follow these principles when working on my code.

## Core Belief

A programmer's job is not to write code. It is to solve data transformation problems. Everything is a data problem, not a code problem. Understand the data and you understand the problem.

## Data Oriented Design

- Design around the data, not an idealized model of the world.
- Hardware is the platform. Software abstractions are tools, not truth.
- Where there is one, there are many. Never design in a vacuum for a single case.
- Solve for the **common case**, not the generic case.
- Think about what the data actually is, where it lives, and how it flows. Think about cache lines, memory layout, and access patterns.
- Prefer big structs that represent the totality of what is being processed. Data that belongs together lives together.
- The pipeline is: Input → Data Transformation → Output. Name and understand each stage clearly before writing code.

## N+1 Thinking

Think in terms of groups of data, not individual items. When you see one thing being processed, immediately ask: what does this look like for N of them? Design the solution for the group, not the singleton. Group lifetimes. 

## Slow is Smooth, Smooth is Fast

Reject "move fast and break things." Speed from cutting corners only creates debt that slows you down later. Software development is a marathon, not a sprint.

- After every "make it work" sprint, do a "make it right" sprint. No exceptions.
- Better yet: make it right from the beginning. The speed you think you gain from quick plumbing only bogs you down later.
- Most of the time, making it right gets you 90% of the way to making it fast.
- If you have strict performance requirements, making it fast *is* making it right.

## Refactoring Approach

When refactoring a pipeline, work from the **ends inward**:

1. Design and implement the **output** first. Know what the final shape of the data looks like.
2. Design and implement the **input** next. Know what you're starting with and how users express it.
3. The **middle** (the transformation) becomes clear once both ends are defined.

Do not try to design the middle of a pipeline without understanding both the input and output.

## How The User Uses LLMs

- LLMs are tools for **exploring conceptual space** and showing language syntax the user doesn't know. They are not reliable programmers.
- LLMs are decent coders when given **precise specifications** - but those same specifications work for any programmer, including the user.
- The user prefers to implement by hand. Typing code builds the mental model. Without implementing, the user won't see how pieces fit together, and without that, the user can't understand the system.
- You cannot be a reliable tech lead for LLMs if you don't understand the system yourself.

## When Helping The User Code

- Understand the data transformation problem before proposing solutions.
- Prefer simple, explicit code over clever abstractions.
- Don't introduce unnecessary dependencies or layers of indirection.
- Match the existing code style: comment blocks at file tops, section separators with `// name === /`, explicit module structure.
- When the user asks for implementation help, give them specifications and snippets, not full rewrites. They want to understand what they are building.
- Solve for the common case first. Do not over-engineer for hypothetical edge cases.
- If something is wrong, tell the user directly. Don't hedge.
- In agent building mode it is time to implement, the user will not be implementing the code on their own, it becomes your job.
