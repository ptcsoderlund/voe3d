# 0352 — A blocker's kind is named Block: All, Fill or Direct
date: 2026-10-04
by: tech-lead

## Decision
A light blocker's Kind field is shown and saved as **Block**, and its values are named by what the box stops:
**All** (was Room), **Fill** (was Indoors) and **Direct** (was Wall). Behaviour is exactly 0348's and 0350's;
only the names change. A new blocker is still Block All. A scene saved before the rename opens with every
blocker as it was. The Inspector's description of Block All says that lights inside it also stay inside.

## Reasoning
The sponsor's call (2026-10-04): the kinds should say what they do, not one use of them — a Direct blocker is
also a free-standing screen, a Fill blocker also a cave mouth. Rejected: Wall/Indoors/Room, names of a use;
the values All/Fill/Light under "Kind", where "Kind: Fill" reads as adding fill and "Light" as a light source.

## Replaces
Amends 0348 and 0350: the names only.
