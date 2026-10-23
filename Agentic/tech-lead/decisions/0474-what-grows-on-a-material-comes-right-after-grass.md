# 0474 — What grows on a material comes right after grass
date: 2026-10-23
by: tech-lead

## Decision
095 (each material chooses what grows on it, 0473) is built right after 094, before anything else, as
it rewrites 094's grass setup. The queue is renumbered: 096 compressed textures (was 095), 097 models
blend into the ground (was 096), 098 the hill shipped (was 097), 099 snap (was 098), 100 docking panels
(was 099). Old numbers in earlier decisions mean these.

## Reasoning
The sponsor wants it as soon as possible; building textures or blending on top of a grass setup that is
about to go would be work redone. Alternative: reopening 094, which would hold its nearly finished cards.

## Replaces
Nothing. Renumbers 0467.
