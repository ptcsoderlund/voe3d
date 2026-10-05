# 02 — The calls between render's files leave device_internal.h
folder: render
after: 01
decisions: 0168

## Change
`render/src/device_internal.h` is 788 lines, and cards 05 and 09 add to it. This card splits it by
what each part does. Nothing changes behaviour. Read its header, and the section from the end of
`struct voe_render_device` (the line after `bool gpu_measured;` and `};`, about line 468) to the
end of the file.

- `render/src/device_calls.h` (new): every declaration after the device struct moves here
  unchanged, each with the comment over it: swapchain.c's, target.c's and the rest, through the
  probe draw. It has `#pragma once` and includes only what its declarations name. It is not
  included on its own. Header points:
  - the calls one file of `render/src` makes into another, grouped by the file that owns each;
  - the device struct is in `device_internal.h`;
  - a new call between files is declared here, beside its owner's group.
- `render/src/device_internal.h`: it keeps its constants, `device_parts.h` and the device struct,
  and includes `device_calls.h` at its end, so every file that includes it sees the same
  declarations. The header's line on where the split runs names the new file.
- `render/src/src.md`: a `device_calls.h` entry, and the `device_internal.h` entry now says it holds
  the device struct and the constants. Each entry under 300 characters.

## Done when
`[ $(wc -l < render/src/device_internal.h) -lt 520 ] && grep -q '"device_calls.h"'
render/src/device_internal.h` exits 0, and the tests `render/passes`, `render/shadow` and
`render/bounce_shadow` pass after the folder's build.
