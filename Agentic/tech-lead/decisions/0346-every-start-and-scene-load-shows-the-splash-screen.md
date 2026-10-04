# 0346 — Every start and scene load shows the splash screen
date: 2026-10-04
by: tech-lead

## Decision
While the editor or a game starts, and while either loads a scene, the window shows a splash screen
until the work is done. There is no minimum time, so a fast load only flashes it.
- **The image.** The engine's splash is `engine_assets/Engine images/splashscreen.png`. A card copies
  it, unchanged, into the code folder that uses it, and the original stays where it is (0263 holds).
  The editor always shows the engine's splash.
- **A game's own splash.** A game shows its project's `Assets/splashscreen.png` if there is one, and
  otherwise the engine's. The chosen PNG ships unchanged beside the game's program, as the WAVs do (0266).
- **The layout.** The image is centred and scaled to fit the window without cropping or stretching.
  Space around it is filled with the image's own edge colour. Across the bottom middle sits a box in the
  theme's background colour, holding the status line ("Starting - preparing shaders...") in the theme's
  text colour. The editor uses its own theme for the box; a game uses the theme its GUI uses (0194).
- **Where it replaces the plain screen.** The splash takes the place of 055's plain themed start
  screen. If no PNG can be read, the plain screen of 055 is shown instead.
- **When it shows.**
  - Editor start, and the editor's New and Open of a scene.
  - Game start: Play and a shipped game.
  - A game's change from one scene to another, once games can do that.

The splash's settings, such as the box's place, a minimum time, a fade and a per-scene image, may become
customisable later. Each of those is a new decision.

## Reasoning
The human wants one recognisable screen wherever the engine is busy, easy to swap, with room to make it
customisable later. Alternatives:
- Reading the PNG straight from `engine_assets/` at build time: the human chose to keep that folder as
  source only.
- An engine-only image for every game: a shipped game should not have to open on the VOE3D logo.
- Embedding the image in the program: unlike the WAVs, it could not be swapped without a rebuild.
- A forced minimum time: it delays every fast load for no gain.

## Replaces
nothing. It amends 055's starting screen (0345): the plain themed background with its centred line
becomes the splash, and the line moves into the bottom box.
