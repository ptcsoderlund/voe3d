# 03 — Every pass timed and labelled, read back as a frame breakdown
folder: render
after: 02
decisions: 0168, 0358, 0367

## Change
- `render/include/render/device.h`, timing section: `#define VOE_RENDER_PASS_NAME 32`;
  `typedef struct { char name[VOE_RENDER_PASS_NAME]; double seconds; } voe_render_pass_time;`;
  `[[nodiscard]] uint32_t voe_render_frame_pass_times(const voe_render_device *device,
  voe_render_pass_time *times, uint32_t capacity)` — the passes of the newest measured frame in the
  order run, at most `capacity`, nought when none or no timestamps. Its comment: the same frame
  and lag as `voe_render_frame_gpu_time`; a pass not run is absent; names are 0367 point 1's.
- `render/src/device_parts.h`: the frame record's pool comment and room for the names of this
  frame's timed passes and their count. `render/src/device_internal.h`: the device holds the last
  read breakdown and its count. `render/src/device_calls.h`: `VOE_RENDER_TIMESTAMPS_PER_FRAME`
  becomes 0367 point 2's count from the capacities (rename to a call or keep the frame pair's two
  as a constant beside it), and the new file's group.
- New `render/src/pass_timing.c`: `voe_render_pass_timing_open(device, frame, name)` writes the
  pass's first timestamp and begins the debug label; `voe_render_pass_timing_close(device, frame)`
  writes the second and ends the label; `voe_render_pass_timing_read(device, frame)` turns the
  pairs into seconds with the valid-bit mask and period `frame.c` uses; and
  `voe_render_frame_pass_times`. A pass past the pool's room is not timed, never an assert.
- `render/src/device.c`: the pool sized per 0367 point 2; the names' room made and freed with the
  slot.
- `render/src/frame.c`: reset this frame's timed count and the whole pool at begin; read the
  passes beside `read_gpu_time`, from the same results.
- `render/src/frame_internal.h` and `render/src/pass.c`: `voe_render_pass_start` takes a
  `const char *name` and opens the timing; `voe_render_pass_end` closes it. `pass.c` names its
  camera, interface and shadow passes; `render/src/point_shadow.c`,
  `render/src/bounce_capture.c` and `render/src/bounce_shadow.c` pass theirs (0367 point 1).
- `render/src/src.md`: an entry for `pass_timing.c`; `frame.c`'s and `pass.c`'s entries name it.
- New test `render/tests/pass_times.c` (headless, `shadow_size` set, `passes` 8): frames with a
  shadow pass on cascade 0 then a camera pass onto the window, run past
  `VOE_RENDER_FRAMES_IN_FLIGHT`; the breakdown lists `shadow light 0 cascade 0` then `view window`,
  each above nought, their sum not above the frame's GPU time; frames after with no shadow pass
  list `view window` alone; a capacity of 1 writes one. When `voe_render_frame_gpu_time` never
  answers, it says so and passes. Entry in `render/tests/tests.md`.

## Done when
- `ctest --test-dir build/debug -R '^render/pass_times$'` passes, and the folder's tests pass.
