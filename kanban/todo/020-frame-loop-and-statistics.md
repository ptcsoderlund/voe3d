# 020 — frame loop, honest timing, statistics to the console

status: todo
claimed-by: -
blocked-by: -

**Independent of every other card and can be pulled forward at any point.** It is
placed here because its value grows with how much there is to measure.

## Goal

A frame loop with real timing, printing honest numbers to the console.

## Why the console and not the screen

**There is no cheap way to put text on screen in this engine, deliberately.**
Everything is in 3D space (ADR-0049), so on-screen text goes through the full
texture-and-quad path like anything else. The principal's own note: more tools once
there is a GUI. Console now, on-screen when text properly exists.

## Scope

- **A real frame loop** with a measured delta time, not a fixed assumption.
- **Honest numbers.** CPU frame time and GPU frame time are different things and a
  single "fps" hides which one is the limit. GPU timing needs timestamp queries.
- **A rolling average and a worst case.** An average alone hides stutter, and stutter
  is what a person actually notices.
- **State clearly what is being measured**, in the output. A number whose meaning is
  ambiguous is worse than no number.

## This card decides frame pacing

The register carries it as open, waiting for exactly this card. **FIFO versus
`MAILBOX`** — the other thing "triple buffering" means: not waiting for the
display, discarding stale frames for lower latency.

- ADR-0050 deliberately left it open because it needs measurements to argue from,
  and this is the card where those exist.
- `MAILBOX` is **optional** — not every driver offers it, so a fallback to FIFO is
  required, not nice to have.
- Present the numbers with a recommendation. **This is a decision for the principal,
  not for the card** — report, do not choose.

## Verify

- Numbers are stable and plausible while nothing changes, and move when something
  does.
- Numbers are the same order of magnitude as an external measure — the compositor's
  own, or a stopwatch on a deliberately slow frame.
- `check.cmake` zero. Windows is the principal's, and the numbers will differ there,
  which is itself worth reporting.
