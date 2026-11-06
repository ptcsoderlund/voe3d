# Agents in projects

## Why
- The editor is a product the sponsor ships; he cannot and should not dictate whether its users use AI tools. He wants agents to cowork with developers in game projects, not only on code, such as changing a light's strength or colour through the sectioned files: where the trend points, and what he wants for his own future games. [intent · tech-lead · 2026-11-06] see 0505

## The format
- The sectioned format is simple but each section's keys are strict: a light cannot take `texture=`. A reference to the strict types is what lets an agent write files the editor accepts. [intent · tech-lead · 2026-11-06] see 0505

## Outside changes
- A file with no unsaved changes is taken in silently as one undo step: the tech-lead's choice; the sponsor spoke only of the unsaved case. [assumed · tech-lead · 2026-11-06] see 0506

## The reference
- The editor rewrites `Docs/` on each open, so the reference is true to the editor that last opened the project. [assumed · tech-lead · 2026-11-06] see 0506

## This repository's agents
- 0376 and 0251 still bind this repository's own agents: the coin game's levels stay the sponsor's hand; the hill, now outside the repository, he makes with or without agents. The tech-lead's reading of the sponsor's "probably a misinterpretation"; the sponsor did not say it. [assumed · tech-lead · 2026-11-06] see 0505, 0507
