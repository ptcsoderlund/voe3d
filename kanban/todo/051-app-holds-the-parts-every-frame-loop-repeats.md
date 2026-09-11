# 051 — `app` exists: the parts every program's frame loop repeats

claimed-by: -
blocked-by: -
status: todo
decision: *`app` is parts a program calls in its own loop* (ADR-0135) — startup, frame open, draw open and close; `app` holds no loop, calls nothing back, and does not own system order, the world, arenas, the readout, key bindings or the present mode.

## Goal

A new library folder `app/` with the startup of a window and a device, the opening of a
frame, and the opening and closing of a draw. Nothing calls it yet; card 052 moves `dev`
onto it.

## Scope

**1. Wiring.**

- `app/CMakeLists.txt`, the four lines: `voe_module(app DEPENDS render platform math base)`.
  The `app` row in `cmake/voe.cmake` already allows these — do not edit the row.
- Root `CMakeLists.txt`: `add_subdirectory(app)` after `ui` and before `dev`.
- `app/app.md`, the folder page: what the folder is and its public surface (ADR-0133).

**2. `app/include/app/clock.h`, `app/src/clock.c` — the frame's interval, as a value.**

```c
typedef struct { double previous; bool started; } voe_app_clock;
typedef struct { double now; double elapsed; double step; bool first; } voe_app_tick;

voe_app_tick voe_app_clock_tick(voe_app_clock *clock, double now, double longest_step);
```

- `elapsed` is `now` minus the previous tick's `now`; `step` is `elapsed` clamped to
  `longest_step`.
- The first tick on a zeroed clock: `first` true, `elapsed` and `step` zero.
- `longest_step` at or below zero asserts. No `platform` call here — the caller passes `now`.

**3. `app/include/app/app.h`, `app/src/app.c` — the three parts.**

```c
typedef struct voe_app voe_app;

typedef struct {
	int width;
	int height;
	const char *title;
	voe_render_capacities capacities;
	double longest_step;
} voe_app_settings;

[[nodiscard]] voe_app *voe_app_new(voe_base_arena *arena, voe_base_arena *scratch,
				   voe_app_settings settings, voe_base_error *error);
void voe_app_destroy(voe_app *app);
voe_platform_window *voe_app_window(voe_app *app);
voe_render_device *voe_app_device(voe_app *app);

typedef struct {
	voe_app_tick tick;
	voe_platform_size size;
	bool minimised;
	bool closing;
} voe_app_frame;

voe_app_frame voe_app_frame_open(voe_app *app);

[[nodiscard]] bool voe_app_draw_open(voe_app *app, voe_platform_size size,
				     voe_render_view view, voe_render_light light,
				     bool *drawing);
[[nodiscard]] bool voe_app_draw_close(voe_app *app);
```

- **`voe_app_new`**: the struct in `arena`; `voe_platform_window_new`, then
  `voe_render_device_new` with `scratch`, the window's native handle and its size. A window
  that does not open is `VOE_BASE_ERROR_UNAVAILABLE` with a line on stderr; a device that
  does not start passes its own error through and the window is closed. NULL either way.
- **`voe_app_destroy`**: the device, then the window. The struct's memory is the arena's.
- **`voe_app_frame_open`**: `voe_platform_clock_now`, the tick, `voe_platform_window_poll`,
  then the size and `voe_platform_window_should_close`. `minimised` is a size with no area.
- **`voe_app_draw_open`**: `voe_render_frame_begin` with what it is given; false, with `the
  GPU stopped answering` on stderr, when that fails. **`voe_app_draw_close`**:
  `voe_render_frame_end` on the same terms, called only when `drawing` came back true.
- Each header says, in the style `render/include/render/device.h` uses, what the part does
  and what it leaves to the program.

**4. Tests — `app/tests/clock.c`.** No window. The first tick; an ordinary interval; an
interval longer than `longest_step` gives the raw `elapsed` and the clamped `step`; two ticks
at the same `now` give zero.

## What must not change

- **No loop, no callback and no function pointer in `app`.** A program writes its own
  `while`.
- **Not in `app`**: the order of systems, a world or its registrations, arenas beyond the two
  handed in, the timing readout, key bindings, the present mode. `voe_app_new` requests no
  present mode — the device opens on FIFO (ADR-0131).
- `dev` is not edited — card 052. The `app` row in `cmake/voe.cmake` is not edited.

## Verify

- Linux: `cmake -P check.cmake` green, including `app` configuring standalone.
- `ctest -R app` passes.
- `grep -rn 'ecs/\|scene/\|3d/' app/include app/src` returns nothing.
- From the planning root, `bash tools/hot.sh` reports no new `OVER`.

## Done looks like

A folder with three parts and a clock tested with no window, from which a program's frame
loop is a dozen lines of its own.

## Notes
