# 005 — The editor draws real pixels on a scaled Windows monitor
folder: platform

## Seen
On Windows with a 4K monitor and 125% text scaling, the editor runs smoothly in a small window. Maximized,
it freezes for one to two seconds every now and then while CPU and GPU sit nearly idle and memory has
room. The hitch log puts the time in whatever span happens to be running (`step`, `undo`, `end`):

```
editor: hitch: frame 2156.3 ms (mean 105.2): step 1872.8, views 143.1, shadows 87.4, pace 72.3, interface 29.5, light bounces
editor: hitch: frame 2302.2 ms (mean 151.0): step 1545.5, undo 362.8, pace 178.0, views 155.8, shadows 98.5, light bounces
```

Setting the exe's Properties → Compatibility → "Override high DPI scaling behaviour: Application" makes
the freezes go away entirely. The editor then only loses some frame rate at 4K.

## Expected
The editor tells Windows it handles scaling itself on every monitor. Its window reports the monitor's
real pixels, the swapchain is that size, and Windows never stretches the editor's frames. Maximized at 4K
it runs without freezes and without the compatibility override.

## Change
The process declares no DPI awareness (`platform/src/seat_win32.c:334`), so on a scaled monitor Windows
gives the window a smaller virtual client size and bitmap-stretches every presented frame. That stretch
is where the stalls come from.

- Declare the process per-monitor DPI aware (v2) before the first window is made, in the Windows
  platform code. If the call is not available (an older Windows), fall back to the older per-monitor or
  system awareness and keep going.
- `platform/src/window_win32.c:58-65` (WM_SIZE) already stores the client size and re-clips the cursor.
  With awareness on, that size is in real pixels. Check that every other place in `platform/src` that
  reads or sets window, cursor or client coordinates agrees with that (cursor clip, pointer position,
  raw motion, the window's first size).
- Handle `WM_DPICHANGED`: take the rectangle Windows suggests, so moving the window to a monitor with
  another scale keeps it sensible, and the next WM_SIZE carries the new real size.

Only `platform/` changes. The editor's interface already sizes from pixels per mm worked out from the
window height, and the swapchain and view targets already follow the client size.

## Done when
- The platform check builds and passes on Linux, and the Windows files compile in the commit (the human
  builds on Windows).
- `platform/platform.md` says the Windows window is per-monitor DPI aware and reports real pixels.
- On Windows, with the compatibility override switched off again, the maximized editor on the 4K monitor
  is sharp, its window is reported at the monitor's full resolution, and flying into the forest gives no
  freezes (checked by the human).
