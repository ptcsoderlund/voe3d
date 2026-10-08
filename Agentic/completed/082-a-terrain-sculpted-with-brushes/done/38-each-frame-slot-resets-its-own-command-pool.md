# 38 — Each frame slot resets its own command pool
folder: render/src
after: none
decisions: 0168, 0385

## Change
Bug 04: the sponsor's Windows layer raises
`BestPractices-vkCreateCommandPool-command-buffer-reset` once, because the
device's one command pool is made with `RESET_COMMAND_BUFFER_BIT` so that
`frame.c` can reset a slot's command buffer by itself. Per 0385 each frame
slot gets its own pool, made without that flag and reset whole at the top of
its frame. The device's pool stays, without the flag, for the one-shot
uploads `buffer.c` allocates and frees (it is the only other user). The
scratch layer cannot raise this id; the proof here is grep and the suite.

- `render/src/loader.h`, `render/src/loader.c`: add `reset_command_pool`
  (`vkResetCommandPool`) beside the other command-pool entries. Remove
  `reset_command_buffer` (`vkResetCommandBuffer`): `frame.c` is its only
  caller and stops calling it.
- `render/src/device_parts.h`, `struct voe_render_frame`: a
  `VkCommandPool pool` beside `commands`. Comment points: this slot's
  command buffer comes out of it; reset whole once its fence says the GPU is
  done with the slot; per slot for the reason the command buffer is.
- `render/src/device_internal.h`, the comment above the device's `pool`: it
  is now the uploads' pool, not the one behind every slot; made without the
  reset flag because nothing in it is reset alone; the guard (ADR-0370)
  still keeps another thread's upload apart from an open frame.
- `render/src/device.c`, `create_frame_objects`: the device's pool is made
  with no flags. In the per-slot loop, each slot's pool is made with no
  flags (graphics queue family as today) and its one command buffer
  allocated from it; the one-call-for-every-slot allocation and its comment
  go. Close-down destroys each slot's pool where it destroys the slot's
  other objects, null-safe as they are; the device's pool as today. The
  comment above `create_frame_objects` no longer says one pool is behind
  all of it.
- `render/src/frame.c`, `voe_render_frame_begin`: where the slot's command
  buffer is reset after the fence reset, reset the slot's pool instead
  (flags 0), same place, same order. Header comment unchanged unless it
  names the buffer reset.

## Done when
- `! grep -n "RESET_COMMAND_BUFFER_BIT\|reset_command_buffer" render/src/*.c render/src/*.h`
  exits 0.
- `grep -n "reset_command_pool" render/src/frame.c` prints one line.
- `bash ~/.claude/skills/checks/scripts/checks.sh --folder render/src`
  prints FINDINGS: 0 (its suite includes `render/best_practices`, the close
  gate on a headless frame).
