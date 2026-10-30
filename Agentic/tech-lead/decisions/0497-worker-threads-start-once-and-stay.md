# 0497 — Worker threads start once and stay
date: 2026-10-30
by: tech-lead

## Decision
1. **One standing set of workers.** The program starts its worker threads once, when it opens, and ends
   them once, when it closes. Every piece of parallel work after that runs on those threads: bursts
   for model and material loads, the model watch, texture compression jobs, and anything later. No
   thread is created or ended between opening and closing. How many workers there are is the
   planner's choice, as long as the count is fixed for the run, and loads stay at least as fast as now.
2. **This holds everywhere the engine runs:** the editor, Play, and a shipped build, on Windows and
   Linux.
3. **Order.** This is work order 110, built after 108. The hill becomes 111 and stays 0.3's last work
   order.

## Reasoning
Today every burst creates its threads and joins them, so the editor's once-a-second model watch alone
starts and ends about four threads per model every second. Outside a debugger that is cheap and goes
unseen. Under a debugger on Windows each start and end pauses the process, and the editor stutters for
seconds. The sponsor debugs in CLion on Windows, so this matters, and a standing set of workers is the
usual design for an engine anyway. Alternatives: only fixing the watch to check the file's stamp first
(done separately as a bug fix, and leaves every load and compression job creating threads); a pool
that grows and shrinks (still creates threads mid-run).

## Replaces
Amends 0496 point 4 and 0479 (the hill renumbered 109 → 111).
