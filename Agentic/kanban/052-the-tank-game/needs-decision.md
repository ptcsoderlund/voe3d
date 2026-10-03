# Needs decision — is the root's README.md held to the folder-index rule?

Card 35 (`blocked/35-check-cmake-runs-its-parts.md`) is done: its `## Done when` passes. It blocked
because `checks.sh --folder .` reports 22 findings, all on `README.md`, all there on the clean tree
before the card. `checks.sh` treats `README.md` as the root's `<folder>.md`: one heading only, no
code blocks, every root code file and code subfolder listed. The README is the human's public front
page (0263: name, logo, disclaimer, quick start, contact, license), not an index, and no
card of this feature may change it or `checks.sh` (the workflow lives in `~/Projekt/agentic_rules`).
The same findings are likely to fail the `checks.sh --all` that `drive.sh` runs after the last card, whatever is planned.

## Options
1. **`checks.sh` exempts the root from the folder-index rule.** The human changes
   `agentic_rules` (skip the `<folder>.md` checks for `.`, keep the header-comment checks on root
   files) and runs `install.sh`. The README stays as 0263 has it; `ONBOARDING.md` already maps the
   folders for a reader.
2. **The README becomes the root's table of contents.** A card rewrites it to one heading, the
   logo, and a `- \`name\`` line per root code file and code subfolder; the disclaimer, quick start,
   contact and license move to `ONBOARDING.md`. Amends 0263 and changes the public page.
3. **The root reads its index from another file.** `checks.sh` takes the root's index from a named
   file (e.g. `ONBOARDING.md` or a new `voe3d.md`); a card writes that file. A workflow change as in 1,
   plus a card.

## Recommendation
Option 1. The README is written for people, not agents, and 0263 already fixes its shape; holding
it to the index rule costs the public page and gains a list `ONBOARDING.md` already gives. Then move
card 35 to `done/` as it stands, as its `## Blocked` says; no new card is needed.
