# 0349 — Several directional lights, each with its own fill and shadows
date: 2026-10-04
by: tech-lead

## Decision
A scene may hold several directional lights at once, such as a sun and a moon. Each has its own direction,
colour, intensity, fill, bounces and Cast shadows, all as one directional light has today, and each one's
light and fill add up. Game code can change any of them while playing, so a day and night cycle can turn the
sun's shadows off and the moon's on at night and back by day, leaving the moon a faint shadowless light in
the day. Every directional light that casts shadows costs its own shadow maps, so the cost is the scene's
choice (0316): a new one casts no shadows. Light blockers (0348) treat each directional light the same way.

## Reasoning
- One light with a day and night colour curve: no sun and moon together, no moon light in the day.
- A fixed limit of one casting light: a dusk with both casting is a fair look to pay for; the scene chooses.

## Replaces
Nothing. Carries out the promise in 0288.
