# 0499 — Fog thickness reaches 4
date: 2026-10-30
by: tech-lead

## Decision
1. **A fog volume's `thickness` runs from 0 to 4.** The default stays 0.4 and 0 is still no fog. Above 4
   is refused as above 1 is today. Its slider in the Inspector runs 0–4, with no unit after the figure.
2. **What a thickness means does not change.** Density stays 0.1 × thickness² per metre, so 1 looks as
   it does now and 4 is 1.6 per metre, sixteen times as dense. Every fog volume saved before looks the
   same.
3. **Order.** This is work order 112, built after 110. The hill becomes 113 and stays 0.3's last work
   order.

## Reasoning
The sponsor finds thickness 1 too thin and wants it thicker. A wider range keeps every saved volume,
the hill's included, exactly as authored, and the squared mapping already gives fine control at the
thin end. Alternatives: a stronger mapping inside 0–1 (rejected: it silently thickens every saved fog
volume); thickness as metres of visibility (rejected in 0493, a larger number would be thinner fog).

## Replaces
Amends 0493 point 1 (thickness 0 to 1), 0498 points 2 and 5 (fog thickness is a 0–4 range with an
empty unit, not a fraction), and 0479 (the hill renumbered 111 → 113).
