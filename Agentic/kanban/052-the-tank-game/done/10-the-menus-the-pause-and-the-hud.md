# 10 — The menus, the pause and the HUD, by pad, keyboard or mouse
folder: examples/tank_game/Code
after: 02, 09
decisions: 0168, 0259, 0333, 0334

## Change
0334 points 7 and 8, through 0333's asks. Read `examples/coin_game/Code/game_interface.c` (the
coin game's screens: the model to follow), in this folder `tank_state.h`,
`tank_state_system.c`, `tank_lives.h`, `tank_lives_system.c`, `tank_control_system.c`
(the pad slot), `project.c`, `Code.md`, and the headers of `game/include/game/project.h`,
`platform/include/platform/input.h` and `ui/include/ui/widgets.h` (`voe_ui_choice_begin`).

- `examples/tank_game/Code/tank_state.h`: the row gains `selected` (the chosen item) and
  `held` (last frame's levels, a bit each: up, down, press, pause, back); declares
  `bool tank_menu_run(const voe_game_project_frame *frame)`.
- `examples/tank_game/Code/tank_menu.c`, new: each frame, one root over `frame->size`; with no
  row it ends the frame, sets `asks->paused` false and returns true. Else the phase's screen:
  menu (Start, Quit); playing, the HUD top left `Score N` and `Lives N`; paused (Resume, Menu,
  Quit); won (`You win`, the score, Menu, Quit); lost (`Game over`, the score, Menu, Quit),
  each item a `voe_ui_choice_begin` selected when it is the chosen one, centred. Keys: W, S, the
  d-pad and the left stick past 0.5 move the choice, wrapping; Enter, Space, the pad's south
  button or a click press it; Escape or Start pauses while playing (choice 0); paused, Escape,
  Start or east resumes. Edges against `held`, which is written each frame; headless no key is
  down. Start and Resume set playing; Menu sets `asks->restart`; Quit returns false. Then
  `asks->paused` is whether the phase is paused, won or lost. The row written whole. Header:
  the screens, the keys, why edges, one writer with `tank_state_system.c`.
- `examples/tank_game/Code/tank_lives.h`, `tank_lives_system.c`: `tank_lives_interface`
  and its HUD go; the headers say the menu's HUD shows the lives.
- `examples/tank_game/Code/project.c`: `voe_game_project_interface` returns
  `tank_menu_run(frame)`; the header's interface sentence.
- `examples/tank_game/Code/Code.md`: the menu's entry; the lives, state and project entries.

## Done when
`(for f in examples/tank_game/Code/*.c; do clang -std=c23 -fsyntax-only -DVOE_BASE_DESCRIPTIONS=1
$(for d in */include; do printf -- "-I%s " $d; done) "$f" || exit 1; done)` exits 0, and `! grep -rq tank_lives_interface examples/tank_game/Code` exits 0.
