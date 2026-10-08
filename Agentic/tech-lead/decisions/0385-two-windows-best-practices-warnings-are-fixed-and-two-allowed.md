# 0385 — Two Windows-only Best Practices warnings are fixed and two allowed
date: 2026-10-08
by: tech-lead

## Decision
For 082 bug 04. Two of the four warnings that only the sponsor's Windows layer raises are fixed in `render/src`:
`vkCreateCommandPool-command-buffer-reset`, by giving each frame slot its own command pool, made without the
reset flag and reset whole at the top of its frame; and `vkEndCommandBuffer-VtxIndexOutOfBounds`, by binding the
static geometry pools at a pass's first mesh draw instead of when the pass opens. The other two join `render`'s
allowlist, each with its reason in the same form as the existing rows: `vkAllocateMemory-small-allocation` (no
sub-allocator yet, the same cause as the two `small-dedicated-allocation` rows) and `pipeline-stage-flags2-compute`
(the barrier review waits for the frame breakdown's measurement, 0358). On Linux the fixes are proven by grep and
the existing suite, because the scratch layer (1.4.341) cannot raise these ids; the Windows close is the sponsor's
to check, on the sponsor's own layer (1.4.304). No validation layer release is required of anyone: VOE3D is an
open-source engine and its contributors run whatever layer they have. The allowlist may therefore hold ids that
only some releases raise, and a row the checks' own layer cannot raise is kept, not pruned; a warning that only
another release raises is a bug like any other, met by a fix or a row as here.

## Reasoning
The two fixes are cheap, stay in one folder and keep per-frame work flat (0386). A sub-allocator and the barrier
review are real work that 0358 says waits for measurement, and Linux could verify neither.
- Allow all four: no code, but keeps two costs that are easy to remove.
- Match the Windows layer to the scratch release: rejected by the sponsor; an open-source engine does not pin its
  contributors' tool versions.
- Fix all four now: a sub-allocator (likely VMA, a new dependency) and a six-file barrier review before measuring,
  which 0358 rejected.

## Replaces
Nothing. Adds two rows to 0358's allowlist.
