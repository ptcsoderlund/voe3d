# 0174 — 006's finished cards are merged back, not redone
date: 2026-09-18
by: tech-lead

## Decision
`feature/006-themes-old` (tip `5f5c622`), which holds 006's finished cards 01–04, the blocked card 05 and
decisions 0170–0172, is merged into `feature/006-themes`. The next `/drive` does the merge as its first step,
before anything is planned. Resolve the four conflicts like this. In `Agentic/tech-lead/decisions/decisions.md`,
`text/text.md` and `ui/ui.md`, keep both sides' lines; in `decisions.md` the 0170–0172 lines go in number order,
and the sentence saying they "arrive when it is accepted" is dropped. In `dev/src/main.c`, keep 008's code and add
card 02's `VOE_TEXT_TYPEFACE_OXANIUM` argument. The build and checks must pass after the merge. Then the planner
re-cuts `blocked/05` against the merged tree. Once the merge is committed, `feature/006-themes-old` may be deleted.

## Reasoning
The four cards are finished and reviewed, and the merge resolves each conflict only once.
- Rebasing the old commits onto `0ad01d5`: the history is cleaner, but the same conflicts are resolved once per commit.
- Planning 006 from scratch: four cards and three decisions would be written again for nothing.

## Replaces
nothing
