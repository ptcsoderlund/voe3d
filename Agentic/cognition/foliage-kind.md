# Foliage kind

## What names a kind
- Nothing in `.landscape` or the Foliage section stops two kinds of one landscape naming the same `.glb`, so a model path does not name a kind uniquely. [observed · planner-108 · 2026-10-30]
- A landscape holds at most 16 kinds. [observed · coder-108-04 · 2026-10-30]
- The editor can add a kind (an Assets drop, with no undo step) but cannot remove one, so a fold kept by kind index never shifts today. [observed · planner-108 · 2026-10-30]

## Its box
- "I cannot fold foliage kind in inspector": the sponsor wants each foliage kind's box to fold (108). [intent · tech-lead · 2026-10-30]
- A kind's drop target is its title row, not the whole box. [observed · accept 108 · 2026-10-30]
- A folded kind's look is not refreshed, which is safe only while nothing reads it when folded. [observed · coder-108-06 · 2026-10-30]
