# 0006. Pre-study execution order; the conventions gate gets narrowed

**Rule:** The pre-study executes in a fixed order — **capability, then
structure, then scaffolding, then conventions.** No artifact is produced before
the thing it derives from exists.

The README exit criterion *"core C conventions decided"* is narrowed: it gates
**engine logic**, not **build plumbing**. Scaffolding — CMake files, the check
script, placeholders that compile and link — may be written before the
conventions are settled.

**Status:** Accepted · 2026-08-28
**Amends:** the exit criteria in `docs/prestudy/README.md`. The criteria
themselves stand; only their ordering and the scope of the conventions gate
change.

**Why:** Delegated by the principal, who declines to track phase structure and
reads only `STATUS.md`. The substantive reason it is safe: build plumbing
encodes no error-handling, allocation or interface decision, so a later
convention cannot make it *wrong* — only cosmetically inconsistent. Gating it
behind the conventions is a false dependency, and an expensive one: it stalls
all verification of ADR-0001's standalone-configure property and ADR-0005's
compiler guard behind decisions that have no input yet.

The ordering also fixes a real defect in the previous plan, which put the module
map first. `guidelines.md` requires grouping by what code *does*; with no
feature spec, that grouping would have been drawn from taste.

**Cost accepted:** Scaffolding written before D-012 may need cosmetic rework —
file and target renaming — once naming conventions land. Cheap, visible, and
confined to files that contain no logic.

**Consequence for the principal's interface:** `STATUS.md` is the only document
written for the principal. Phase structure, `D-` ids and register state never
appear in it, and are never quoted at the principal in conversation.
