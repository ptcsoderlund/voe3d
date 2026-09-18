# Needs decision — how a new code folder gets registered in the root build

## The question

Card 05 is blocked because it cannot be split into cards that pass. The `theme/` folder (decided in 0170) needs
three edits outside `theme/`. They are its row in `cmake/voe.cmake`, `theme` added to `editor`'s row there, and
`add_subdirectory(theme)` in the root `CMakeLists.txt`. No card shape available today can carry those edits:

- **In the `theme/` card:** `checks.sh --folder theme` fails any diff outside `theme/`, and a card names one folder.
- **In a card of their own (`folder: cmake`, or the root):** `checks.sh --folder cmake` wants a `cmake/cmake.md`
  and runs `cmake --build … --target voe_cmake`, which does not exist. A root card fails the out-of-folder check
  on every path. The two edits also have to land in a set order. `voe_module(theme …)` does not configure until
  the row exists, and `add_subdirectory(theme)` breaks the configure until `theme/` exists. So even a card that
  passed would leave the tree broken between two commits.

Every future new folder hits the same problem, so this is a workflow question, not a question about 006.

## Options

1. **A new folder's card registers itself.** That card may also edit its own row in `cmake/voe.cmake` and add its
   one `add_subdirectory` line to the root `CMakeLists.txt`. It must name the decision that grants the edge, as
   0170 does here. A card that adds a folder to another folder's row (`theme` on `editor`'s) does the same edit
   from that folder's card, which is the `editor/` card here. Cost: `checks.sh --folder` in
   `~/Projekt/agentic_rules` has to allow those two files when the card in `doing/` has a `decisions:` line
   (the same test it already applies to dependency manifests), and then `install.sh`.
2. **The tech-lead makes the build edits when it writes the decision.** The map row and the editor edge go in
   with 0170, since rule 2 already calls an edge "reported, not made". The root `add_subdirectory` line then
   needs a `theme/` that configures, so the tech-lead also commits a stub folder: the four-line
   `CMakeLists.txt`, `theme.md`, and one header. No tooling changes. Cost: code outside any card, and a stub
   folder that `voe_module` may refuse because it has no sources.
3. **The root build finds folders itself.** It would glob every folder that has a `CMakeLists.txt`, and
   check.cmake step 1b would be dropped. The row in `cmake/voe.cmake` would still need option 1 or 2. Cost: this
   reverses a deliberate choice (step 1b's note in `check.cmake`) and only half-solves the problem.

## Recommendation

Option 1. Registration stays in the same commit as the folder it registers, the tree builds at every commit, and
the only change is one allowance in `checks.sh`, keyed to a named decision the way manifests already are. Once
this is decided, card 05 splits like this:

- `theme/`, with its row and its root line
- `editor/` × 3: themes and the chosen one, with `theme` on its row; then Preferences; then live re-reading
- `authoring/` × 1: take the line numbers from the parser

The last card makes `## How to test` true.
