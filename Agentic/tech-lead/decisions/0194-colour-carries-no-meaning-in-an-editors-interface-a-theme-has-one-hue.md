# 0194 — Colour carries no meaning in an editor's interface; a theme has one hue
date: 2026-09-19
by: tech-lead

## Decision
An editor's interface is monochrome in the sense of a green-screen or amber terminal: every role the
palette derives has the same hue, and roles differ only in lightness (chroma may only shrink where a
lightness needs it to stay in gamut, and is otherwise the same for all). No role is set apart by colour.
The accent and the ink that goes on it are gone. Selected, pressed, focused, hovered and dragged are
shown the way consoles showed them: inverted (light ground, dark ink or the reverse), a heavier
background, a border, an underline, a brighter or dimmer step. A theme authors that one hue as `hue=`,
which replaces `accent=` and takes the same colour value; its lightness is ignored, its hue and chroma
tint the whole palette, and a grey gives a neutral theme. Near black and Near white have zero chroma.
`accent=` is not read any more: a file that has it is refused like any other bad line (ADR-0172), with
no migration, because the only theme files that exist are the human's. Dark mode's harder chroma clamp
(ADR-0171) now applies to this hue. The rule covers the interface of every editor tool the engine ships,
including the runtime-GUI editor when it comes, and it is also where a cooked game's interface starts:
a game's GUI draws with a theme (ADR-0172), so by default it is one hue too. Setting properties per
element, and overriding the theme, will be how a game gives its interface colour on purpose; both are
later work, not this decision. The colour of an object in a level (ADR-0191) is content and not
covered. The first game builds its GUI in code, without a GUI editor. `ui`'s tests pin
the rule: every role of every derived palette, across both modes and both scalars' ranges, shares one
hue.

## Reasoning
The human agreed a monochrome look earlier but it was never recorded, so ADR-0097 and 0171 kept an
accent and the editor draws the selected row in blue. Meaning carried by colour is lost to anyone who
cannot tell the colours apart and to anyone whose theme changes them; lightness, inversion and shape
survive both. One hue keeps colour available to people who like it without letting it mean anything.
- **Pure greyscale, no hue at all**: rejected. The human wants people who like colour to have it, and a
  single tint means nothing, so it costs the rule nothing.
- **Keep the accent but forbid it from marking state**: rejected. A colour that exists will be used for
  something; removing the role is what enforces it.
- **Keep reading `accent=` as an alias**: rejected. Nobody else has a theme file, and an alias is a
  second spelling forever.

## Replaces
ADR-0097's and ADR-0171's accent: its place in the authored set, the accent and on-accent roles, and
"a pressed control and a number box being dragged are the accent". ADR-0172's list of required keys,
where `hue` takes `accent`'s place. The rest of 0171 (OKLab, lightness carries the palette, hue never
rotated, contrast as a constraint) stands.
