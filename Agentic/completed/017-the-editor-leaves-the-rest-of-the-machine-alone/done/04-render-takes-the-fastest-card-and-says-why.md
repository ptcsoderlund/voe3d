# 04 — render takes the fastest card and says why
folder: render
decisions: 0168, 0201, 0214

## Change
Files: `render/src/startup.h`, `render/src/card.c`, `render/src/device.c`, new
`render/tests/card.c`, `render/tests/tests.md`, `render/src/src.md`.

`startup.h` gains the facts and the ranking, pure, so a test needs no card:

```c
typedef struct {
	VkPhysicalDeviceType kind;
	uint64_t memory;      // bytes in the largest DEVICE_LOCAL heap
	bool vulkan_1_3;
	bool draws;           // a queue family with graphics
	bool presents;        // that family can present (ignored headless)
} voe_render_card_facts;

uint32_t voe_render_card_rank(const voe_render_card_facts *cards, uint32_t count,
			      bool headless, uint32_t *fastest);
```

Returns the index of the chosen card, or `UINT32_MAX` when none qualifies; `*fastest` is the
best-ranked card that has Vulkan 1.3 and draws, whether or not it presents, so the caller can tell
it fell back. Ranking by 0214: kind (discrete, integrated, virtual, other, CPU), then `memory`,
then the earlier index.

`card.c`:
- Choosing the card gathers the facts for every card the instance enumerates (properties, the
  largest device-local heap from `get_memory_properties`, a family that draws and one that also
  presents — `graphics_family` today answers only the second on a window, so it gains a parameter
  saying whether presenting is asked) into the arena, calls `voe_render_card_rank`, and sets `physical` and
  `queue_family` as before. The failure message when nothing qualifies stays.
- It then prints the one line 0214 describes, with the `render     ` prefix the current line has:
  name, kind in words (a CPU card is called a software rasteriser), how many cards there were,
  fastest or the fastest's name and that this is the next one down because it cannot present,
  the Vulkan version, and the rate — on a window drawing in step with the display, headless
  nothing presented. `say_which_card` goes; the choice is the one place that knows why.
- The header of card.c replaces "discrete first, then anything else" with the ranking and cites
  0201 and 0214.

`device.c`: `open_device` no longer calls `say_which_card`; `startup.h` drops it.

`render/tests/card.c` — new, needs no graphics card, includes `../src/startup.h`. Claims:
integrated, discrete, CPU in that order picks the discrete; the discrete unable to present picks
the integrated and reports the discrete as fastest; the same headless picks the discrete; a CPU
card alone is picked; two discrete cards pick the larger memory; a card without 1.3 or without a
drawing queue is never picked; none qualifying is `UINT32_MAX`. `tests.md` gains its entry.

## Done when
`checks.sh --folder render` exits 0 including test `render/card`, and `voe_dev` prints one `render`
line naming the card, its kind and the rate.
