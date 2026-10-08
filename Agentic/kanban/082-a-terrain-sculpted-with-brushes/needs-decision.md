# Needs decision — four Best Practices ids only the Windows layer raises

Bug: `bugs/04-four-best-practices-warnings-assert-the-editor-on-close.md`.

## What was found while planning

The validation layer the checks run on (`~/voe3d-scratch/tools/sysroot`,
api_version 1.4.341) does not contain any of the four id names. A string
search of its `libVkLayer_khronos_validation.so` finds
`BestPractices-PushConstants` and `BestPractices-vkBindBufferMemory-small-dedicated-allocation`,
but none of the four below. So Linux cannot raise them, and no card's
`## Done when` can prove they are gone. Only the sponsor's Windows layer
raises them. A count of 10 is likely the layer's default limit per id, not
the real number.

Where each one comes from (found by grep):

- `vkCreateCommandPool-command-buffer-reset`: `render/src/device.c` creates
  the one command pool with `RESET_COMMAND_BUFFER_BIT`. The only reason is
  that `frame.c` resets the slot's command buffer by itself each frame.
- `vkAllocateMemory-small-allocation`: every buffer, image, target, shadow
  map and bounce resource has its own `vkAllocateMemory`. This is the same
  cause as the two `small-dedicated-allocation` rows already allowed ("no
  allocator yet").
- `pipeline-stage-flags2-compute`: `VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT`
  in barriers in `texture.c`, `target.c`, `target_own.c`, `target_read.c`,
  `bounce_volume.c` and `bounce_capture.c`. These are the "coarse barriers"
  0358's review named, and the `ImageBarrierAccessLayout` row already says
  the barrier review is its own card.
- `vkEndCommandBuffer-VtxIndexOutOfBounds`: `pass.c` binds the static
  geometry pools as every pass opens. A frame whose passes draw only
  elements (the interface) and no mesh ends with a vertex buffer bound that
  nothing used. This likely depends on whether a mesh is in view. The
  other three come from any project, with or without a landscape.

0358 says adding to the allowlist is a decision, and the bug says it is not
the planner's.

## Question

How are the four to be met, given that the layer the checks run on cannot
see them?

## Options

1. **Fix two, allow two.** Two cards in `render/src`:
   - each frame slot gets its own command pool, made without the reset
     flag and reset whole at the top of the frame;
   - the pools are bound at a pass's first mesh draw, not when the pass
     opens.
   A decision adds two allowlist rows, each with its reason as the existing
   rows give one: `vkAllocateMemory-small-allocation` (no sub-allocator
   yet) and `pipeline-stage-flags2-compute` (the barrier review waits for a
   measurement, 0358). On Linux the proof is grep plus the existing suite.
   The Windows close is the sponsor's to check.
2. **Allow all four.** No code. This is cheapest, but it keeps two costs
   that are easy to avoid.
3. **Fix all four now.** This adds a sub-allocator for all of render (VMA
   would be a new dependency) and a barrier review across six files. 0358
   rejected exactly that: fixing the review's findings before measuring
   them. Linux could verify neither.

Separately from these: the Windows layer is a different release from the
scratch one. If the sponsor installs the same release (1.4.341) on Windows,
these four may stop being raised with no code at all. Both machines would
then be gated alike. That is the programmer's environment (0004, 0384) and
can go with any option.

## Recommendation

Option 1, with the Windows layer matched to the scratch release. The two
fixes are real on any card and stay inside `render/src`. The two rows have
the same causes as rows 0358 already allows. When it is decided, replan this
bug: two cards, plus the allowlist rows if the decision adds them.
