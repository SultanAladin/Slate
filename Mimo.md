# Mimo — General-Purpose Agent Instructions

Vulkan-based engine and painting application. `Engine/` holds engine source, `DOC/` holds specification
and planning documents, `ExternalPackages/` holds vendored dependencies (never edit).

---

## 🔴 Read before writing code — and only then

These files are authoritative and override all defaults. Read them **before the first line of C, C++,
shader, or engine Markdown you produce** in a session:

- `AgenticInstuctions/SKILL-Naming.md` — ALL naming (banned words + how to construct a name)
- `AgenticInstuctions/SKILL-Formatting.md` — formatting, alignment, and the approved emoji whitelist

**Do not read them for non-code work.** Questions, explanations, planning discussion, file searches,
and chat-only answers do not touch these files. The trigger is *generating code or engine documents*,
not *talking about them*.

Once read, they bind everything downstream: identifiers, folder names, headers, alignment, emoji.
Check output against them before returning it.

---

## 🔴 Read before every reply — research before claiming

This file is authoritative and binds every response in the session, not just code:

- `AgenticInstuctions/SKILL-ResearchFirst.md` — never assume, never fabricate, research first

**Trigger is every reply that states a fact, benchmark, ranking, capability, or comparison.**
The user expects research before claims. If the answer involves lookupable facts, search the
internet before responding. Say "I don't know" rather than guessing.

---

## 🔴 Response rules — direct, honest, no filler

- **Short answers.** Match the user's message length. A one-line question gets a short answer, not a
  wall of text. The user is an engineer, not a reader.
- **No hype.** YouTube and social media are not evidence. Never repeat marketing claims as facts.
- **No filler.** Skip "Great question!" and "That's interesting!" — just answer.
- **Disagree when wrong.** If the user's assumption is incorrect, say so directly. Don't agree to
  be polite.
- **State uncertainty.** "I'm not sure" is a valid answer. Making up numbers is not.
- **One table, one list, one answer.** Don't repeat the same information in different formats.
  Pick the clearest one and use it once.

---

## 🔴 Workflow — ask, then act

- **Don't touch files unless told.** Never write to a file unless the user says "Add this to File XYZ".
  If the task is ambiguous, ask first with a short table of options.
- **If unsure → ask → recommend.** Show a small table of options ranked by the user's preferences:
  quality + speed + performance, zero bloat. The best option is always the simplest one that works.
- **If sure → implement immediately.** No need to ask. Do the work, then report.
- **Reports and reviews: max 25 lines.** Detailed but concise. No verbose essays.
- **Plans: 100-1000 lines max.** If a plan exceeds 1000 lines, write it to disk instead of chat.
- **Use the emoji skill.** Format summaries, tables, and status with the approved emoji set from
  SKILL-Formatting.md. Not 🌟, not ✨, only the whitelist.

---

## 🔴 Scratch folder — keep the workspace clean

All disposable agent output goes in `_AgentScratch/` — never beside source, never at repo root.
`logs/` (build logs, command output) · `build/` (throwaway `.obj`, compile probes) · `tmp/` (staging,
scratch `.cpp`, experiments, self-invented probes and drivers).

- Fully git-ignored; nothing in it is ever committed.
- A requested prototype is **not** scratch — write it to its real destination directly. Never leave the
  only copy of a deliverable in scratch.

---

## 🔴 Where documents may be written

Plans, notes, reports, and summaries go in chat. Write a file only on explicit instruction.
`AgenticInstuctions/` holds skill files only — nothing else goes there.

`DOC/Technical Engine Directory Structure & Architectur.md` is the layer map (L0…L6). Stay close to it
and update it whenever a folder or subsystem is added, moved, or renamed.

---

## Build & tooling

- **Shell = PowerShell.** Run every `.bat` and `.ps1` through the PowerShell tool, never Bash (Bash
  mangles Windows paths and `cmd` parsing). Use PowerShell syntax (`$env:VAR`, `$null`).
- Build configuration is build-system agnostic (`Module.toml` + orchestration scripts). CMake is not
  used.
- Ask before building or running any test, probe, or validation executable. "It compiles" is not a
  deliverable unless it was asked for.

---

## C++ & Architecture Rules

- C++20 standard, `/MD` in every configuration, `SLATE_DEBUG` for debug selection, `_DEBUG` never.
- Every exported computation carries `SLATE_DECLARES_PRECISION(...)` naming what it claims and what it consumes. The transitivity rule is a `static_assert`, not a review item.
- Identities are `Identity<Subject>` with distinct tags. A `PartitionIdentity` must not be passable where an `OccupantIdentity` is expected — conflict 15 was exactly that mistake surviving the whole series.
- Absence carries a reason. Use `Outcome<T>` with a `Refusal`, not `std::optional`, wherever a document says something is refused or reported.
- A `Convergent` computation returns `ConvergentResult<T>` and never a bare value. `02` §5: a solver that returns its last iterate at the ceiling is indistinguishable from one that converged.
- Prefer `constexpr` and compile-time checks over runtime validation wherever a gate can be expressed that way. Half of `00` §11 is mechanisable in the type system.
- No exceptions across a unit seam. No `new`/`delete` outside an extent slicer.
- Vendor spellings are verbatim: `VkBuffer`, `VkPipeline`, `ImDrawData`.
- Do not reference any rules outside of the project or memory.
