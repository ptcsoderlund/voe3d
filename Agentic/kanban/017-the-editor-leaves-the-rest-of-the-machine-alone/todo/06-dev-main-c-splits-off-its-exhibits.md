# 06 — dev's main.c splits off its exhibits
folder: dev
decisions: 0168

## Change
`dev/src/main.c` is 2861 lines and card 09 changes it. This is the first of three cards that bring it
under 800; it moves code and changes nothing the program shows. Read main.c's header first. Each
function moves with the comment block and the `#define`s above it that only it reads; a constant
`main()` also reads stays in main.c.

- `dev/src/cubes.h`, `cubes.c` — gain `add_cube`, `textured_material`, `add_the_cubes` (today at
  main.c lines 652–815) and the embedded pictures they upload. cubes.h's header widens from the
  geometry to the two cubes as an exhibit.
- `dev/src/quad.h`, `quad.c` — gain `quad_material`, `quad_at`, `add_panel`, `add_quad`,
  `add_the_quads` and the "two in the world / three in the overlay" paragraphs (816–1197). Header
  widened the same way.
- `dev/src/text.h`, `text.c` — new. `add_text` (1198–1367).
- `dev/src/facing.h`, `facing.c` — new. `struct view_basis`, `basis_of`, `square_to_the_camera`,
  `facing_the_camera`, `top_left_of_the_view`, `behind_the_line` (1368–1538): the transforms that
  turn an exhibit to the camera. Its header says who calls them each frame.
- `dev/src/model.h`, `model.c` — new. `add_a_model` (1539–1692) and the embedded `.glb`.

A moved function loses `static` and takes a `voe_dev_` name; its call sites in main.c follow. Each
new header says what the file owns and why it is its own file, in dev's style (see `sprites.h`).
Where main.c's header points at a paragraph that moved, it points at the new file instead.

`dev/src/src.md` gains the new files and widens the entries for `cubes` and `quad`.

## Done when
`checks.sh --folder dev` exits 0, `wc -l dev/src/main.c` is under 1800, and `voe_dev` shows the
same cubes, quads, writing and model as before this card.
