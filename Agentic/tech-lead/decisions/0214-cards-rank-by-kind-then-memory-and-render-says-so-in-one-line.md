# 0214 — Cards rank by kind, then by memory, and render says so in one line
date: 2026-09-22
by: planner

## Decision
0201's "fastest card" is decided by `render` from two facts the driver gives
without measuring anything: the card's kind, then the size of its largest
device-local memory heap. Kind ranks discrete, then integrated, then virtual,
then any other, then a software rasteriser (`VK_PHYSICAL_DEVICE_TYPE_CPU`) last.
A card that does not claim Vulkan 1.3 or has no queue that draws is not ranked at
all. On a window the chosen card is the best-ranked one whose queue can also
present to the surface; headless, presenting is not asked. The ranking is a pure
function over those facts, so a test can prove it with no graphics card.

The one startup line is `render`'s, printed where the card is chosen, and it
carries: the card's name, its kind in words (a software rasteriser is named as
one), how many cards there were, that it is the fastest or — when the fastest
could not present — which card that was and that this one is the next one down,
the Vulkan version, and the rate: on a window, that it draws in step with the
display (the device opens on FIFO, ADR-0131); headless, that nothing is
presented. The rate is the present mode, not a number of hertz: nothing in the
engine knows the display's refresh and a figure measured at startup would be a
guess.

## Reasoning
Kind alone cannot tell two discrete cards apart and cannot be made to by the
enumeration order, which 0201 rules out; the device-local heap is the one number
every driver reports that tracks how big a card is. Benchmarking at startup was
rejected as slow and noisy. Printing hertz would need `platform` to bind
`wl_output` and follow the window between outputs, for a figure the programmer
can already read off the display settings.

## Replaces
nothing
