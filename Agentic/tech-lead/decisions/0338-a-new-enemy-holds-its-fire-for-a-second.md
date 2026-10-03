# 0338 — A new enemy holds its fire for a second
date: 2026-10-03
by: planner

## Decision
For 052 bug 03: the `Tank / Enemy` row's default `wait` is 1 s, not 0. Every enemy row starts
from that default, a wave's spawn and one placed in the level alike, and its system counts `wait`
down every playing step as before, so an enemy fires no sooner than 1 s after it appears. It
turns its turret and drives in that second as before. After its first shot, `wait` is 1 / `rate`
as before. `wait` stays read-only; no new field, no new constant.

## Reasoning
The fire rule already holds an enemy until `wait` is spent, and a row's missing fields come from
its default, in the editor's Play and in the shipped game alike. A default of 1 is the whole rule
in one number, with no spawn-time write and nothing for the spawner to know.

## Replaces
Nothing. Amends 0294 point 5 and 0297 (an enemy's first shot waits 1 s).
