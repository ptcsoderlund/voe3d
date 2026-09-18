# 006 — needs decision: the done half of 006 is not on this branch

## Question

`feature.md` says cards 01–04 are done and committed on `feature/006-themes`, with `review/` and
`blocked/` filled. They are not on the branch as it stands. `feature/006-themes` was re-created from
`dev` at `0ad01d5` ("take work order"); the old branch's tip, `5f5c622`, was left dangling. It holds:

- `review/01` assets — a section and a key carry the line they came from (`7a34e4f`)
- `review/02` text — Pixel Operator beside Oxanium, a font asked for by typeface (`c116ab2`, `0574e78`, `ac5a1a2`)
- `review/03` ui — the theme: authored inputs in, a derived palette out (`8f90033`)
- `review/04` ui — every widget draws from the nearest theme, its call sites and review fixes (`8dcfbd1`, `00ec48c`, `e5297ca`)
- `blocked/05` — the rest of the old plan, to be re-cut
- decisions 0170, 0171, 0172 (where a theme is read and derived, the palette in OKLab, one file per theme remembered by name)

The planner pinned it as branch `feature/006-themes-old` so it cannot be garbage-collected. Work
order 009 also assumes card 02's font is in the tree. Planning the rest without that base would
either redo four finished cards or write cards against surface that does not exist.

A trial merge (`git merge-tree 0ad01d5 5f5c622`) conflicts in four files:
`Agentic/tech-lead/decisions/decisions.md`, `text/text.md`, `ui/ui.md` and `dev/src/main.c`. The
first three are index pages; `dev/src/main.c` is code, where the typeface argument from card 02 meets
008's changes.

## Options

1. **Merge `feature/006-themes-old` into `feature/006-themes`** and resolve the four conflicts by hand:
   keep both sides' index lines, and in `dev/src/main.c` keep 008's code with card 02's
   `VOE_TEXT_TYPEFACE_OXANIUM` argument. Then run the planner again to re-cut `blocked/05`. Nothing is
   redone.
2. **Rebase the old commits onto `0ad01d5`** instead: the history is cleaner and the conflicts are
   the same, but they have to be resolved once for each commit that touches those files.
3. **Throw the old work away and plan 006 from scratch**: four cards and three decisions are written
   again.

## Recommendation

Option 1. A person or the driver does the merge; it is not a card, because it touches several folders
and resolves a code conflict. After that the planner re-cuts `blocked/05`.
