// voe_dev — the one program a person runs to see what the engine can currently
// do. Today it opens a window holding a world: two cubes placed by hand, a
// model read out of a `.glb` file, two see-through quads standing either side of
// them, a lettered sign above them, a line of writing locked to the camera, a
// screen off to the left showing a second camera's view of the same world, one
// sun going round it all, and a camera that either orbits them or is
// flown. There is one of these and it always shows the
// current state, so what is here now is expected to be deleted rather than kept
// behind a flag when the next thing lands.
//
// IT IS A CALL SITE AND EVERYTHING IN IT IS WIRING. What is here is which key
// means which direction, where a placeholder cube stands, and the loop that runs
// the systems in order. Anything in it that starts to look worth keeping belongs
// in a folder, with a test — the moment it is worth testing it is in the wrong
// place.
//
// THE CLOCK IS REAL NOW AND CARD 020 IS WHAT MADE IT ONE. Every frame is stepped
// by however long the last one actually took, and not by a nominal sixtieth of a
// second — so the orbit takes the number of seconds it says it does on a display
// of any refresh rate. Since card 052 the reading and the subtraction are
// voe_app_frame_open's: it hands back both numbers, the interval that happened
// and that interval clamped, and this file reports the first and steps the scene
// by the second. MAX_FRAME_SECONDS is the ceiling it is handed, and nothing
// clamps what is reported.
//
// AND IT PRINTS WHAT IT MEASURED. Four numbers every couple of seconds, each an
// average and a worst over exactly that period: the frame, this program's own
// work, the draw, and the graphics card's own clock. say_what_is_measured() is
// the legend and it is printed once at startup, because a number whose meaning
// is ambiguous is worse than no number. P switches between the two present
// modes, which is the measurement the frame-pacing decision is waiting on.
//
// AND SINCE CARD 028 IT DRAWS THEM TOO. The same numbers stand in the top-left
// of the view as the period's running averages, rebuilt every frame as a text
// block through voe_text_block_create_transient and never cached: a readout that
// remembered its string until it changed would, the day its invalidation missed,
// show yesterday's numbers and look exactly like a frozen program. The console
// block stays, because a period's worst is a different, still-useful thing.
//
// THE LOOP OWNS THE FRAME (ADR-0098), AND SINCE CARD 052 ITS PARTS COME FROM
// `app` (ADR-0135). voe_app_frame_open opens the frame, voe_app_draw_open and
// voe_app_draw_close bracket the draw, and the phases between them run in a
// fixed order: open; build what changes this frame, which is the readout and the
// two panels' records; a pass onto the monitor's target with the monitor's
// camera, walked and closed; a pass onto the window with the world's camera,
// walked and closed; close, which presents. Building comes after the open
// because geometry that lives one frame can only be built once the frame's slot
// is known, and before both passes because both of them walk the same world —
// something built after the first pass would be missing from that pass and stale
// in it the frame after. An open that says there is nothing to draw into — a
// window with no area, a swapchain that has just gone stale — skips all of it;
// that case is the loop's and not the draw system's.
//
// THE MONITOR'S PASS IS FIRST BECAUSE THE WINDOW'S PASS SHOWS WHAT IT DREW. A
// target written after it has been read in the same frame would put last frame's
// picture on the screen, and reading and writing one image in one pass is what
// `hidden` exists to prevent — see src/monitor.h.
//
// WHAT `app` DOES NOT DO IS THE POINT OF IT. There is no loop, no callback and
// no function pointer in that folder: the `while` below is this file's, and so
// is the order the systems run in, what is submitted in the gap between two of
// them, which key means what, the present mode and the readout. What was
// factored out is the handful of lines every program would write the same way
// and one of them would write subtly wrong.
//
// THREE THINGS LIVE HERE THAT WILL NOT LIVE HERE LONG, and each of them is a
// call site's business only until the folder that owns it exists:
//
//   - THE CAMERA PATH. The orbit is a function of that clock and it submits a
//     camera placement every frame. It is a demonstration and not a feature:
//     what a camera does about being moved is `scene`'s, and where a camera
//     should be is whatever is driving it.
//   - THE SPIN. The turning cube is a transform intent submitted every frame.
//     Same reasoning: how a transform is written is `scene`'s, what turns and
//     how fast is a scene's own, and there is no scene file yet.
//   - THE SUN'S PATH. The light circles the scene on the same clock, as a light
//     intent every frame, for one reason: a still light is a light nobody can
//     tell from a wrong one. A scene will say where its sun is the day there is
//     a scene file.
//
// NOT ONE #ifdef. If this file ever needs to know which operating system it is
// on, the API in platform/window.h, platform/input.h, render/device.h or 3d's
// headers has a hole and that is the finding, not a reason to reach for the
// preprocessor.
//
// ---- WHAT IT SHOULD LOOK LIKE ----
//
// A flat blue-green background with three lit things in it, from left to right:
//
//   - A cube standing still at the origin, which is where the camera looks.
//   - A squashed cube turning on a tilted axis, just to its right.
//   - A figure standing on nothing: `dev/src/textured_primitives_human.glb`,
//     three primitives out of Blender sharing one material — a body, a bar of
//     arms and a spherical head, about two metres tall, standing with its feet
//     at y = 0 rather than centred like the cubes.
//
// And two see-through squares, one warm and one cool, standing a metre and a
// half either side of the cubes.
//
// Then three smaller squares — one solid purple, one see-through green, one
// see-through orange — standing inside the turning cube and never hidden by it,
// however far round the orbit goes. Those are the overlay.
//
// Above all of it, three lines of writing on nothing — a sign in the world,
// lettered on both faces — and across the bottom of the view, one line of it
// that stays where it is however the camera moves and that nothing gets in front
// of.
//
// AND OFF TO THE LEFT, A SCREEN SHOWING THIS SAME WORLD FROM SOMEWHERE ELSE. A
// square standing on nothing, turned towards the middle of the scene, holding a
// second camera's picture: the cubes and the figure from high up and in front,
// lit by the same sun at the same instant, with the turning cube turning in it
// and the sun crossing it. It is a target drawn into once a frame and worn as an
// ordinary texture by an ordinary quad — see src/monitor.h. The world's own
// camera orbits and the second one does not, so what is on the screen holds
// still while everything around it swings.
//
// IT IS ONE-SIDED, SO HALF OF EVERY LAP IT IS NOT THERE. A screen has a back,
// and the back of this one is culled; the alternative would show the picture
// mirrored, which is the one thing this program's whole cast of lettered objects
// exists to make a person suspicious of.
//
// AND IT IS MISSING FROM ITS OWN PICTURE, WHICH IS THE POINT. The pass that
// fills the target leaves the screen out (ADR-0158) — so there is no screen
// inside the screen, and no hall of mirrors, and no image being read while it is
// written.
//
// And in the top-left of the view, lines of numbers that change as you watch:
// the frame rate, the four timings — the same ones the console prints — the
// pointer, and what the last frame cost in draw commands and element records.
// Laid out again every frame.
//
// THE DRAWS LINE IS A PERIOD METRIC LIKE THE FOUR ABOVE IT, average and worst
// over the same window, and the element records beside it are the last
// completed frame's. It is built before anything is drawn, so the one frame
// after each console block has no sample yet and shows the previous period's;
// a program that has completed no frame at all says "no frame yet" rather than
// showing a nought that reads as a claim.
//
// AND THE PAIR ON IT IS ADR-0092'S CLAIM RATHER THAN A NUMBER STANDING FOR IT.
// Ninety-odd element records against thirty-odd draw commands is what "many
// small things in one draw" means; watch the records climb while the commands
// hold still. The count includes the draw that put this readout on screen,
// which is why counting the things you can see gives one fewer.
//
// AND IT COUNTS BOTH PASSES, WHICH IS WHY IT IS ABOUT TWICE WHAT IS ON SCREEN.
// The monitor walks the same world into its own target before the window's pass
// runs, so nearly everything visible is drawn twice a frame. The console line
// printed once at startup says how many of the frame's commands the window's own
// walk was, which is the half that matches what a person can count.
//
// THE WORST COLUMN IS THE POINT OF MAKING IT A METRIC. In this demo the scene
// is identical every frame, so the average and the worst are both the same
// number and the column looks like decoration. The day anything varies what is
// drawn — culling, streaming, an interface that grows — the worst frame in the
// period is the number that matters and the average is the one that hides it.
//
// And three surfaces of elements, which are two different kinds of thing:
//
//   - THE EXHIBIT, a panel standing in the world behind the cubes: forty
//     coloured rectangles and two lines of writing, all of it one draw command.
//     It is an object in metres, so the cubes and the figure pass in front of it
//     as the camera goes round, and it is seen from behind — writing and all —
//     for half of every lap.
//   - THE BADGE, a small violet panel in the overlay, sitting in the turning
//     cube. Nothing ever covers it, which is what the overlay layer means; it
//     still keeps a real position in metres and is still seen in perspective.
//   - THE PLATE AND THE ROW OF TICKS in the top-left corner, which is not a
//     panel at all: it is mapped straight onto the window and has no position.
//     Drag the window narrow and it is the only thing that changes — the same
//     rectangles at the same size, with the far ticks off the right edge. See
//     src/surface.h for why that is the decision working.
//
// ---- THE TWO SEE-THROUGH QUADS, AND WHAT THEY ARE FOR ----
//
// THE CUBES ARE VISIBLE THROUGH THEM AND TINTED BY THEM. That is the whole claim
// of the blended pass: half of what a quad covers is the quad's colour and half
// is whatever was behind it. A quad that hides what is behind it is a blend that
// is not happening; a quad that has gone dark is the opaque path forcing alpha
// to one where it should not, or a colour premultiplied twice.
//
// AND WHICH OF THE TWO IS ON TOP CHANGES AS THE CAMERA GOES ROUND, WHICH IS THE
// SORT. They stand at +z and −z either side of the cubes, so the camera is
// behind one of them for half its lap and behind the other for the other half,
// and the nearer one's colour has to be the one on top of the overlap every
// time. If it is right from one side and wrong from the other, the sort's sign
// is backwards — that is the failure 3d/tests/depth_sort.c exists to catch
// before it gets here, and this is what it looks like when it does.
//
// ---- THE TWO STRINGS, AND WHAT EACH OF THEM IS FOR ----
//
// THE SIGN STANDS IN THE WORLD AND THE LINE IS LOCKED TO THE CAMERA, which are
// the two placements text has. The sign is three lines of Oxanium above the
// cubes and it is part of the scene: it turns with the orbit, it is read at an
// angle for most of a lap, and something in front of it hides it. The line sits
// a metre in front of the eye and stays where it is on screen however the camera
// moves — and it is still an object in the world, placed by a transform intent
// every frame. There is no screen-space path in this engine and there is not
// going to be one, so a heads-up display is a quad in front of the camera.
//
// AND THE LINE IS IN THE OVERLAY, SO NOTHING COVERS IT. Fly into a cube and the
// writing stays readable on top of it, which it did not before card 024: at one
// metre in front of the eye it went inside anything you walked into. The sign
// stays in the world and is still hidden by whatever gets between it and the
// camera, and having both is the point — the layer is a property of a drawable
// and not a switch the program is in.
//
// THE TEXT DOES NOT CHANGE AS THE SUN GOES ROUND, AND THAT IS THE UNLIT FLAG.
// The cubes brighten and darken through the lap; the writing keeps exactly the
// colour its material asks for, because an unlit material skips the whole
// shading model. Writing that dims when the sun crosses it is the flag missing,
// and it is the failure this is here to make obvious.
//
// AND IT IS BLENDED, SO A GLYPH IS A SHAPE AND NOT A BOX. Each letter is a quad
// whose alpha the shader works out from the sheet's distance field; a square of
// background round every letter is the alpha mode wrong, and text noticeably
// paler than the tint it asks for is a colour multiplied by its coverage twice.
//
// AND IT IS SHARP AT EVERY SIZE, WHICH IS WHAT THE SIGN IS FOR. Fly up to the
// sign until one letter fills the screen: its edges stay clean and its corners
// stay square. Fly away and it fades rather than crawling. Soft edges close up
// mean the material forgot base_colour_distance_field or the sheet was uploaded
// smooth; rounded corners mean the three channels came out of the sheet the
// same, which is the colouring in text/src/raster.c having gone wrong.
//
// THE HEADS-UP LINE SITS ON A DARK PANEL, AND THAT IS ABOUT CONTRAST AND NOT
// ABOUT SHARPNESS. Pale blue over teal is a small enough difference in luminance
// that a one-pixel edge still reads as soft, which sends anyone looking at it
// hunting for a blur that is not there. The panel takes that question off the
// table: the edge is the same width over it, and what still looks soft on it is
// soft. See HUD_PANEL_R.
//
// AND THERE IS NO ANTIALIASING ANYWHERE IN THE PICTURE, WHICH IS THE ENGINE'S
// RULE AND NOT A GAP. Letters have hard edges, textures show their texels close
// up and shimmer at a distance, and polygon silhouettes are stair-stepped. Every
// one of those is intended. What to look for instead is that the edges are in
// the RIGHT PLACE: a letter walked up to has straight sides and square corners
// rather than blocks, which is the distance field doing its job under a hard
// cut. Blocks would mean the sheet had become a picture of coverage again.
//
// TEXT A LONG WAY OFF BREAKS INTO SPECKS AND THEY MOVE. Expected, and the direct
// cost of the rule above; the sign at the top of the scene is where to see it.
//
// THE ACCENTED CHARACTERS ARE THE READER'S TEST. `Å`, `Ö`, `é`, `ü`, `å` and `Ç`
// are composite glyphs — references to other glyphs with an offset — and a
// reader that handles only simple outlines draws them as blanks while an English
// string looks perfect. If they are missing, that is the bug.
//
// THE COUNTERS ARE HOLES. The middles of `O`, `D`, `e`, `a`, `o`, `ö` and `å`
// are the background and not the letter. Filled in solid is the fill rule: see
// text/tests/raster.c, which is where that should have been caught.
//
// THEY ARE THE ONLY THINGS IN THE SCENE DRAWN IN THE SECOND PASS ALONGSIDE THE
// QUADS. Everything else is opaque and goes through the ordinary draw in table
// order; see 3d/draw_system.h.
//
// The two placeholder cubes wear WRITING — the still one the app icon, the
// turning one the wordmark — so every face says which way up and which way
// round it is, and a mirrored or upside-down face is unreadable rather than
// merely wrong. The figure wears its own albedo map, which is the thing to look
// at for whether a real exporter's texture coordinates arrive intact.
//
// IT IS LIT BY ONE SUN AND THE SUN GOES ROUND. One directional light, circling
// the scene once every SUN_SECONDS, so the bright side of everything moves and
// the far side of everything is black — there is no ambient light and no bounce,
// so an unlit face really is nothing. The figure's ORM map is read now:
// occlusion, roughness and metalness out of one picture, which is why it does
// not look like plastic in the way the cubes do.
//
// THE TURNING CUBE IS SQUASHED, AND THAT IS THE NORMAL MATRIX ON SCREEN. Its
// scale is not the same on all three axes (CUBE_SCALE_*), which is the one case
// where transforming a normal by the world matrix is visibly wrong: the shading
// would slide across the faces as it turned instead of staying stuck to them.
// 3d/tests/normal_matrix.c is the automated half; this is the half a person can
// see.
//
// NOTHING IS TONE MAPPED, SO THE BRIGHT SIDE CAN CLIP. A highlight that goes
// flat white is expected and is on the later list, not a mistake in the shading.
//
// It prints a line whenever something changes — the window's size, who is
// drawing its frame, whether the camera is being flown, whether the pointer is
// locked — one line per model at startup for what it cost, a block of timings
// every couple of seconds, and exits zero when the window is closed.
//
// TAB FLIES IT AND TAB HANDS IT BACK, AND BOTH STATES ARE WORTH LOOKING AT.
// There are two things to check here and they need different cameras: whether
// the rendering is right, which wants a camera nobody is touching, and whether
// the input is right, which wants a hand on it. Tab switches, Escape hands the
// camera back and closes the window when the camera is already back, and the
// orbit is what the program starts in.
//
// ---- ORBITING: THREE MOTIONS, AND ALL THREE HAVE TO BE THERE ----
//
// This is the thing to look at for the rendering, and the reason there are
// several objects rather than one:
//
//   - The camera orbits, once every twelve seconds or so. What says so is
//     parallax: the objects pass in front of and behind one another, which is
//     the only thing a moving camera can do and a rotating object cannot.
//   - One cube stands still, in the middle of the frame, and stays there. Its
//     faces turn because the camera goes round it. It never drifts, never
//     changes size and never leaves the centre — the camera looks straight at
//     it, from wherever it is.
//   - The other cube spins, three times as fast as the camera orbits, about a
//     tilted axis so that it cannot be mistaken for a second orbit.
//   - The model does neither: it sits where its file and one transform intent
//     put it.
//
// WHAT IS WRONG IF IT LOOKS WRONG. Each failure has its own shape:
//
//   - Nothing on screen, or a cube inside out — the depth test or the winding.
//     render/tests/offscreen.c is the automated form of that one.
//   - A quad hiding what is behind it rather than tinting it — the blend state,
//     or the material's mode arriving as opaque.
//   - The overlap of the two quads showing the far one's colour on top, from
//     some camera angles and not others — the sort's sign.
//   - An opaque surface gone dark — the alpha mode, and it looks like a lighting
//     regression rather than an alpha one. See render/shaders/draw.slang.
//   - Writing that brightens and dims as the sun goes round — the material's
//     `unlit` flag never reached the shading record.
//   - THE READOUT'S NUMBERS FROZEN WHILE THE CONSOLE BLOCKS KEEP COMING — the
//     transient path. Either the block is not being rebuilt each frame, or a
//     stale id is being drawn and refused on stderr every frame. There is no
//     cache anywhere that could make them merely slow to update, so frozen is
//     always a bug and never a saving.
//   - The readout's left edge jumping sideways as a number gains a digit — it
//     is being centred on its width. It is meant to be left-aligned, which is
//     why its placement asks nothing about its size.
//   - THE HEADS-UP LINE DISAPPEARING WHEN YOU FLY INTO SOMETHING — the layer.
//     Either the line is not in the overlay or the depth clear between the two
//     is not happening, and both look identical from here.
//   - Everything on top of everything, or the sign no longer hidden by what is
//     in front of it — the opposite failure, and the worse one: the layer has
//     become a switch the whole frame is in rather than a property of one
//     drawable. The sign and the cubes are what to check, not the line.
//   - THE PANEL OF RECTANGLES IN THE BOTTOM-RIGHT (card 030). Forty of them, in
//     one draw command, and the console says so once. Three things in it are
//     worth a look: the orange bar at its top is forty millimetres of rectangle
//     clipped to twenty, so half of it is missing on purpose; the row of six
//     bars below it is one colour at six alphas and has to read as a smooth
//     ramp, because a shader that forgot to premultiply leaves the faintest one
//     still obvious and one that did it twice makes the row vanish too early;
//     and the thirty-two squares below that are thirty-two different colours,
//     which is the thing one draw of one shading record could not be.
//   - The exhibit upside down, or the badge's orange corner mark at the bottom
//     — the element surface's Y. It runs down from the surface's top-left
//     corner, and the negation that makes that true is in
//     voe_render_element_surface_matrix. Nothing here negates anything.
//   - THE EXHIBIT VISIBLE THROUGH A CUBE THAT IS IN FRONT OF IT — the panel has
//     stopped being sorted with the see-through meshes and is being drawn after
//     everything, which is the one thing card 032 exists to prevent. The badge
//     is the opposite case and is meant to be visible through everything: it is
//     in the overlay, on the far side of the depth clear.
//   - The exhibit or the badge stretched, or changing size, when the window is
//     dragged narrow — a bug. Both are objects in metres and the window only
//     changes the camera's aspect. The surface that does answer to the window is
//     the plate and ticks in the top-left corner, and what it does is hold fewer
//     millimetres rather than narrower ones: the ticks keep their size and
//     spacing and the far ones fall off the right edge. See src/surface.h.
//   - The world's picture gone and only the overlay left — the depth clear
//     cleared colour as well. Only the depth aspect may be named; see
//     render/src/frame.c.
//   - THE PROGRAM STOPPING AT AN ASSERT NAMING A DRAW WHOSE SHADING RECORD
//     NAMES THE OPEN PASS'S TARGET TEXTURE — the monitor's `hidden` is not
//     reaching the walk, so the pass filling the target is drawing the screen
//     that shows it. It is a debug check (render/device.h): a release build
//     draws it, and what a person sees is whatever the driver felt like doing
//     with an image being read and written at once. See src/monitor.h.
//   - The screen darkening and going black as the sun crosses it — its material
//     is not unlit, so what is on it is the picture times a lambert term rather
//     than the picture.
//   - The screen holding one still picture, or noise, or the background colour —
//     nothing drew into its target this frame. A target nobody draws into keeps
//     whatever its frame slot last held, which is a picture several frames old
//     and then never changes; the monitor's pass is not being opened, or it was
//     opened onto the wrong target.
//   - The screen showing a screen showing a screen — `hidden` is naming the
//     wrong entity, and in a release build that is what the undefined read
//     happens to look like on a card that keeps the previous contents.
//   - The screen's picture stretched or squashed — it is square, drawn square
//     and shown on a square quad, so every one of those three has to agree; the
//     aspect ratio in src/monitor.c is the target's own and never the window's.
//   - The three quads inside the turning cube showing the far one's colour on
//     top of the near one's — the sort, inside the overlay, which is the same
//     sort and the same sign as the world's pair. It swaps twice a lap, so a
//     backwards sign is right for half of it.
//   - The solid overlay quad not hiding the see-through one behind it — the
//     overlay's solid group is not writing depth, or the depth clear is
//     happening after it rather than before.
//   - The two lit overlay quads not changing as the sun goes round, or the
//     unlit one changing — the layer has picked up a meaning about lighting
//     that it must not have. It decides order and nothing else.
//   - A square of background round every letter — the text material's alpha mode
//     arriving as opaque, which is the same failure as the quad above wearing a
//     different shape.
//   - Writing noticeably paler than the tint it asks for — a colour multiplied
//     by its coverage twice, which is the atlas having been premultiplied when
//     the shader is what does that. See text/src/font.c.
//   - Letters soft or blurry as the camera closes on the sign — the material's
//     base_colour_distance_field not set, or the sheet uploaded as colour or
//     sampled smooth. All three look the same and text/src/font.c is where the
//     three are decided together.
//   - Corners on `V`, `A` and the flat terminals coming out rounded — the sheet
//     is a distance field but its three channels agree, so the median has
//     nothing to reconstruct. That is the edge colouring, and
//     text/tests/raster.c is the automated form of it.
//   - Accented characters missing while an English string is perfect — composite
//     glyphs, and text/tests/truetype.c is the automated form.
//   - The middles of `O`, `e` and `a` filled in solid — the fill rule.
//     text/tests/raster.c is the automated form of that one.
//   - The sign missing for half the lap — one of its two faces did not get
//     built. A text block is one face, and the pair of entities is what makes it
//     a sign rather than a decal.
//   - A box where a character should be — the character is outside the range the
//     atlas covers, and that box is the font's own missing-glyph glyph. Correct,
//     and a reason to change the string rather than the folder.
//   - Everything drifting or growing — the projection or the aspect ratio.
//   - The picture upside down — the one Y flip went the wrong way or happened
//     twice. Every word on every cube is upright when it is right, and the app
//     icon's yellow "3D" badge is in its BOTTOM-RIGHT corner.
//   - WRITING THAT READS BACKWARDS — a mirror, and this is the failure worth
//     staring at. A model can come out mirrored from a transposed rotation or a
//     coordinate conversion nobody should have added, and a mirrored cube looks
//     completely normal until you try to read it. 3d/tests/import.c is the
//     automated form.
//   - The still cube not still, or not centred — the model matrix or the
//     look-at. scene/tests/transform.c and scene/tests/camera.c check both on
//     the CPU, so this should have failed before it got here.
//   - A model missing while the cubes are there — that import failed and said so
//     on stderr, or the world ran out of room for it. Each model is tried on its
//     own, so one of them can be missing without the other.
//   - The figure's texture smeared or in the wrong place while the cubes read
//     properly — a real exporter's texture coordinates, which nothing in this
//     repository generated. That is what having a file nobody here wrote is for.
//   - THE WORDMARK SQUEEZED ON THE TURNING CUBE'S NARROW FACES IS CORRECT, and
//     so is it reading almost undistorted on the two wide ones: the picture is
//     4800 by 2000 and those faces are CUBE_SCALE_X by CUBE_SCALE_Y, which is
//     nearly the same shape. A wordmark that looked the same on all six faces of
//     a cube that is not a cube would be the bug.
//   - EVERYTHING BLACK — the sun is pointing away from everything, its intensity
//     is nought, or the light never reached the shader. The background is
//     cleared and not lit, so a black scene on a coloured background is a
//     lighting failure and a black window is not.
//   - Everything pale and washed out, or muddy and too dark — a colour space.
//     One of the two sRGB halves (the texture format and the target format) is
//     doing its job without the other; see render/src/texture.c.
//   - The shading sliding across the squashed cube as it turns rather than
//     staying on its faces — the normal matrix, and the one thing that cube is
//     there to show.
//   - The bright side of the still cube not moving as the sun goes round — the
//     light intent is not landing, or the light system is not being run.
//
// ---- FLYING: W A S D, Q E, SPACE, CTRL, SHIFT, AND THE MOUSE ----
//
// Tab, then: W and S forwards and back along where the camera is looking, A and
// D left and right, E or Space up and Q or Ctrl down — straight up and down
// whatever the camera is looking at — Shift to go four times as fast, and the
// mouse to look around. Tab again or Escape to give it back, and Escape once
// more to close the window. E and Q are there so that the whole of flying is
// reachable from the left hand alone.
//
// P is the one key that is not about the camera: it switches the present mode,
// in either camera state, and is nowhere near the movement keys for that reason.
//
// WHAT IS WRONG IF IT FEELS WRONG, AND EACH OF THESE IS A DIFFERENT MISTAKE:
//
//   - The view jumps the moment Tab is pressed — the handover. The orbit stops
//     submitting placements and the keyboard starts submitting motions, and
//     because a placement applies before a motion in the same run, the camera
//     simply continues from where the orbit left it. A jump means one of those
//     two is submitting when it should not be.
//   - The mouse turns the wrong way, or up looks down — a sign in
//     scene/src/camera_system.c.
//   - Looking straight up or straight down and everything vanishes — the pitch
//     clamp. Try to look further up than you can; it should simply stop.
//   - Strafing while looking at the floor sinks into it — right is being taken
//     from the camera's own frame rather than kept horizontal.
//   - A diagonal is faster than a straight line — the movement direction is not
//     being normalized. Hold W, then hold W and D, and the speed should not
//     change.
//   - The camera keeps flying with nobody touching anything — a held key that
//     was never released. This is the one to look for after alt-tabbing away
//     with W down.
//
// WHAT THERE IS TO TRY:
//
//   - Alt-tab away while holding W, and come back. It must not still be flying
//     when focus is gone, and it must not need a fresh press of W to notice it
//     is still held on the way back.
//   - Look around, a lot, in one direction. It should not slow down, drift or
//     stick — mouse look with no pointer lock walks the cursor out of the window
//     and stops, which is what the `locked` line is for.
//   - Watch the `locked` line. Asking to fly asks for the pointer; a compositor
//     may say no, and then mouse look works only while the cursor happens to be
//     over the window. That is not a failure and nothing here treats it as one.
//   - Resize it. A `size` line should follow, the background should still reach
//     every corner, and the cubes should stay cubes rather than stretching — a
//     wider window shows more of the scene, it does not squash it.
//   - Toggle the frame off and on. On KWin: right-click the titlebar ->
//     More Actions -> No Borders, or Alt+F3. A `size` line follows and a
//     `decorated` line does not, which is the measured answer and not a gap.
//   - Minimise it. Nothing should happen and nothing should crash: a window with
//     no area has no frame to draw and the frame is skipped. The scene does not
//     advance while it is away, because the step below is inside that same test
//     — and the timing blocks keep coming, at thousands of frames a second,
//     which is the loop with nothing in it to wait for.
//   - WATCH THE TIMING BLOCKS, AND WATCH THEM MOVE. On fifo, `frame` should sit
//     within a few tenths of a millisecond of the display's refresh interval —
//     16.7 ms at sixty hertz — and the reciprocal printed beside it should be
//     the refresh rate. `draw` should be nearly all of it and `update` almost
//     none: the program is waiting for the display, which is what fifo means.
//     Then make something happen: drag the window bigger and `gpu` should go up
//     with the pixel count, minimise it and the rate should go through the roof.
//     A number that never moves is a number that is not being measured.
//   - READ THE SAME NUMBERS OFF THE SCREEN. The readout shows the period's
//     running averages, so it settles over the first few hundred milliseconds
//     after each console block and should then agree with the block that
//     follows, to within the averaging. Watch a few periods go by: it resets
//     with every block, and the frame rate moves the moment the window is
//     resized or minimised and restored. The `gpu` line must not flash `no
//     measurement` at the reset — it holds the last period's average for the
//     one frame the new period has no sample yet. If it does flash, the
//     three-case fallback in build_the_readout has been collapsed to two.
//
//   - WATCH THE `mouse` LINE OF THE READOUT (card 029). It is the pointer's
//     position in the window's own pixels and the three buttons, live. Move
//     the pointer to each corner: top-left should read close to 0 0 and
//     bottom-right one less than the `size` line in both numbers, with neither
//     overshooting nor inverting — a swapped or mirrored axis is a backend
//     mistake. Press the buttons: the dots become L, M and R. Hold one and drag
//     out of the window: the numbers go negative or past the size and keep
//     following, which is the drag both window systems promise. Move the
//     pointer off the window with nothing held and the line says `away`.
//     Press Tab to fly and it says `locked`: the camera has the pointer and
//     there is nothing at its position to point at, as include/platform/input.h
//     says. Tab back and move, and the numbers are live again.
//   - PRESS P AND COMPARE. It asks for fifo, and the `present` word on every
//     block says which is actually in force — a machine that has no mailbox was
//     already saying fifo and will go on saying it, and that is an answer.
//     What to look for is `frame` dropping onto the display's refresh interval
//     and the rate pinned to the refresh rate, in exchange for not drawing
//     frames nobody ever sees. Press it again to come back.
//   - Hold the window still and read the `worst` column. It should be close to
//     the average. A worst several times the average is stutter, and stutter is
//     the thing a person actually notices — which is why it is printed at all.
//   - Close it. It should print `closed` and exit zero.
//
// IT WILL SPIN A CORE WHILE IT IS OPEN, AND ON MAILBOX IT WILL SPIN THE GRAPHICS
// CARD TOO. Mailbox is what this program asks for as soon as its device is open —
// the engine itself opens on fifo — so it runs as fast as the card and the
// program between them allow, drawing many frames for every one anybody sees. P
// is how to stop it doing that: fifo waits
// for the display, so a visible window then costs one frame's worth of work per
// refresh. Either way a minimised one presents nothing and _poll returns
// immediately, because platform has no way to wait yet.
#include "cubes.h"
#include "elements.h"
#include "interface.h"
#include "monitor.h"
#include "surface.h"
#include "quad.h"
#include "shrink.h"
#include "sprites.h"

#include <3d/draw_system.h>
#include <3d/import.h>
#include <3d/material_component.h>
#include <3d/mesh_component.h>
#include <3d/panel_component.h>
#include <app/app.h>
#include <assets/image.h>
#include <base/arena.h>
#include <base/assert.h>
#include <base/error.h>
#include <base/report.h>
#include <base/samples.h>
#include <ecs/world.h>
#include <math/quat.h>
#include <platform/clock.h>
#include <platform/input.h>
#include <platform/window.h>
#include <render/device.h>
#include <scene/camera_system.h>
#include <scene/light_system.h>
#include <scene/camera_component.h>
#include <scene/transform_system.h>
#include <text/font.h>

#include <math.h>
#include <stdio.h>


// Scratch for the questions starting the GPU asks the driver — how many cards,
// which queue families, which surface formats. It is handed over, used and
// destroyed here, because nothing the device keeps comes out of it.
#define STARTUP_SCRATCH (64 * 1024)

// The arena the world, the decoded pictures and everything the model reader
// builds come out of. It is the arena's block size and not a limit: the arena
// chains blocks, so a push larger than this gets one of its own. A megabyte at a
// time is enough that the model below takes two or three blocks.
#define WORLD_ARENA (1024 * 1024)

// How much of everything the world may hold. Numbers rather than guesses, so
// that a model too big for them says so at the call that could not fit it.
#define MAX_ENTITIES 4096
#define MAX_COMPONENT_TYPES 8
#define MAX_INTENT_TYPES 8

// What the GPU makes room for. The cube is 24 vertices and the model is not
// much more; the rest is headroom for the next thing dropped in here — the
// monitor's screen is one geometry, one shading record and one entity out of it.
//
// MAX_DRAWN_OBJECTS IS PER FRAME AND THE FRAME NOW HAS TWO PASSES IN IT. Every
// drawable is recorded once per pass it is drawn in, and the monitor draws the
// whole world a second time, so the number this program actually spends is about
// twice what is on the screen. It is still a small fraction of this.
#define MAX_VERTICES (64 * 1024)
#define MAX_INDICES (128 * 1024)
#define MAX_MESHES 64
#define MAX_DRAWN_OBJECTS 256
#define MAX_SHADINGS 64

// How many passes one frame opens, and how many targets this program makes over
// its life. Two passes: the monitor's own target and then the window. One
// target, the monitor's — the window's is not one of these and costs nothing
// here. Each target also spends one of the device's texture slots, which is the
// one number in this list that is not asked for by name.
#define MAX_PASSES 2
#define MAX_TARGETS 1

// What the GPU makes room for per frame: geometry that lives one frame, which
// today is the readout and nothing else. Sized in glyphs because that is what
// fills it — four vertices and six indices each — and the readout is about
// seventy of them, so this is under double with nothing "to be safe" in it. The
// program prints what the readout actually took beside this number, once, so the
// next thing that needs transient room has a measurement to start from. Two
// ranges: the readout is one, and the other is for the next thing.
#define MAX_TRANSIENT_GLYPHS 128
#define MAX_TRANSIENT_GEOMETRIES 2

// The longest step the scene is ever advanced by, in seconds, however long the
// frame actually took.
//
// IT IS A CLAMP ON THE SCENE AND NOT ON THE MEASUREMENT. The numbers reported
// below are what the clock said, always; this is only what the orbit, the spin
// and the sun are stepped by. Without it, a frame that took two seconds — the
// window dragged to another monitor, the machine swapping, a debugger stopped at
// a breakpoint — teleports everything a sixth of the way round its lap in one
// step, and what a person sees is a scene that jumped rather than a frame that
// was slow. A quarter of a second is longer than any frame worth watching and
// shorter than any pause worth catching up on.
#define MAX_FRAME_SECONDS 0.25

// How often the timing block below is printed, in seconds. Each block is an
// average and a worst over exactly the period since the last one, so this is
// also the window every number in it is measured over. Two seconds is long
// enough to average a hundred frames and short enough that changing something
// and looking at the console is the same motion.
#define REPORT_SECONDS 2.0

// A full turn, radians.
#define TURN 6.2831853f

// The camera the program starts with. A sixty-degree vertical field of view, a
// near plane close enough to walk up to something and a far plane past anything
// in the scene.
#define FIELD_OF_VIEW 1.0471976f
#define NEAR_PLANE 0.1f
#define FAR_PLANE 100.0f

// The orbit: how far out, how high, and how long a lap takes. Three motions that
// can each be told apart — see the header.
//
// THE RADIUS IS WHATEVER FITS WHAT IS IN THE SCENE, and the scene is about six
// metres across. It has grown twice for that reason and shrank once, when the
// lettered test model came out and took the left-hand end of the scene with it;
// the radius was left where it was, so there is more empty space on the left
// than on the right now. It decides nothing and is a number to change when
// somebody minds.
#define ORBIT_RADIUS 7.0f
#define ORBIT_HEIGHT 1.8f
#define ORBIT_SECONDS 12.0f

// Where the two placeholder cubes stand, and how fast the second one turns about
// its tilted axis.
#define CUBES_APART 1.6f
#define SPIN_SECONDS 4.0f
#define SPIN_AXIS_X 1.0f
#define SPIN_AXIS_Y 1.0f
#define SPIN_AXIS_Z 0.0f

// The turning cube's scale, and the three numbers are different on purpose: a
// non-uniform scale is the only case where a normal matrix and a world matrix
// disagree, so this is what makes that difference something a person can look
// at. See the header.
#define CUBE_SCALE_X 1.4f
#define CUBE_SCALE_Y 0.6f
#define CUBE_SCALE_Z 1.0f

// The two see-through quads: how far either side of the cubes they stand, how
// big they are, how see-through, and what colour each one is.
//
// THEY ARE ON OPPOSITE SIDES OF THE SCENE BECAUSE THAT IS WHAT MAKES THE SORT
// SOMETHING A PERSON CAN SEE. The camera orbits, so which of the two is nearer
// swaps twice a lap; a sort with its sign the wrong way round is right for half
// the lap and wrong for the other half, and two quads at one depth would not
// show it. The offset in x and y is so that they overlap partly rather than
// exactly — the overlap is where the near one's colour has to be the one on top.
#define QUAD_Z 1.6f
#define QUAD_X 0.8f
#define QUAD_Y 0.4f
#define QUAD_SIZE 2.2f

// Half see-through, which is where a mistake in the blend is most visible: fully
// transparent hides a wrong colour and nearly opaque hides a wrong order.
#define QUAD_ALPHA 0.5f

// The three quads in the overlay: how big they are and how far either side of
// the middle one the other two stand. They are put at the turning cube, so these
// are the numbers that decide whether they are inside it.
//
// SMALL ENOUGH TO FIT INSIDE THE TURNING CUBE ACROSS. That cube is CUBE_SCALE_X
// by CUBE_SCALE_Y by CUBE_SCALE_Z and it spins, so a quad noticeably narrower
// than the smallest of those is enclosed by it from every angle — which is what
// makes "nothing in the world covers it" something a person can watch rather
// than take on trust. They stick out above and below, and that is fine: the
// claim is about the part that is inside.
//
// AND FAR ENOUGH APART TO OVERLAP RATHER THAN COINCIDE. Two quads at one depth
// would show nothing about the order they were drawn in. This is the same reason
// QUAD_Z is not nought, at a smaller scale.
#define OVERLAY_QUAD_SIZE 0.5f
#define OVERLAY_QUAD_Z 0.22f

// The two element panels: how big each one is in the world, and where it stands.
//
// A MILLIMETRE IS A MILLIMETRE AND THE SCALE IS WHAT MAKES THEM BIG ENOUGH TO
// LOOK AT. The exhibit is authored 240 by 135 mm, which is 0.24 by 0.135 metres
// — a postcard, and unreadable from seven metres out. The scale below is the
// entity's own transform doing what a transform does; it is not a second
// millimetre convention, and dividing the authored numbers by it would give the
// same picture with the layout's units made meaningless. See
// 3d/panel_component.h.
//
// THE EXHIBIT STANDS BEHIND THE CUBES SO THAT THEY PASS IN FRONT OF IT. That is
// the picture the whole card is for: a panel in the world layer is occluded by
// what is between it and the camera, which nothing drawn after the world could
// ever be. Half a lap it is partly hidden and half a lap it is not.
#define EXHIBIT_SCALE 14.0f
#define EXHIBIT_X 0.0f
#define EXHIBIT_Y 1.2f
#define EXHIBIT_Z (-2.2f)

// The badge sits in the turning cube, exactly where the three overlay quads do
// and for the same reason: something is in front of it for most of the lap and
// it is never hidden, which is what the overlay layer means. It is small
// because its job is to be a second range of the one element buffer rather than
// a second exhibit.
#define BADGE_SCALE 16.0f
#define BADGE_X 0.8f
#define BADGE_Y 0.35f
#define BADGE_Z 0.0f

// The two strings, and the two placements the card asks to see: one standing in
// the world and one locked to the camera.
//
// THE SIGN IS TWO ENTITIES SHARING ONE MESH, BACK TO BACK. A text block is four
// vertices and six indices per glyph — one face — and the engine culls back
// faces, so a single sign vanishes for half of the camera's lap. Two entities
// with the same geometry, one of them turned half a turn about Y, is a sign
// lettered on both sides; it costs one more transform and one more draw and
// nothing else. That is a double-sided *arrangement* and not a double-sided
// material, the same distinction dev/src/quad.h already makes about the quads.
//
// IT HAS AN ACCENTED CHARACTER IN IT ON PURPOSE. Most accented characters are
// composite glyphs — references to other glyphs with an offset — and a reader
// that handles only simple outlines draws them as blanks while looking perfectly
// correct on an English string. If the `å` is missing, that is the bug and
// text/tests/truetype.c is where it should have been caught.
#define SIGN_TEXT "VOE3D\nunlit · blended · Oxanium\nÅNGSTRÖMÄ · éüåäöÇ"
#define SIGN_EM 0.30f
#define SIGN_HEIGHT 3.4f

// The heads-up line: how far in front of the eye it sits, how big it is, and how
// far below the middle of the view. It is placed by a transform intent every
// frame, from where the camera actually is, which is the whole of what "locked
// to the camera" means here — there is no screen-space path and there is not
// going to be one.
//
// NEITHER STRING HOLDS A CHARACTER OUTSIDE LATIN-1, AND THAT IS DELIBERATE. The
// atlas covers the space to U+00FF; anything else is drawn as the font's
// missing-glyph box, which is right and looks exactly like a bug. An em dash
// stood here until it did.
#define HUD_TEXT "locked to the camera · Tab to fly"
#define HUD_EM 0.055f
#define HUD_DISTANCE 1.0f
#define HUD_DROP 0.36f

// What the two are tinted. The base colour factor of an unlit material is
// exactly what comes out, because nothing multiplies it by a light — so these
// are the colours on screen and not a starting point for one.
#define SIGN_TINT_R 1.0f
#define SIGN_TINT_G 0.86f
#define SIGN_TINT_B 0.45f
// The dark panel behind the heads-up line, and what it is for.
//
// IT IS A READING AID AND IT IS NOT PART OF THE ENGINE'S ANSWER TO ANYTHING. The
// line is pale blue over whatever the scene happens to put behind it, and over
// the teal background the two are close enough in luminance that a perfectly
// sharp edge still reads as soft — which is a question about contrast and not
// about the glyphs. A dark, opaque panel behind it settles that by making the
// contrast the largest it can be: what still looks soft on this is soft, and
// what does not was never soft.
//
// OPAQUE AND IN THE OVERLAY, WHICH IS WHY IT DOES NOT NEED SORTING. The overlay
// draws its solid group first and writes depth, then its blended group tests
// against it — so a panel pushed a little further from the eye than the line is
// behind it by depth and not by luck. It is the same arrangement the solid
// overlay quad already proves; see add_the_quads.
//
// THERE IS NO KEY TO TURN IT OFF, AND THAT IS `platform`'s DOING RATHER THAN A
// CHOICE. Every key voe_platform_key names is already bound — P is the present
// mode and Tab is the camera — so a toggle would mean adding one, which is
// another folder. It costs the strip of scene directly behind the line, which is
// background in every frame this program draws.
#define HUD_PANEL_R 0.04f
#define HUD_PANEL_G 0.05f
#define HUD_PANEL_B 0.06f

// How far past the line's own box the panel reaches, in ems of the line. Enough
// to read as a plate the writing sits on rather than as a box cropping it.
#define HUD_PANEL_MARGIN 0.5f

// How much further from the eye the panel is than the line. Small, because the
// two have to stay square to each other, and any positive number at all is
// enough for the depth test — this is not a sorting nudge, it is the whole
// distance between two planes that are parallel.
#define HUD_PANEL_BEHIND 0.01f

#define HUD_TINT_R 0.75f
#define HUD_TINT_G 0.92f
#define HUD_TINT_B 1.0f

// The readout: the same distance in front of the eye as the heads-up line,
// smaller than it, and snapped into the top-left corner of the view. Where the
// corner is in metres at that distance follows from the camera's field of view
// and the window's aspect ratio — see top_left_of_the_view — so it stays in the
// corner when the window is resized. The margin keeps it off the edge, in ems of
// its own size, and the first baseline sits one em below the top so the tallest
// glyph clears it. The string it holds is formatted into a buffer this long,
// which is well over the seven short lines it now holds.
#define READOUT_EM 0.040f
#define READOUT_MARGIN_EMS 0.5f
#define READOUT_CHARS 192

// The sun: how long a lap takes, how high it sits, and how strong it is.
//
// IT MOVES BECAUSE A STILL LIGHT PROVES NOTHING. A light that never moves is
// indistinguishable from a light pointing the wrong way, from a normal matrix
// that is the world matrix, and from shading that is stuck to the screen rather
// than to the surface. One lap every twenty seconds is slow enough to watch a
// face brighten and fast enough not to have to wait.
//
// THE HEIGHT IS THE VERTICAL PART OF THE DIRECTION IT TRAVELS, so a negative
// number is a sun above the scene shining downwards — see
// scene/light_component.h on which way a direction points. Not so steep that
// the sides of things go dark and not so shallow that the tops do.
//
// THE INTENSITY IS ABOVE ONE BECAUSE THE DIFFUSE TERM DIVIDES BY PI. A surface
// facing a white light of one comes back at about a third of its albedo, which
// is a scene that looks underexposed; π is what makes "one" mean "as bright as
// the texture". There is no exposure control and no tone mapping yet, so this is
// a number that looks right rather than a number that means something.
#define SUN_SECONDS 20.0f
#define SUN_HEIGHT (-0.8f)
#define SUN_INTENSITY 3.14159265f

// Where the model is put, once, after it is imported. A file places its
// contents wherever its author left them — a model should not have an opinion
// about what else is in the scene — so this is the call site moving it out of
// the cubes' way, and it moves it the way anything moves anything: by
// submitting a transform intent.
#define HUMAN_X 3.5f

// The two pictures on the placeholder cubes, embedded at build time. One each,
// which is a change from the "F" both of them used to share.
//
// THEY ARE WRITING, WHICH IS WHAT THE "F" WAS FOR AND IS WHY SWAPPING THEM COSTS
// NOTHING. A checker board looks right upside down and looks right mirrored, and
// both are mistakes this engine can make; a word does not. VOE3D read backwards
// is obvious across the room, and the icon's yellow "3D" badge sits in ONE
// corner, so which corner is which is still a thing the picture says rather than
// a thing to take on trust. What went with the old texture is only the four
// coloured corner blocks, and the writing says the same thing.
//
// THE WIDE ONE GOES ON THE WIDE CUBE AND THE SQUARE ONE ON THE CUBE THAT IS NOT
// SQUASHED, which is worth doing rather than pretty. The logo is 4800 by 2000
// and the turning cube's front face is CUBE_SCALE_X by CUBE_SCALE_Y, which is
// nearly the same ratio — so the wordmark reads almost undistorted on the two
// faces that matter, and is visibly squeezed on the four that are a different
// shape. That is the texture coordinates doing exactly what they should, and a
// picture that came out square on a face that is not would be the bug.
static const uint8_t LOGO_PNG[] = {
#embed "logo.png"
};

static const uint8_t APP_ICON_PNG[] = {
#embed "app icon light.png"
};

// The model, embedded the same way as the pictures and for the same reason:
// `platform` has no file API yet, so nothing here opens a file. The card that
// gives it one is the card that makes this path.
//
// IT CAME OUT OF BLENDER, WHICH IS THE POINT OF IT. Everything else
// here was built by this repository and agrees with this repository by
// construction; this is a file a real exporter wrote, with three primitives
// sharing one material, an albedo map and an ORM map, both a thousand pixels
// square. What it is really testing is that the reader survives a file nobody
// here wrote.
//
// BOTH ITS MAPS SHOW NOW. The albedo map is the texture coordinates' half and
// the ORM map — occlusion, roughness and metalness in the red, green and blue
// channels of one picture — is the shading's: card 019 lit the engine and reads
// all three of those channels. Its own export wires that picture into glTF's
// metallic-roughness slot and its normal slot rather than its occlusion slot, so
// the occlusion channel of it is not read as occlusion; that is the file's
// arrangement and assets/include/assets/model.h says why it is taken at its
// word.
static const uint8_t HUMAN_GLB[] = {
#embed "textured_primitives_human.glb"
};

// The bindings, and the only thing in this file that decides anything. Which key
// means forward is a call site's business — the engine's job is to know what
// forward does, not which finger asks for it.
//
// OPPOSITE KEYS HELD TOGETHER CANCEL, WHICH IS WHAT ADDING AND SUBTRACTING GIVES
// FOR FREE. W and S together is nought and not "whichever was pressed last",
// which needs an order this file does not keep.
static voe_scene_camera_motion camera_motion(voe_platform_window *window,
					     voe_ecs_entity eye, float seconds)
{
	voe_scene_camera_motion motion = {
		.entity = eye,
		.seconds = seconds,
	};
	voe_platform_motion mouse;

	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_W))
		motion.forward += 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_S))
		motion.forward -= 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_D))
		motion.right += 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_A))
		motion.right -= 1.0f;
	// Space and E are the same instruction, and so are Ctrl and Q, which is
	// why each pair is one test and not two. Two tests adding a step each
	// would make Space and E held together a rise of two: normalizing later
	// fixes the speed but not the direction.
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_SPACE) ||
	    voe_platform_input_key_down(window, VOE_PLATFORM_KEY_E))
		motion.up += 1.0f;
	if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_CONTROL) ||
	    voe_platform_input_key_down(window, VOE_PLATFORM_KEY_Q))
		motion.up -= 1.0f;

	motion.fast = voe_platform_input_key_down(window,
						  VOE_PLATFORM_KEY_SHIFT);

	mouse = voe_platform_input_motion(window);
	motion.look_x = mouse.x;
	motion.look_y = mouse.y;

	return motion;
}

// Where the orbit is at this many seconds in, as a placement: an eye and the two
// angles that look at the origin from it.
//
// THE ANGLES ARE WORKED OUT HERE AND NOT LEFT TO THE CAMERA, because a placement
// is an absolute answer and the camera's job is to hold it, not to guess what it
// was aimed at. It is also what makes the handover to flying seamless: the last
// placement the orbit submitted is exactly where the hand takes over from.
static voe_scene_camera_placement orbit(voe_ecs_entity eye, float seconds)
{
	float angle = seconds * TURN / ORBIT_SECONDS;
	voe_math_float3 position = { sinf(angle) * ORBIT_RADIUS, ORBIT_HEIGHT,
				     cosf(angle) * ORBIT_RADIUS };
	voe_math_float3 towards = voe_math_float3_normalize(
		voe_math_float3_neg(position));
	voe_scene_camera_placement placement = {
		.entity = eye,
		.eye = position,
		.pitch = asinf(towards.y),
		// Zero yaw looks along -Z and a positive yaw turns towards -X,
		// which is what this pair of arguments says.
		.yaw = atan2f(-towards.x, -towards.z),
	};

	return placement;
}

// Where the sun is pointing at this many seconds in, as an intent.
//
// IT CIRCLES ON THE HORIZONTAL PLANE AND LEANS DOWNWARDS. The x and z components
// go round with the clock and the y component is fixed, so the light comes from
// a different side of the scene every few seconds and always from above. The
// direction is not normalized here: the light system does that, which is the
// point of it doing it there — see scene/light_system.h.
//
// THE LAP IS NOT THE CAMERA'S LAP. SUN_SECONDS and ORBIT_SECONDS are different
// numbers on purpose: if the sun went round with the camera, every surface would
// keep the same brightness and the whole thing would look like shading stuck to
// the screen — which is one of the failures this program exists to show.
static voe_scene_light_intent sunlight(voe_ecs_entity sun, float seconds)
{
	float angle = seconds * TURN / SUN_SECONDS;
	voe_scene_light_intent intent = {
		.entity = sun,
		.light = {
			.direction = { sinf(angle), SUN_HEIGHT, cosf(angle) },
			.colour = { 1.0f, 1.0f, 1.0f },
			.intensity = SUN_INTENSITY,
		},
	};

	return intent;
}

// One cube, one entity: geometry it shares with its neighbour, a material it
// shares with its neighbour, a transform of its own, and the world layer.
//
// IT NAMES ITS LAYER RATHER THAN LETTING A ZEROED STRUCT PICK ONE. World is
// nought, so this line changes nothing and is here because every drawable in
// this file says which layer it is in — see the header on the two placements and
// why neither is the normal case.
static bool add_cube(voe_ecs_world *world, voe_render_geometry geometry,
		     voe_3d_material material, voe_math_float3 position,
		     voe_math_float3 scale, voe_ecs_entity *out)
{
	voe_scene_transform transform = {
		.position = position,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = scale,
	};

	if (!voe_ecs_entity_create(world, out))
		return false;
	if (!voe_scene_transform_add(world, *out, transform))
		return false;
	if (!voe_3d_mesh_add(world, *out,
			     (voe_3d_mesh){ .geometry = geometry,
					    .layer = VOE_3D_LAYER_WORLD }))
		return false;
	return voe_3d_material_add(world, *out, material);
}

// One embedded picture into a texture slot, and the shading record that wears
// it. Both cubes want the same material but for the picture, so the material is
// built here once and the caller says which bytes.
//
// THE PICTURE ARRIVES THE WAY THE SHADERS DO: `#embed`ded at build time, which
// is what keeps "nothing is read from disk at run time" true. The decoded pixels
// are scratch — `render` has taken its own copy by the time the upload returns —
// so the arena goes back on every path out, the failing ones included.
static bool textured_material(voe_render_device *gpu, voe_base_arena *arena,
			      const uint8_t *png, size_t size,
			      voe_3d_material *out, voe_base_error *error)
{
	voe_render_texture texture = { 0 };
	voe_assets_image picture;
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);
	bool ok;

	*out = (voe_3d_material){
		.base_colour = { 1.0f, 1.0f, 1.0f, 1.0f },
		.metallic = 0.0f,
		.roughness = 0.8f,
	};

	ok = voe_assets_png_decode(png, size, arena, &picture, error);
	// AND THEN HALVED UNTIL IT IS A SENSIBLE SIZE, WHICH IS dev's OWN DOING
	// AND NOT THE ENGINE'S. The two pictures here are far bigger than any
	// face they land on, and this engine samples one level with no mip
	// chain, so the level the file happened to ship is the level that gets
	// sampled. src/shrink.h is the whole argument, including why this is
	// not mipmapping and what it does not fix.
	if (ok)
		voe_dev_image_shrink(&picture, VOE_DEV_SHRINK_LONG_SIDE);
	// A colour, so it goes up in the sRGB format and the hardware decodes it
	// before the shading multiplies by it. An ORM map would be the other kind
	// — see voe_render_texture_kind.
	if (ok)
		ok = voe_render_texture_create(gpu, VOE_RENDER_TEXTURE_COLOUR,
					       VOE_RENDER_SAMPLING_SMOOTH,
					       picture.width, picture.height,
					       picture.pixels, &texture, error);
	voe_base_arena_rewind(arena, mark);
	if (!ok)
		return false;

	out->base_colour_texture = texture;
	return voe_3d_material_upload(gpu, out, error);
}

// The two placeholder cubes: their geometry into the pools, a picture each into
// a texture slot, a shading record each, and two entities.
static bool add_the_cubes(voe_ecs_world *world, voe_render_device *gpu,
			  voe_base_arena *arena, voe_ecs_entity *turning,
			  voe_base_error *error)
{
	voe_render_geometry geometry = { 0 };
	voe_3d_material icon;
	voe_3d_material logo;
	voe_ecs_entity still = { 0 };

	if (!voe_render_geometry_create(gpu, voe_dev_cube_vertices,
					VOE_DEV_CUBE_VERTEX_COUNT,
					voe_dev_cube_indices,
					VOE_DEV_CUBE_INDEX_COUNT, &geometry,
					error))
		return false;

	if (!textured_material(gpu, arena, APP_ICON_PNG, sizeof APP_ICON_PNG,
			       &icon, error))
		return false;
	if (!textured_material(gpu, arena, LOGO_PNG, sizeof LOGO_PNG, &logo,
			       error))
		return false;

	// ONE GEOMETRY, TWO SHADING RECORDS, TWO ENTITIES, AND THE PAIR IS WORTH
	// MORE THAN THE ONE RECORD IT REPLACED. Both cubes are the same
	// twenty-four vertices; what differs is a texture id in a shading
	// record, so this is also the smallest demonstration in the program
	// that one mesh can be worn two ways. The second one is squashed, which
	// is what makes the normal matrix visible — see the header.
	return add_cube(world, geometry, icon,
			(voe_math_float3){ 0.0f, 0.0f, 0.0f },
			(voe_math_float3){ 1.0f, 1.0f, 1.0f }, &still) &&
	       add_cube(world, geometry, logo,
			(voe_math_float3){ CUBES_APART, 0.0f, 0.0f },
			(voe_math_float3){ CUBE_SCALE_X, CUBE_SCALE_Y,
					   CUBE_SCALE_Z },
			turning);
}

// The material a quad wears. The three things that differ between the five of
// them are all here; everything else is the same for all of them.
//
// THE COLOUR IS NOT PREMULTIPLIED HERE. A material's base colour is an ordinary
// colour with an alpha beside it; the shader multiplies at the very end, which
// is where the engine's premultiplied contract is applied. See
// render/shaders/draw.slang.
//
// FULLY ROUGH AND NOT METALLIC, so what is seen through a see-through one is the
// blend and not a highlight. A metal has no diffuse response, which would make
// most of the quad nearly black and the blend impossible to judge.
static voe_3d_material quad_material(voe_math_float4 colour,
				     voe_render_alpha_mode alpha_mode,
				     bool unlit)
{
	voe_3d_material material = {
		.base_colour = colour,
		.metallic = 0.0f,
		.roughness = 1.0f,
		.alpha_mode = alpha_mode,
		.alpha_cutoff = 0.5f,
		.unlit = unlit,
	};

	return material;
}

// Where one quad stands and how big it is. They are all square and all upright,
// so a position and one number is the whole of a quad's transform.
static voe_scene_transform quad_at(voe_math_float3 position, float size)
{
	voe_scene_transform transform = {
		.position = position,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { size, size, 1.0f },
	};

	return transform;
}

// One panel, one entity: a transform saying where the surface stands and how big
// it is, and a panel component saying how big the surface is in its own
// millimetres and which layer it is drawn in.
//
// NO MATERIAL AND NO MESH, WHICH IS THE SHAPE WORTH NOTICING. An element carries
// its own colour, so there is no shading record to point at and no geometry to
// name — the two rows here are the whole of a drawable surface. The range is
// left at nought and is written every frame by the loop; until the first frame
// writes one, a count of nought draws nothing and is not an error.
static bool add_panel(voe_ecs_world *world, voe_math_float3 position,
		      float scale, voe_math_float2 millimetres,
		      voe_3d_layer layer, voe_ecs_entity *out)
{
	voe_ecs_entity entity = { 0 };
	voe_scene_transform transform = {
		.position = position,
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { scale, scale, scale },
	};

	if (!voe_ecs_entity_create(world, &entity))
		return false;
	if (!voe_scene_transform_add(world, entity, transform))
		return false;
	if (!voe_3d_panel_add(world, entity,
			      (voe_3d_panel){ .size = millimetres,
					      .layer = layer }))
		return false;

	*out = entity;
	return true;
}

// One quad, one entity: geometry it shares with every other quad, a material of
// its own because the colour differs, a transform of its own, and the layer it
// is drawn in.
static bool add_quad(voe_ecs_world *world, voe_render_device *gpu,
		     voe_render_geometry geometry, voe_3d_material material,
		     voe_scene_transform transform, voe_3d_layer layer,
		     voe_ecs_entity *out, voe_base_error *error)
{
	voe_ecs_entity entity = { 0 };

	if (!voe_3d_material_upload(gpu, &material, error))
		return false;
	if (!voe_ecs_entity_create(world, &entity))
		return false;
	if (!voe_scene_transform_add(world, entity, transform))
		return false;
	if (!voe_3d_mesh_add(world, entity,
			     (voe_3d_mesh){ .geometry = geometry,
					    .layer = layer }))
		return false;
	if (!voe_3d_material_add(world, entity, material))
		return false;

	// Every quad but the panel is placed once and never moved again, so the
	// entity is of no use to them. `out` may be NULL for those.
	if (out != NULL)
		*out = entity;
	return true;
}

// The five quads: one square of geometry into the pools, two entities standing
// either side of the cubes in the world, and three more standing inside the
// turning cube in the overlay.
//
// A MATERIAL EACH AND NOT ONE BETWEEN THEM, unlike the cubes. They have to be
// different colours or there is no way to tell from the picture which one ended
// up on top, and a material is where a colour lives.
//
// ---- THE TWO IN THE WORLD ----
//
// The warm one nearer +Z and the cool one nearer −Z, so that whichever the
// camera is behind is the one whose colour is on top of the other. They are
// added in this order deliberately: the table order is warm then cool for the
// whole run, so a frame that looks right from one side and wrong from the other
// is the sort and nothing else.
//
// ---- THE THREE IN THE OVERLAY, AND WHAT EACH ONE IS FOR ----
//
// THEY STAND INSIDE THE TURNING CUBE, WHICH IS THE POINT. The squashed cube is
// solid and it is right there around them, so every one of these would be hidden
// for most of a lap if the layer were not working. They keep real positions in
// metres and are seen through the same camera as everything else — this is
// "always on top", not screen space, and there is no orthographic projection
// anywhere in this engine.
//
// AND THEY STILL OCCLUDE EACH OTHER, WHICH IS THE HALF THAT A LAYER MADE OF A
// DISABLED DEPTH TEST WOULD GET WRONG. The two see-through ones stand at +Z and
// −Z of the solid one exactly as the world's pair straddles the cubes, so which
// of them is nearer swaps twice a lap and the nearer one's colour has to be the
// one on top of the overlap — the same diagnostic, one layer up. The solid one
// between them writes depth like any other solid thing, so it hides whichever of
// the two is behind it and is tinted by whichever is in front.
//
// ONE OF THEM IS LIT AND ONE IS NOT, AND THAT IS A SEPARATE AXIS ON PURPOSE. The
// layer decides order and never lighting. The lit pair brighten and darken as
// the sun goes round; the unlit one keeps exactly the colour its material asks
// for. An overlay that quietly stopped lighting things would look like a
// reasonable convenience and it is the one this arrangement is here to catch.
static bool add_the_quads(voe_ecs_world *world, voe_render_device *gpu,
			  voe_render_geometry *quad, voe_base_error *error)
{
	voe_render_geometry geometry = { 0 };
	voe_math_float4 warm = { 0.9f, 0.25f, 0.15f, QUAD_ALPHA };
	voe_math_float4 cool = { 0.15f, 0.35f, 0.9f, QUAD_ALPHA };
	voe_math_float4 over_lit = { 0.25f, 0.9f, 0.4f, QUAD_ALPHA };
	voe_math_float4 over_unlit = { 0.95f, 0.55f, 0.1f, QUAD_ALPHA };
	voe_math_float4 over_solid = { 0.55f, 0.2f, 0.75f, 1.0f };
	voe_math_float3 middle = { CUBES_APART, 0.0f, 0.0f };

	if (!voe_render_geometry_create(gpu, voe_dev_quad_vertices,
					VOE_DEV_QUAD_VERTEX_COUNT,
					voe_dev_quad_indices,
					VOE_DEV_QUAD_INDEX_COUNT, &geometry,
					error))
		return false;

	// The one square every quad in this program wears, the heads-up panel
	// included. One upload and one id; what differs between them is a
	// material and a transform.
	*quad = geometry;

	if (!add_quad(world, gpu, geometry,
		      quad_material(warm, VOE_RENDER_ALPHA_BLENDED, false),
		      quad_at((voe_math_float3){ QUAD_X, QUAD_Y, QUAD_Z },
			      QUAD_SIZE),
		      VOE_3D_LAYER_WORLD, NULL, error))
		return false;
	if (!add_quad(world, gpu, geometry,
		      quad_material(cool, VOE_RENDER_ALPHA_BLENDED, false),
		      quad_at((voe_math_float3){ QUAD_X, QUAD_Y, -QUAD_Z },
			      QUAD_SIZE),
		      VOE_3D_LAYER_WORLD, NULL, error))
		return false;

	// The solid one first, so that the two see-through ones are not merely
	// being drawn in an order that happens to look right: it writes depth
	// before either of them is issued, and both of them test against it.
	if (!add_quad(world, gpu, geometry,
		      quad_material(over_solid, VOE_RENDER_ALPHA_OPAQUE, false),
		      quad_at(middle, OVERLAY_QUAD_SIZE), VOE_3D_LAYER_OVERLAY,
		      NULL, error))
		return false;
	if (!add_quad(world, gpu, geometry,
		      quad_material(over_lit, VOE_RENDER_ALPHA_BLENDED, false),
		      quad_at((voe_math_float3){ middle.x, middle.y,
						 OVERLAY_QUAD_Z },
			      OVERLAY_QUAD_SIZE),
		      VOE_3D_LAYER_OVERLAY, NULL, error))
		return false;
	return add_quad(world, gpu, geometry,
			quad_material(over_unlit, VOE_RENDER_ALPHA_BLENDED,
				      true),
			quad_at((voe_math_float3){ middle.x, middle.y,
						   -OVERLAY_QUAD_Z },
				OVERLAY_QUAD_SIZE),
			VOE_3D_LAYER_OVERLAY, NULL, error);
}

// One text block, one entity: the mesh the font built, an unlit blended material
// wearing the atlas, and the transform it is placed with.
//
// THE FIVE THINGS A TEXT MATERIAL HAS TO SAY, and each of them is visible if it
// is missing. The atlas as the base colour texture, or there is nothing to see.
// The tint as the base colour factor, which for an unlit material is exactly the
// colour on screen. `unlit`, or the letters darken and brighten as the sun goes
// round — text lit by a sun is the failure this flag exists to prevent. BLENDED,
// or every glyph arrives in a square of its own background. And
// `base_colour_distance_field`, or the sheet is read as a picture and every
// glyph is a solid rectangle.
//
// AND THE TINT IS NOT PREMULTIPLIED HERE. It is an ordinary colour with an alpha
// beside it; the shader multiplies at the very end. See dev's other blended
// thing, add_quad, which says the same in the same words.
//
// THE LAYER IS THE CALLER'S AND IT IS NOT A FIFTH THING THE MATERIAL SAYS. Both
// of this program's strings wear the same material and they are in different
// layers: the sign is part of the scene and the heads-up line is above it. Unlit
// and overlay travel together here by coincidence and not by rule — see the
// header.
static bool add_text(voe_ecs_world *world, voe_render_device *gpu,
		     const voe_text_font *font, voe_text_block block,
		     voe_math_float4 tint, voe_scene_transform transform,
		     voe_3d_layer layer, voe_ecs_entity *out,
		     voe_base_error *error)
{
	voe_ecs_entity entity = { 0 };
	voe_3d_material material = {
		.base_colour = tint,
		.metallic = 0.0f,
		.roughness = 1.0f,
		.alpha_mode = VOE_RENDER_ALPHA_BLENDED,
		.alpha_cutoff = 0.5f,
		.unlit = true,
		.base_colour_texture = voe_text_font_atlas(font),
		// The atlas is a distance field and not a picture. Without this
		// the shader multiplies the tint by three distances and reads
		// the sheet's alpha, which is opaque everywhere: solid coloured
		// rectangles where the writing should be.
		.base_colour_distance_field = true,
	};

	if (!voe_3d_material_upload(gpu, &material, error))
		return false;
	if (!voe_ecs_entity_create(world, &entity))
		return false;
	if (!voe_scene_transform_add(world, entity, transform))
		return false;
	if (!voe_3d_mesh_add(world, entity,
			     (voe_3d_mesh){ .geometry = block.geometry,
					    .layer = layer }))
		return false;
	if (!voe_3d_material_add(world, entity, material))
		return false;

	*out = entity;
	return true;
}

// The font, and the four text entities it is drawn into: the sign's two faces,
// the line locked to the camera, and the readout, which gets its geometry every
// frame rather than here.
//
// THE FONT IS MADE HERE AND NOT AT STARTUP, which is the difference between a
// program that draws text and one that does not. Nothing in `render` builds an
// atlas; this call is what builds it, once, and a program that never makes one
// never pays for it.
//
// THE SIGN IS CENTRED ON ITS OWN WIDTH. A block's origin is the left end of its
// first baseline, so a sign that was not shifted would hang off to one side of
// whatever it is standing on.
static bool add_the_text(voe_ecs_world *world, voe_render_device *gpu,
			 voe_base_arena *arena, voe_render_geometry quad,
			 voe_text_font **font, voe_ecs_entity *hud,
			 voe_ecs_entity *panel, voe_ecs_entity *readout,
			 voe_math_float2 *hud_size, voe_base_error *error)
{
	voe_text_block sign;
	voe_text_block line;
	voe_math_float4 sign_tint = { SIGN_TINT_R, SIGN_TINT_G, SIGN_TINT_B,
				      1.0f };
	voe_math_float4 hud_tint = { HUD_TINT_R, HUD_TINT_G, HUD_TINT_B, 1.0f };
	voe_scene_transform front;
	voe_scene_transform back;
	voe_ecs_entity unused = { 0 };

	// Oxanium, which is dev's face and stays so — 005's own criterion 7
	// keeps the dev program on it while the editor is free to name either.
	*font = voe_text_font_new(VOE_TEXT_TYPEFACE_OXANIUM, gpu, arena, error);
	if (*font == NULL)
		return false;

	if (!voe_text_block_create(*font, gpu, arena, SIGN_TEXT, SIGN_EM, &sign,
				   error))
		return false;
	if (!voe_text_block_create(*font, gpu, arena, HUD_TEXT, HUD_EM, &line,
				   error))
		return false;

	front = (voe_scene_transform){
		.position = { -sign.size.x * 0.5f, SIGN_HEIGHT, 0.0f },
		.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
		.scale = { 1.0f, 1.0f, 1.0f },
	};
	// The same mesh, turned half a turn about Y and shifted the other way,
	// so that its left end lands where the front face's right end is and the
	// two sit exactly back to back.
	back = (voe_scene_transform){
		.position = { sign.size.x * 0.5f, SIGN_HEIGHT, 0.0f },
		.rotation = voe_math_quat_from_axis_angle(
			(voe_math_float3){ 0.0f, 1.0f, 0.0f }, TURN * 0.5f),
		.scale = { 1.0f, 1.0f, 1.0f },
	};

	// The sign is in the world, so that walking something in front of it
	// still hides it. That is half of what the two placements are for.
	if (!add_text(world, gpu, *font, sign, sign_tint, front,
		      VOE_3D_LAYER_WORLD, &unused, error))
		return false;
	if (!add_text(world, gpu, *font, sign, sign_tint, back,
		      VOE_3D_LAYER_WORLD, &unused, error))
		return false;

	// The heads-up line starts wherever; the loop places it every frame from
	// where the camera actually is, and its width is what centres it there.
	//
	// AND IT IS IN THE OVERLAY, WHICH IS THE OTHER HALF. A line of writing
	// that tells you what the keys do is no use at the moment you fly into
	// something, and until card 024 that is exactly when it disappeared. It
	// is still an ordinary object in the world with a position in metres —
	// what changed is when it is drawn, not where it is.
	// The panel first, so that the solid thing exists before the blended
	// thing that sits on it. Order of creation decides nothing here — the
	// draw system sorts by layer and alpha mode — but reading it in this
	// order is how the picture is built.
	if (!add_quad(world, gpu, quad, quad_material((voe_math_float4){
					     HUD_PANEL_R, HUD_PANEL_G,
					     HUD_PANEL_B, 1.0f },
				     VOE_RENDER_ALPHA_OPAQUE, true),
		      quad_at((voe_math_float3){ 0.0f, 0.0f, 0.0f }, 1.0f),
		      VOE_3D_LAYER_OVERLAY, panel, error))
		return false;

	*hud_size = line.size;
	if (!add_text(world, gpu, *font, line, hud_tint, front,
		      VOE_3D_LAYER_OVERLAY, hud, error))
		return false;

	// The readout: an entity wearing the text material and, for now, no
	// geometry. The block it draws is built inside every frame and put on
	// it with voe_3d_mesh_set_geometry, so a zeroed id — which names
	// nothing — is exactly right until the first frame opens, and nothing
	// draws it before then. Same material, same layer and, until the loop
	// places it, the same transform as the line.
	return add_text(world, gpu, *font, (voe_text_block){ 0 }, hud_tint,
			front, VOE_3D_LAYER_OVERLAY, readout, error);
}

// The camera's frame, for placing things that travel with it: where the eye is,
// which way it looks, which way is right and up across the view, and the
// rotation that stands a quad square to it. The three things locked to the
// camera below are all placed from one of these.
//
// THE ROTATION IS THE CAMERA'S TWO ANGLES, COMPOSED IN THAT ORDER. Yaw about Y
// and then pitch about X, which is the same composition voe_scene_camera_view
// builds its basis from; _mul reads right to left, so the pitch is the one
// applied first. Swap them and the line rolls as the camera looks up.
struct view_basis {
	voe_math_float3 eye;
	voe_math_float3 forward;
	voe_math_float3 right;
	voe_math_float3 up;
	voe_math_quat rotation;
	// How far the view reaches above its centre, per metre of distance in
	// front of the eye: the tangent of half the vertical field of view.
	// Times the aspect ratio for how far it reaches to the side. It is what
	// lets something be put in a corner of the view rather than near one.
	float half_height;
};

static struct view_basis basis_of(const voe_ecs_world *world,
				  voe_ecs_entity eye)
{
	static const voe_math_float3 UP = { 0.0f, 1.0f, 0.0f };
	static const voe_math_float3 SIDE = { 1.0f, 0.0f, 0.0f };
	const voe_scene_camera *camera = voe_scene_camera_get(world, eye);
	struct view_basis basis;

	basis.eye = camera->eye;
	basis.forward = voe_scene_camera_forward(*camera);
	basis.right = voe_math_float3_normalize(
		voe_math_float3_cross(basis.forward, UP));
	basis.up = voe_math_float3_cross(basis.right, basis.forward);
	basis.rotation = voe_math_quat_mul(
		voe_math_quat_from_axis_angle(UP, camera->yaw),
		voe_math_quat_from_axis_angle(SIDE, camera->pitch));
	basis.half_height = tanf(camera->fov_y * 0.5f);
	return basis;
}

// A transform standing square to the camera at `at`, with the scale given.
static voe_scene_transform_intent square_to_the_camera(
	const struct view_basis *basis, voe_ecs_entity entity,
	voe_math_float3 at, voe_math_float3 scale)
{
	return (voe_scene_transform_intent){
		.entity = entity,
		.transform = {
			.position = at,
			.rotation = basis->rotation,
			.scale = scale,
		},
	};
}

// Where the heads-up line goes this frame: in front of the eye, square to it,
// centred across it and a little below the middle.
//
// IT IS AN ORDINARY TRANSFORM IN THE WORLD AND THAT IS THE POINT. There is no
// screen-space path in this engine and a "just for debug" one is exactly what
// that rule exists to prevent, so a heads-up display is a quad standing in front
// of the camera and moved with it.
//
// WHAT STOPS A CUBE GETTING IN FRONT OF IT IS THE LAYER AND NOT THIS FUNCTION.
// Standing a metre from the eye used to mean flying into anything put that thing
// in front of the writing; the line is in the overlay now, so it is drawn after
// the world's depth is thrown away. This function still only decides where the
// line is, and it would put it in exactly the same place if it were in the world
// — those are two separate answers to two separate questions and neither one
// implies the other.
static voe_scene_transform_intent facing_the_camera(const voe_ecs_world *world,
						    voe_ecs_entity eye,
						    voe_ecs_entity text,
						    voe_math_float2 size)
{
	struct view_basis basis = basis_of(world, eye);
	voe_math_float3 at = voe_math_float3_add(
		basis.eye, voe_math_float3_scale(basis.forward, HUD_DISTANCE));

	at = voe_math_float3_add(
		at, voe_math_float3_scale(basis.right, -size.x * 0.5f));
	at = voe_math_float3_add(at, voe_math_float3_scale(basis.up, -HUD_DROP));

	return square_to_the_camera(&basis, text, at,
				    (voe_math_float3){ 1.0f, 1.0f, 1.0f });
}

// Where the readout goes this frame: square to the camera like the line, in the
// top-left corner of the view, a margin in from both edges.
//
// THE CORNER IS WORKED OUT FROM THE FIELD OF VIEW AND THE ASPECT RATIO. At a
// metre in front of the eye the view reaches tan(fov/2) metres up and that times
// the aspect ratio to the side, so the corner is a point on the same plane the
// line stands on and the readout is pinned to the edge of the window, whatever
// size the window is. This is still an ordinary transform in the world: a
// resize moves the corner and the next frame's intent follows it.
//
// IT IS LEFT-ALIGNED AND NOT CENTRED, BECAUSE ITS WIDTH CHANGES. A block's
// origin is the left end of its first baseline, so holding that point still
// holds the left edge still while the digits change; centring on the width, as
// the line does, would shift the whole readout sideways every time a number
// gained a digit.
//
// AND THAT IS WHAT LETS IT BE PLACED HERE AT ALL. The block is built inside the
// frame, after the transform system has run, so a placement that wanted its size
// would be a frame late. This one asks only where the camera is and how big the
// window is.
static voe_scene_transform_intent top_left_of_the_view(
	const voe_ecs_world *world, voe_ecs_entity eye, voe_ecs_entity readout,
	voe_platform_size size)
{
	struct view_basis basis = basis_of(world, eye);
	// A window with no area has no corner; one is as good as any other
	// then, because nothing is about to be drawn.
	float aspect = size.width > 0 && size.height > 0 ?
			       (float)size.width / (float)size.height :
			       1.0f;
	float half_height = basis.half_height * HUD_DISTANCE;
	float half_width = half_height * aspect;
	float margin = READOUT_EM * READOUT_MARGIN_EMS;
	voe_math_float3 at = voe_math_float3_add(
		basis.eye, voe_math_float3_scale(basis.forward, HUD_DISTANCE));

	at = voe_math_float3_add(
		at, voe_math_float3_scale(basis.right, -(half_width - margin)));
	at = voe_math_float3_add(
		at, voe_math_float3_scale(basis.up, half_height - margin -
							    READOUT_EM));

	return square_to_the_camera(&basis, readout, at,
				    (voe_math_float3){ 1.0f, 1.0f, 1.0f });
}

// Where the panel behind the heads-up line goes this frame: the same plane as the
// line, a shade further from the eye, centred on the line's own box and a margin
// larger than it.
//
// IT SHARES THE LINE'S ROTATION AND NOT ITS POSITION. The line's origin is the
// left end of its first baseline, so a panel placed there would hang off to one
// side and sit too low; it is moved right by half the width and up by a quarter
// of the height, which puts it around the band the glyphs actually occupy rather
// than around the line box. A quarter and not a half because a line box is
// mostly above its baseline.
//
// THE SCALE IS NOT UNIFORM, WHICH IS WHY THIS DOES NOT USE quad_at. A line of
// writing is wide and short and the quad it sits on has to be the same shape.
static voe_scene_transform_intent behind_the_line(const voe_ecs_world *world,
						  voe_ecs_entity eye,
						  voe_ecs_entity quad,
						  voe_math_float2 size)
{
	struct view_basis basis = basis_of(world, eye);
	float margin = size.y * HUD_PANEL_MARGIN;
	voe_math_float3 at = voe_math_float3_add(
		basis.eye, voe_math_float3_scale(basis.forward,
						 HUD_DISTANCE + HUD_PANEL_BEHIND));

	at = voe_math_float3_add(at, voe_math_float3_scale(basis.up, -HUD_DROP));
	at = voe_math_float3_add(at,
				 voe_math_float3_scale(basis.up, size.y * 0.25f));

	return square_to_the_camera(&basis, quad, at,
				    (voe_math_float3){ size.x + margin * 2.0f,
						       size.y + margin * 2.0f,
						       1.0f });
}

// One model, then one transform intent per entity to move the whole thing aside.
//
// EVERY ENTITY HAS TO BE MOVED AND NOT JUST THE FIRST, WHICH IS THE FLATTENING
// SHOWING THROUGH. There is no parent component: the import composed the file's
// tree into world transforms, so moving a model means moving each of the things
// it turned into. The card that adds a hierarchy is the card that makes this one
// intent.
static bool add_a_model(voe_ecs_world *world, voe_render_device *gpu,
			voe_base_arena *arena, const char *name,
			const uint8_t *bytes, size_t size, float offset_x,
			voe_base_error *error)
{
	voe_3d_import imported = { 0 };
	// Everything the import builds on the way through — the parsed model,
	// the decoded picture, the JSON, the list of entities — is scratch: the
	// GPU has taken its own copy of the uploads and the components hold the
	// ids, so none of it is read after this function returns. It is a few
	// megabytes and this is the mark that gives them back.
	struct voe_base_arena_mark mark = voe_base_arena_mark(arena);

	if (!voe_3d_import_glb(world, gpu, arena, bytes, size, &imported,
			       error)) {
		voe_base_arena_rewind(arena, mark);
		return false;
	}

	for (uint32_t i = 0; i < imported.entity_count; i++) {
		const voe_scene_transform *placed =
			voe_scene_transform_get(world, imported.entities[i]);
		voe_scene_transform moved;

		if (placed == NULL)
			continue;

		// Read, change, submit: an intent carries the whole transform,
		// so a submitter reads the current one first. Reading is
		// anybody's; writing is the transform system's.
		moved = *placed;
		moved.position.x += offset_x;
		if (!voe_scene_transform_submit(
			    world, (voe_scene_transform_intent){
					   .entity = imported.entities[i],
					   .transform = moved })) {
			voe_base_arena_rewind(arena, mark);
			return false;
		}
	}

	// Textures and not pictures: a picture wanted as both a colour and a data
	// map is uploaded twice, so the two numbers are not always the same. See
	// 3d/import.h.
	printf("model      %-9s %u entities, %u meshes, %u materials, %u textures\n",
	       name, imported.entity_count, imported.geometry_count,
	       imported.material_count, imported.texture_count);

	// The intents carry the transforms by value, so nothing above is read
	// again and the whole import's working memory goes back here.
	voe_base_arena_rewind(arena, mark);
	return true;
}

// The four numbers the loop measures, and the clock reading that says when a
// period ends. One struct because they are gathered together, reported together
// and reset together, and four loose pairs at the top of main() would be twelve
// variables to keep in step.
//
// EACH ONE BRACKETS EXACTLY ONE THING AND THE NAMES BELOW ARE THE WHOLE POINT. A
// single "frame time" would hide which of the four is the one that got longer,
// and that is the question a person is asking when they look at this at all.
struct timing {
	// One top of the loop to the next. Everything is inside it and its
	// average is what "frames per second" is the reciprocal of.
	voe_base_samples frame;
	// The poll, the input, and the three systems. This program's own work
	// before it asks the GPU for anything.
	voe_base_samples update;
	// The loop's draw phase, _begin to _end inclusive: waiting for the frame
	// slot, taking a swapchain image, laying out and uploading the readout,
	// recording every draw, submitting and presenting.
	voe_base_samples draw;
	// The graphics card's own clock, over that frame's commands only.
	voe_base_samples gpu;
	// The last full period's averages of the two numbers measured AFTER the
	// readout is built, and — for the card — whether it has ever answered.
	//
	// THEY EXIST BECAUSE THE PERIOD IS RESET WITH THE READOUT STILL
	// RUNNING, AND THEY ARE FOR THE READOUT AND NOT THE CONSOLE. `draw` and
	// `gpu` are sampled below build_the_readout in the loop, so for the one
	// frame after every console block their sample sets are empty while the
	// readout is being built — and voe_base_samples_average of nothing is
	// nought. Showing that nought is a draw phase that appears to have taken
	// no time at all, once every period, which is a number a person would
	// believe. `frame` and `update` need none of this: they are sampled
	// ABOVE build_the_readout, so they always hold at least this frame.
	bool gpu_timed;
	double gpu_last;
	double draw_last;
	// Draw commands per completed frame, averaged and worsted over the
	// period exactly as the four durations above are.
	//
	// IT IS A METRIC AND NOT A SNAPSHOT, WHICH IS THE PRINCIPAL'S CALL AND
	// REVERSES WHAT CARD 040 FIRST SAID. That card argued a draw count is
	// exact and that averaging it turns a true number into a smeared one.
	// That is right about this demo, where the scene is identical every
	// frame and the average is 32.0 for ever — and it is wrong about the
	// direction of travel. The moment anything varies what is drawn, culling
	// or streaming or an interface that grows, the number a person needs is
	// the WORST frame in the period rather than the typical one, which is
	// the same reason `frame` and `gpu` carry a worst beside their average.
	// A metric that only becomes interesting later is still the right shape
	// now; a snapshot that has to be replaced later is not.
	//
	// IT IS A voe_base_samples EVEN THOUGH A COUNT IS NOT A DURATION. That
	// type's real constraint is that `worst` is the LARGEST value, so a
	// quantity where small is bad does not belong in it. More draw commands
	// is worse, so this belongs. See base/include/base/samples.h, whose
	// header still says everything measured with one is a duration — true of
	// every other caller and no longer of this one.
	voe_base_samples draws;
	// What the readout shows for the one frame per period that has no sample
	// yet, and whether any frame has completed at all. The same three cases
	// `draw` and `gpu` have above and for the same reason: the period is
	// reset with the readout still running.
	bool draws_measured;
	double draws_last;
	double draws_last_worst;
	// And how many element records the last completed frame submitted, which
	// is the other half of ADR-0092's claim and the reason the pair is worth
	// showing.
	//
	// THIS ONE IS NOT AVERAGED AND THAT IS NOT AN OVERSIGHT. The claim is
	// "this many records cost that few commands", and a record count averaged
	// over a period no longer lines up with the commands beside it — one
	// would describe the period and the other a frame in it. The exact count
	// from the same completed frame keeps the pair a pair.
	//
	// THE GAP BETWEEN THE TWO IS THE CLAIM, AND NEITHER NUMBER SAYS IT
	// ALONE. Ninety-odd records and thirty-odd commands is "many small
	// things in one draw" stated rather than asserted; the commands alone
	// could be thirty-two of anything, and the records alone say nothing
	// about what they cost. See voe_render_frame_elements_submitted, whose
	// own header is where that gap is spelled out.
	uint32_t drawn_elements;
	// When this period began, on the same clock every sample is taken with.
	// Kept rather than a deadline, because the rate printed has to be over
	// the time the period really covered and a frame always straddles the
	// end of one.
	double started;
};

// The legend, once, at startup. It is here and not repeated in every block
// because the four names never change and the block is meant to be glanced at,
// and it is printed at all because a number whose meaning is ambiguous is worse
// than no number.
static void say_what_is_measured(void)
{
	printf("timing     four numbers every %.0f s, each averaged over that period with its worst\n",
	       REPORT_SECONDS);
	printf("           frame   one top of the loop to the next. The rate is its reciprocal\n");
	printf("           update  the poll, the input and the three systems\n");
	printf("           draw    the loop's draw phase, begin to end: the wait for the frame\n");
	printf("                   slot, the acquire, the readout's layout and upload, the\n");
	printf("                   recording, the submit and the present. On fifo the wait\n");
	printf("                   for the display is in here and is most of it\n");
	printf("           gpu     the graphics card's own clock, over that frame's commands\n");
	printf("                   only — the wait for the display is not in it. It runs two\n");
	printf("                   frames behind, and is absent on a card that cannot time\n");
	printf("           P switches between fifo and mailbox. Numbers from both are what\n");
	printf("           the frame-pacing decision wants; the mode is on every block below\n");
	printf("           The same five numbers stand top-left of the view as the period's\n");
	printf("           running averages, laid out again every frame\n");
}

// The readout's text this frame, out of what the period has measured so far,
// laid out through the transient path and put on the readout entity. Called
// between _begin and the draw system, which is the only place it can be called.
//
// THE NUMBERS ARE THE RUNNING AVERAGES OF THE CURRENT PERIOD. The console block
// prints a period's average and worst once the period is over; this prints the
// average so far, every frame, so it settles over the first few hundred
// milliseconds of a period and starts again with the next block. The rate is the
// reciprocal of the frame average rather than a count over elapsed time, because
// the frame sample always holds at least this frame. For the one frame after
// each console block the other three read nought — the period has no sample of
// them yet — and printing last period's number instead would be the cache this
// card forbids in a smaller coat.
//
// THERE IS NO CACHE AND NO "HAS IT CHANGED". The string is formatted and laid
// out every frame whether or not a digit moved. It is a few dozen glyphs and
// well under a tenth of a millisecond, and it is the design that was decided; a
// readout that stops changing is therefore a bug in the transient path and never
// a cache being clever.
//
// `glyphs` comes back as how many characters were laid out, so that what the
// readout actually consumed can be printed once beside what was asked for.
static bool build_the_readout(voe_ecs_world *world, voe_render_device *gpu,
			      const voe_text_font *font, voe_base_arena *arena,
			      voe_ecs_entity readout, voe_platform_window *window,
			      const struct timing *timing, uint32_t *glyphs,
			      voe_base_error *error)
{
	char gpu_line[READOUT_CHARS];
	char draws_line[READOUT_CHARS];
	char mouse_line[READOUT_CHARS];
	char text[READOUT_CHARS];
	voe_text_block block;
	double frame = voe_base_samples_average(&timing->frame);
	// The same two cases gpu has below, and for the same reason: the period
	// was reset with this readout still running, so for one frame per period
	// there is no sample of the draw phase yet and last period's average is
	// what holds the line still. Before the first console block there is no
	// last one either, and nought is then the truth rather than a stale
	// number.
	double draw = timing->draw.count > 0 ?
			      voe_base_samples_average(&timing->draw) :
			      timing->draw_last;
	uint32_t drawn = 0;
	voe_platform_pointer pointer = voe_platform_input_pointer(window);
	// The buttons as three letters in the order they sit on a mouse, a dot
	// for one that is up. Read every frame like everything else here: this
	// is a call site showing what platform hands out, not a GUI.
	char left = voe_platform_input_button_down(window,
						   VOE_PLATFORM_BUTTON_LEFT) ?
			    'L' :
			    '.';
	char middle = voe_platform_input_button_down(
			      window, VOE_PLATFORM_BUTTON_MIDDLE) ?
			      'M' :
			      '.';
	char right = voe_platform_input_button_down(
			     window, VOE_PLATFORM_BUTTON_RIGHT) ?
			     'R' :
			     '.';

	// THREE CASES AND NOT TWO, BECAUSE THE PERIOD IS RESET WITH THE READOUT
	// STILL RUNNING. A period with samples shows its running average. A
	// period with none yet — the one frame after each console block, since
	// the timestamp is asked for after the draw and this is built before it
	// — shows the last full period's average, so the line holds still
	// rather than flashing the fallback below for a single frame that
	// mailbox sometimes presents. Only a card that has never answered gets
	// the fallback, which is then the truth.
	if (timing->gpu.count > 0)
		snprintf(gpu_line, sizeof gpu_line, "gpu    %6.2f ms",
			 voe_base_samples_average(&timing->gpu) * 1000.0);
	else if (timing->gpu_timed)
		snprintf(gpu_line, sizeof gpu_line, "gpu    %6.2f ms",
			 timing->gpu_last * 1000.0);
	else
		snprintf(gpu_line, sizeof gpu_line, "gpu    no measurement");

	// THREE CASES, THE SAME THREE THE GPU LINE HAS AND FOR THE SAME REASON.
	// A period with samples shows its running average and worst. The one
	// frame after each console block has neither yet — the count is added
	// after the draw and this is built before it — and shows the last full
	// period's, so the line holds still rather than flashing. Only a program
	// that has not completed a frame at all gets the third case, which is
	// then the truth.
	//
	// AND THE COUNT INCLUDES THE DRAW THAT PUT THIS READOUT ON SCREEN. The
	// readout is a mesh like any other and is drawn like any other, so a
	// person counting the things they can see will find one fewer than this
	// says. That is correct and it is not adjusted for: subtracting it would
	// make this a number about something other than what the frame did.
	//
	// THE RECORDS BESIDE IT ARE ONE FRAME'S AND THE COMMANDS ARE A PERIOD'S,
	// which is a mixture on one line and is deliberate — see struct timing.
	// The pair only means something if both halves describe the same drawing.
	if (timing->draws.count > 0)
		snprintf(draws_line, sizeof draws_line,
			 "draws  %5.1f avg %4.0f worst  %u elements",
			 voe_base_samples_average(&timing->draws),
			 timing->draws.worst,
			 (unsigned)timing->drawn_elements);
	else if (timing->draws_measured)
		snprintf(draws_line, sizeof draws_line,
			 "draws  %5.1f avg %4.0f worst  %u elements",
			 timing->draws_last, timing->draws_last_worst,
			 (unsigned)timing->drawn_elements);
	else
		snprintf(draws_line, sizeof draws_line, "draws  no frame yet");

	// Three states and three words, because a number that looked live
	// while the pointer was locked or gone is exactly what the platform
	// header refuses to hand out. The position is printed whole: Wayland
	// reports fractions and they are not interesting to a person.
	if (pointer.over)
		snprintf(mouse_line, sizeof mouse_line,
			 "mouse  %5.0f %5.0f %c%c%c", pointer.x, pointer.y,
			 left, middle, right);
	else if (voe_platform_input_pointer_locked(window))
		snprintf(mouse_line, sizeof mouse_line, "mouse  locked      %c%c%c",
			 left, middle, right);
	else
		snprintf(mouse_line, sizeof mouse_line, "mouse  away        %c%c%c",
			 left, middle, right);

	snprintf(text, sizeof text,
		 "%5.0f fps\nframe  %6.2f ms\nupdate %6.2f ms\ndraw   %6.2f ms\n%s\n%s\n%s",
		 frame > 0.0 ? 1.0 / frame : 0.0, frame * 1000.0,
		 voe_base_samples_average(&timing->update) * 1000.0,
		 draw * 1000.0, gpu_line, draws_line, mouse_line);

	// Spaces and newlines lay out nothing; every other character in this
	// string is a glyph the font carries.
	for (const char *at = text; *at != '\0'; at++)
		if (*at != ' ' && *at != '\n')
			drawn++;
	*glyphs = drawn;

	if (!voe_text_block_create_transient(font, gpu, arena, text, READOUT_EM,
					     &block, error))
		return false;

	// The entity is this program's and is never destroyed, so a mesh that
	// is not there is a bug here and not a thing to handle.
	VOE_BASE_ASSERT(voe_3d_mesh_set_geometry(world, readout, block.geometry),
			"the readout entity has lost its mesh");
	return true;
}

// One block, and then the period starts again. `seconds` is how long the period
// really lasted rather than REPORT_SECONDS, because a frame straddles the end of
// one and the rate has to be over the time actually covered.
//
// THE GPU LINE IS ABSENT RATHER THAN NOUGHT WHEN THERE IS NO MEASUREMENT. A card
// that cannot write timestamps would otherwise report a graphics card that takes
// no time at all, which is the most misleading thing this could print.
static void report(struct timing *timing, double seconds,
		   voe_render_present present)
{
	printf("timing     %llu frames in %.2f s — %.1f per second, %s\n",
	       (unsigned long long)timing->frame.count, seconds,
	       seconds > 0.0 ? (double)timing->frame.count / seconds : 0.0,
	       present == VOE_RENDER_PRESENT_MAILBOX ? "mailbox" : "fifo");
	printf("           frame  %7.2f ms avg  %7.2f ms worst\n",
	       voe_base_samples_average(&timing->frame) * 1000.0,
	       timing->frame.worst * 1000.0);
	printf("           update %7.2f ms avg  %7.2f ms worst\n",
	       voe_base_samples_average(&timing->update) * 1000.0,
	       timing->update.worst * 1000.0);
	printf("           draw   %7.2f ms avg  %7.2f ms worst\n",
	       voe_base_samples_average(&timing->draw) * 1000.0,
	       timing->draw.worst * 1000.0);
	if (timing->gpu.count > 0)
		printf("           gpu    %7.2f ms avg  %7.2f ms worst\n",
		       voe_base_samples_average(&timing->gpu) * 1000.0,
		       timing->gpu.worst * 1000.0);
	else
		printf("           gpu        no measurement — this card or its queue cannot write timestamps\n");
	// The one line here that is not milliseconds, in the same two columns as
	// the four that are. What varies it is the scene rather than the
	// machine, so in this demo the average and the worst are the same number
	// — and the day they differ is the day something started drawing more in
	// some frames than others, which is exactly what a worst column is for.
	printf("           draws  %7.1f    avg  %7.0f    worst\n",
	       voe_base_samples_average(&timing->draws), timing->draws.worst);
	fflush(stdout);

	// What the readout shows until this new period has a sample of its own.
	// Only the two measured after it is built need it; see struct timing.
	if (timing->draw.count > 0)
		timing->draw_last = voe_base_samples_average(&timing->draw);
	if (timing->gpu.count > 0)
		timing->gpu_last = voe_base_samples_average(&timing->gpu);
	// Both halves of this one, because the readout shows both.
	if (timing->draws.count > 0) {
		timing->draws_last = voe_base_samples_average(&timing->draws);
		timing->draws_last_worst = timing->draws.worst;
	}

	voe_base_samples_reset(&timing->frame);
	voe_base_samples_reset(&timing->update);
	voe_base_samples_reset(&timing->draw);
	voe_base_samples_reset(&timing->gpu);
	voe_base_samples_reset(&timing->draws);
}

int main(void)
{
	// The window and the device, opened together by `app` and reached
	// through it. Both are taken into locals once, below, because this file
	// names them on nearly every line and voe_app_window(app) on each of
	// them would say nothing the name does not.
	voe_app *app;
	voe_platform_window *window;
	voe_base_arena *scratch;
	voe_base_arena *arena;
	voe_ecs_world *world;
	voe_render_device *gpu;
	voe_base_error error = VOE_BASE_OK;
	voe_platform_size size;
	voe_ecs_entity eye = { 0 };
	voe_ecs_entity sun = { 0 };
	voe_ecs_entity turning = { 0 };
	voe_ecs_entity hud = { 0 };
	voe_ecs_entity panel = { 0 };
	voe_ecs_entity readout = { 0 };
	// The two element surfaces that are entities: the exhibit standing in
	// the world and the badge in the overlay. Their ranges are rewritten
	// every frame — see the loop.
	voe_ecs_entity exhibit_panel = { 0 };
	voe_ecs_entity badge_panel = { 0 };
	// How many glyphs the readout laid out, printed once against the room
	// made for it — see MAX_TRANSIENT_GLYPHS.
	uint32_t readout_glyphs = 0;
	bool readout_reported = false;
	// Whether every surface's submits and draws were accepted, and the
	// frame's draw count either side of the draw system's walk — the
	// difference is what the walk cost, which is every mesh plus one per
	// panel.
	//
	// THE FIRST IS THE MONITOR'S WHOLE PASS, WHICH IS WHY THE PAIR IS A
	// SUBTRACTION AND NOT ONE READ. The monitor walks the same world into
	// its own target before the window's pass opens, so by the time the
	// window's walk starts the frame already holds that pass's draws. The
	// difference is still exactly what the window's walk cost, which is what
	// the line printed below claims it is.
	bool elements_ok = true;
	uint32_t draws_before_walk = 0;
	uint32_t draws_after_walk = 0;
	// Where each panel's own records start in this frame's element buffer.
	// One buffer, three ranges: the exhibit, the badge and the
	// screen-filling surface, which takes its own.
	uint32_t elements_first = 0;
	uint32_t elements_count = 0;
	uint32_t badge_first = 0;
	voe_render_geometry quad = { 0 };
	voe_dev_sprites sprites = { 0 };
	// The second camera's picture and the screen in the world that shows it.
	// Its target, its camera and its screen entity are all made once, below;
	// the loop asks it only what its pass is drawn with.
	voe_dev_monitor monitor = { 0 };
	voe_text_font *font = NULL;
	voe_ui_context *interface = NULL;
	uint32_t interface_elements = 0;
	voe_math_float2 hud_size = { 0.0f, 0.0f };
	voe_math_float3 spin_axis = { SPIN_AXIS_X, SPIN_AXIS_Y, SPIN_AXIS_Z };
	voe_render_capacities capacities = {
		.vertices = MAX_VERTICES,
		.indices = MAX_INDICES,
		.geometries = MAX_MESHES,
		.objects = MAX_DRAWN_OBJECTS,
		.shadings = MAX_SHADINGS,
		.transient_vertices = 4 * MAX_TRANSIENT_GLYPHS,
		.transient_indices = 6 * MAX_TRANSIENT_GLYPHS,
		.transient_geometries = MAX_TRANSIENT_GEOMETRIES,
		// Everything every surface in the frame submits, into the one
		// buffer: the exhibit, the badge and the screen-filling
		// surface. Four ranges of it now — the interface is the
		// fourth, and its number is the only one of the four that is a
		// ceiling rather than a count, because `ui` emits one record
		// per letter of whatever the labels happen to say.
		.elements = VOE_DEV_ELEMENTS + VOE_DEV_BADGE_ELEMENTS +
			    VOE_DEV_SURFACE_ELEMENTS +
			    VOE_DEV_INTERFACE_ELEMENTS,
		// Two passes a frame: the monitor's, onto its own target with
		// its own camera, and then the window's with the world's
		// camera. Everything this program draws is inside one of the
		// two, and the monitor's target is the one target it makes.
		.passes = MAX_PASSES,
		.targets = MAX_TARGETS,
	};
	voe_ecs_limits limits = {
		.entities = MAX_ENTITIES,
		.component_types = MAX_COMPONENT_TYPES,
		.intent_types = MAX_INTENT_TYPES,
	};
	voe_scene_camera camera = {
		.fov_y = FIELD_OF_VIEW,
		.near_plane = NEAR_PLANE,
		.far_plane = FAR_PLANE,
	};
	bool decorated;
	bool flying = false;
	bool was_flying = false;
	bool locked = false;
	// Last frame's Tab, because a toggle is an edge and platform hands out
	// state. Two bools at a call site is what include/platform/input.h says
	// this costs instead of an event queue, and this is that call site.
	bool tab_was_down = false;
	// And last frame's Escape, for the same reason and one more: Escape does
	// two different things depending on which camera is in force, so held
	// down it would do both, one frame after the other.
	bool escape_was_down = false;
	// Last frame's P, for the same reason, and the mode it asks for. It
	// starts true because this program asks for mailbox as soon as the device
	// is open, with this variable, so the first press of P asks for fifo
	// rather than for what is already happening.
	//
	// What is actually in force is the device's answer and is asked for
	// rather than remembered: a surface with no mailbox leaves this true and
	// the device on fifo, and printing what was asked for would be a lie.
	bool p_was_down = false;
	bool mailbox_wanted = true;
	voe_render_present present;
	// The scene's clock: measured now, and the sum of every step taken, not
	// of every second that passed. See MAX_FRAME_SECONDS and the skip below.
	float seconds = 0.0f;
	// The real clock, and what it is read into. `top` is this frame's
	// reading, taken off the tick `app` hands back rather than read again
	// here, so the interval the readout reports and the step the scene takes
	// are the same subtraction — see voe_app_frame_open.
	struct timing timing = { 0 };
	double top;
	double after_update;
	double after_draw;
	double step;

	// The world and everything read into it live here, and it is destroyed
	// at the end: the world is the arena's, which is what rule 11 asks for.
	// It is made before the window now, because the app struct lives in it
	// too and has to outlive every call made through it.
	arena = voe_base_arena_new(WORLD_ARENA);

	// The window and the device, in one call and in that order. Nothing is
	// kept out of `scratch`, so it goes as soon as this returns.
	//
	// NOTHING IS PRINTED HERE ON A FAILURE AND THAT IS NOT AN OVERSIGHT.
	// `app` says which of the two refused and `render` says why, both on
	// stderr, before this returns NULL; a third line from this file would
	// only repeat them.
	scratch = voe_base_arena_new(STARTUP_SCRATCH);
	app = voe_app_new(arena, scratch,
			  (voe_app_settings){
				  .width = 960,
				  .height = 540,
				  .title = "voe3d — a model, two cubes, one camera",
				  .capacities = capacities,
				  .longest_step = MAX_FRAME_SECONDS },
			  &error);
	voe_base_arena_destroy(scratch);
	if (app == NULL) {
		voe_base_arena_destroy(arena);
		return 1;
	}

	window = voe_app_window(app);
	gpu = voe_app_device(app);

	// Mailbox, asked for rather than inherited: a device opens on fifo, and
	// `app` asks for no mode at all, so this is the program's own request. It
	// is the call P makes and the variable P flips, so what was asked for at
	// startup and what the first press takes back cannot disagree.
	voe_render_present_set(gpu, mailbox_wanted ? VOE_RENDER_PRESENT_MAILBOX :
						     VOE_RENDER_PRESENT_FIFO);

	world = voe_ecs_world_new(arena, limits);

	// Registration, once, and this is the whole of what a call site has to
	// know about which components exist. Each folder says what one of its
	// components is; nothing here does.
	voe_scene_transform_register(world, MAX_ENTITIES);
	voe_scene_camera_register(world, 4);
	voe_scene_light_register(world, 4);
	voe_3d_mesh_register(world, MAX_ENTITIES);
	voe_3d_material_register(world, MAX_ENTITIES);
	// The second kind of drawable. Two of them in this program, and the
	// table is registered like any other component — which is the whole of
	// what a panel costs a call site.
	voe_3d_panel_register(world, MAX_ENTITIES);

	if (!voe_ecs_entity_create(world, &eye) ||
	    !voe_scene_camera_add(world, eye, camera)) {
		VOE_BASE_ERROR("dev", "could not make a camera");
		goto stop;
	}

	// The sun, at wherever its lap starts. The draw system needs exactly one
	// light in the world, so this is not optional wiring — a world without it
	// asserts rather than drawing something black.
	if (!voe_ecs_entity_create(world, &sun) ||
	    !voe_scene_light_add(world, sun, sunlight(sun, 0.0f).light)) {
		VOE_BASE_ERROR("dev", "could not make a sun");
		goto stop;
	}

	if (!add_the_cubes(world, gpu, arena, &turning, &error)) {
		VOE_BASE_ERROR("dev", "could not build the two cubes: %s",
			       voe_base_error_string(error));
		goto stop;
	}

	if (!add_the_quads(world, gpu, &quad, &error)) {
		VOE_BASE_ERROR("dev", "could not build the two see-through quads: %s",
			       voe_base_error_string(error));
		goto stop;
	}

	if (!voe_dev_sprites_add(world, gpu, arena, &sprites, &error)) {
		VOE_BASE_ERROR("dev", "could not build the sprites: %s",
			       voe_base_error_string(error));
		goto stop;
	}

	// The font and the three text entities. Not optional the way a model is:
	// there is one font, it is in the binary, and a failure here is a bug in
	// the reader rather than a file somebody could not open.
	if (!add_the_text(world, gpu, arena, quad, &font, &hud, &panel,
			  &readout, &hud_size, &error)) {
		VOE_BASE_ERROR("dev", "could not build the text: %s",
			       voe_base_error_string(error));
		goto stop;
	}

	// The interface, which needs the font and so cannot be made before it.
	// It is the context and nothing else — what is on the interface is
	// built afresh every frame inside the loop.
	interface = voe_dev_interface_new(arena, font);

	// The two panels. Nothing is on them yet: what a panel holds is a range
	// of the frame that is open, and no frame is open until the loop starts.
	if (!add_panel(world,
		       (voe_math_float3){ EXHIBIT_X, EXHIBIT_Y, EXHIBIT_Z },
		       EXHIBIT_SCALE,
		       (voe_math_float2){ VOE_DEV_ELEMENTS_PANEL_WIDE,
					  VOE_DEV_ELEMENTS_PANEL_HIGH },
		       VOE_3D_LAYER_WORLD, &exhibit_panel) ||
	    !add_panel(world, (voe_math_float3){ BADGE_X, BADGE_Y, BADGE_Z },
		       BADGE_SCALE,
		       (voe_math_float2){ VOE_DEV_BADGE_WIDE,
					  VOE_DEV_BADGE_HIGH },
		       VOE_3D_LAYER_OVERLAY, &badge_panel)) {
		VOE_BASE_ERROR("dev", "could not build the two element panels");
		goto stop;
	}

	// The model is the one thing here that is allowed to fail without
	// stopping the program: the cubes are what says the renderer works, and
	// a person looking at a window is better served by seeing them and a
	// message than by seeing nothing.
	if (!add_a_model(world, gpu, arena, "human", HUMAN_GLB,
			 sizeof(HUMAN_GLB), HUMAN_X, &error))
		VOE_BASE_ERROR("dev", "could not read the human model: %s",
			       voe_base_error_string(error));

	// The monitor, last, because its screen stands in the world everything
	// above just built and its picture is that world seen from somewhere
	// else. Making a target waits for the card, so it happens here and never
	// in the loop — see src/monitor.h.
	if (!voe_dev_monitor_create(&monitor, world, gpu, &error)) {
		VOE_BASE_ERROR("dev", "could not build the monitor: %s",
			       voe_base_error_string(error));
		goto stop;
	}

	size = voe_platform_window_size(window);
	decorated = voe_platform_window_decorated(window);
	printf("opened     %dx%d\n", size.width, size.height);
	printf("decorated  %s\n", decorated ? "yes" : "no");
	printf("camera     orbit — Tab to fly, Escape to hand back or close\n");
	// Fifo here, whatever was asked for above: the swapchain the device
	// opened with is already built on fifo, and the request is acted on at
	// the top of the first frame. Every block from then on says what is in
	// force.
	present = voe_render_present_get(gpu);
	printf("present    %s\n",
	       present == VOE_RENDER_PRESENT_MAILBOX ? "mailbox" : "fifo");
	say_what_is_measured();
	fflush(stdout);

	// When the first reporting period started. The frame's own interval is
	// the clock inside `app` and this file no longer keeps a previous
	// reading of its own.
	timing.started = voe_platform_clock_now();

	// THE LOOP IS THIS FILE'S AND THE PARTS IN IT ARE `app`'S (ADR-0135).
	// Everything between the calls below — the key edges, the order the
	// systems run in, what is submitted in the gap between two of them — is
	// this program deciding, which is why `app` hands back a frame instead
	// of running one.
	while (true) {
		voe_app_frame opened;
		voe_platform_size now_size;
		bool now_decorated;
		bool now_locked;
		bool tab_down;
		bool escape_down;
		bool p_down;
		const voe_scene_transform *spinning;
		double gpu_seconds;
		voe_3d_frame frame;
		bool drawing = false;
		bool readout_ok = true;

		// The clock, the poll and what the window says afterwards, in
		// that order and once. The window closing is the only reason
		// this loop ends that is not a key or a failure.
		opened = voe_app_frame_open(app);
		if (opened.closing)
			break;

		// The reading that opened the frame, kept because the update
		// and the draw are measured from it. Everything below is inside
		// the interval it ends.
		top = opened.tick.now;

		// WHAT IS REPORTED AND WHAT THE SCENE TAKES ARE THE TWO NUMBERS
		// ON THE TICK, AND THEY ARE NOT THE SAME ONE. `elapsed` is what
		// really happened and is what the readout says; `step` is that
		// with MAX_FRAME_SECONDS on it and is all the orbit, the spin
		// and the sun are advanced by. The first tick has no interval
		// behind it and says so, and recording it would make the
		// readout's first line claim four million frames a second.
		if (!opened.tick.first)
			voe_base_samples_add(&timing.frame, opened.tick.elapsed);
		step = opened.tick.step;

		// The frame opened with a poll in it, so what follows is this
		// program reading state that is already this frame's. Everything
		// is asked every frame because platform hands out state, not
		// events.
		now_size = opened.size;
		if (now_size.width != size.width ||
		    now_size.height != size.height) {
			size = now_size;
			printf("size       %dx%d\n", size.width, size.height);
			fflush(stdout);
		}

		now_decorated = voe_platform_window_decorated(window);
		if (now_decorated != decorated) {
			decorated = now_decorated;
			printf("decorated  %s\n", decorated ? "yes" : "no");
			fflush(stdout);
		}

		// Tab toggles on the press and not while held, which is what
		// turning state back into an edge means. Escape and P below do
		// the same and for the same reason: `platform` hands out which
		// keys are down, and all three of these are actions rather than
		// things held.
		tab_down = voe_platform_input_key_down(window,
						       VOE_PLATFORM_KEY_TAB);
		if (tab_down && !tab_was_down)
			flying = !flying;
		tab_was_down = tab_down;

		// ESCAPE HANDS THE CAMERA BACK, AND ESCAPE WITH THE CAMERA
		// ALREADY BACK CLOSES THE WINDOW. Two presses and not one,
		// because the first thing anybody wants out of a locked pointer
		// is the pointer: a key that released the pointer and quit in
		// the same press would quit every time somebody wanted their
		// mouse back.
		//
		// AND IT HAS TO BE AN EDGE, WHICH IT DID NOT WHEN IT ONLY EVER
		// HANDED BACK. Held down, one frame would hand the camera back
		// and the very next would close the window, so a single long
		// press would look like the program exiting for no reason.
		escape_down = voe_platform_input_key_down(
			window, VOE_PLATFORM_KEY_ESCAPE);
		if (escape_down && !escape_was_down) {
			if (flying)
				flying = false;
			else
				break;
		}
		escape_was_down = escape_down;

		// P asks for the other present mode, on the press and not while
		// held, exactly as Tab does. What the device does about it is
		// asked for below rather than assumed: a surface with no mailbox
		// stays on fifo however often this is pressed, and that is a
		// measurement of the machine rather than a failure.
		p_down = voe_platform_input_key_down(window, VOE_PLATFORM_KEY_P);
		if (p_down && !p_was_down) {
			mailbox_wanted = !mailbox_wanted;
			voe_render_present_set(gpu,
					       mailbox_wanted ?
						       VOE_RENDER_PRESENT_MAILBOX :
						       VOE_RENDER_PRESENT_FIFO);
		}
		p_was_down = p_down;

		if (flying != was_flying) {
			was_flying = flying;
			printf("camera     %s\n", flying ? "flying" : "orbit");
			fflush(stdout);
		}

		// Asked every frame rather than on the change, because a lock is
		// a request the window system may have taken away — losing focus
		// takes it — and asking again is how it comes back.
		voe_platform_input_lock_pointer(window, flying);

		now_locked = voe_platform_input_pointer_locked(window);
		if (now_locked != locked) {
			locked = now_locked;
			printf("locked     %s\n", locked ? "yes" : "no");
			fflush(stdout);
		}

		// The clock, and then everything that moves on it. A minimised
		// window draws nothing, and the clock stops with it: nothing
		// below advances a scene nobody is looking at. The loop still
		// goes round rather than skipping to the top, because the draw
		// below is what finds out when there is something to draw into
		// again.
		if (!opened.minimised) {
			seconds += (float)step;

			// One of the two, never both, and the camera system
			// applies placements before motions — so a frame that
			// submitted both would take the hand's answer, which is
			// exactly what a handover wants.
			if (flying)
				(void)voe_scene_camera_move(
					world,
					camera_motion(window, eye, step));
			else
				(void)voe_scene_camera_place(
					world, orbit(eye, seconds));

			// The sun, as an intent like everything else.
			(void)voe_scene_light_submit(world,
						     sunlight(sun, seconds));

			// The turning cube, as an intent like everything else.
			spinning = voe_scene_transform_get(world, turning);
			if (spinning != NULL) {
				voe_scene_transform moved = *spinning;

				moved.rotation = voe_math_quat_from_axis_angle(
					spin_axis,
					seconds * TURN / SPIN_SECONDS);
				(void)voe_scene_transform_submit(
					world,
					(voe_scene_transform_intent){
						.entity = turning,
						.transform = moved });
			}
		}

		// The systems, in order, and then the draw. Each of them drains
		// what was submitted since it last ran; nothing here calls into
		// one system from another.
		voe_scene_camera_system_run(world);

		// THE HEADS-UP LINE IS PLACED HERE, BETWEEN TWO SYSTEMS, AND
		// THAT POSITION IS THE WHOLE OF WHETHER IT WORKS. It is derived
		// from where the camera is, so it has to be worked out after the
		// camera system has moved it and submitted before the transform
		// system drains — which is exactly this gap, and it costs
		// nothing: both systems still run once.
		//
		// PLACING IT UP WITH THE OTHER INTENTS PUTS IT ONE FRAME BEHIND,
		// AND ONE FRAME IS PLENTY. It reads as jitter rather than as
		// lag, and the reason is worth writing down because the frame
		// rate makes it look impossible: a mouse delivers motion in
		// lumps, so at a thousand frames a second most frames turn the
		// camera by nothing and the occasional one turns it by the whole
		// of a lump. A line placed from the previous frame's camera is
		// therefore not a fraction of a millimetre out — it is a whole
		// mouse movement out, for one frame — and mailbox shows whatever
		// frame happens to be newest when the display asks, so some of
		// those frames are the ones a person sees. Drawing faster makes
		// it worse rather than better.
		(void)voe_scene_transform_submit(
			world, facing_the_camera(world, eye, hud, hud_size));
		// The panel travels with the line, one frame behind it in
		// exactly the same way and for exactly the same reason.
		(void)voe_scene_transform_submit(
			world, behind_the_line(world, eye, panel, hud_size));
		// And the readout, placed from the camera alone — its geometry
		// does not exist yet and its placement does not need it.
		(void)voe_scene_transform_submit(
			world,
			top_left_of_the_view(world, eye, readout, now_size));
		// And the two sprites that turn towards the camera, in this
		// same gap and for this same reason. The engine does not
		// billboard, so this is a call site turning them itself — see
		// src/sprites.c.
		voe_dev_sprites_face(world, eye, &sprites);

		voe_scene_transform_system_run(world);
		voe_scene_light_system_run(world);

		// The line between `update` and `draw`, and the reason the two
		// are measured apart: everything above is this program's own
		// work and everything below is the GPU's frame, the wait for it
		// included. One number covering both would not say which of them
		// grew.
		after_update = voe_platform_clock_now();
		voe_base_samples_add(&timing.update, after_update - top);

		// THE FRAME, IN THE ORDER THE HEADER GIVES: the camera and the sun
		// out of the tables, the draw opened, build what changes this
		// frame, a pass onto the monitor's target with the monitor's
		// camera, a pass onto the window with the world's camera, the
		// walk in each of them, and the draw closed. An open that says
		// there is nothing to draw into skips everything in the middle
		// and the loop comes round again — it does not wait, which is
		// what the spin on a minimised window is.
		frame = voe_3d_draw_system_frame(world, now_size);
		if (!voe_app_draw_open(app, now_size, &drawing))
			break;
		if (drawing) {
			voe_render_pass_camera pass_camera = {
				.view = frame.view,
				.light = frame.light,
			};
			// The monitor's own answer to the same question, and the
			// one place in this program where a second camera
			// exists. Its `hidden` is its screen — see
			// src/monitor.h for why that is not optional.
			voe_3d_frame monitor_frame =
				voe_dev_monitor_frame(&monitor, world);
			voe_render_pass_camera monitor_camera = {
				.view = monitor_frame.view,
				.light = monitor_frame.light,
			};

			// Built inside the frame and before either pass, because
			// that is the only place a one-frame mesh can be built
			// and still be drawn — and because both passes walk the
			// world, so anything built after the first of them would
			// be missing from that one and stale in it the frame
			// after. Its failure is looked at after the frame has
			// been ended, so the slot's fence is never left waiting
			// on a frame that was abandoned half recorded.
			readout_ok = build_the_readout(world, gpu, font, arena,
						       readout, window, &timing,
						       &readout_glyphs, &error);

			// THE TWO PANELS' CONTENT, BEFORE THE WALK, WHICH IS
			// THE PHASE THIS EXISTS FOR. A panel holds a range of
			// the frame that is open and the buffer is empty at
			// the top of every frame, so a range not written here
			// is a panel that is not drawn. Read the count, submit
			// one surface, read it again, and the difference is
			// that surface's range — there is no id and nothing
			// allocated.
			//
			// AND BEFORE ANY PASS, WHICH IS ALLOWED AND IS WHAT
			// TWO PASSES NEED. Element submission belongs to the
			// frame and not to a pass (render/device.h), so both
			// the monitor's walk and the window's draw the same
			// ranges of the same buffer — which is why the panels
			// are on the picture the monitor shows.
			//
			// NOTHING IS DRAWN HERE. The draw system issues one
			// draw per panel a moment later, in layer and sort
			// order, which is what lets a cube stand in front of
			// the exhibit.
			elements_first = voe_render_frame_elements_submitted(gpu);
			elements_ok = voe_dev_elements_submit(gpu, font);
			elements_count =
				voe_render_frame_elements_submitted(gpu) -
				elements_first;
			(void)voe_3d_panel_set_range(world, exhibit_panel,
						     elements_first,
						     elements_count);

			badge_first = voe_render_frame_elements_submitted(gpu);
			elements_ok = voe_dev_elements_badge_submit(gpu) &&
				      elements_ok;
			(void)voe_3d_panel_set_range(
				world, badge_panel, badge_first,
				voe_render_frame_elements_submitted(gpu) -
					badge_first);

			// THE MONITOR'S PASS, AND IT IS FIRST BECAUSE THE
			// WINDOW'S SHOWS WHAT IT DREW. The same world, the same
			// walk and the same tables, from a camera of its own
			// into a target of its own; the screen that shows the
			// result is left out of it by monitor_frame.hidden.
			// Second would be a target read in one pass and written
			// in the next, and what the window showed would be the
			// picture of the frame before.
			if (!voe_render_pass_begin(gpu, monitor.target,
						   &monitor_camera)) {
				(void)voe_app_draw_close(app);
				break;
			}
			voe_3d_draw_system_run(world, gpu, arena,
					       monitor_frame);
			voe_render_pass_end(gpu);

			// The second pass of a frame on a device made with room
			// for two; refused only if that number were too small,
			// which is this file's mistake. The frame is still
			// closed on the way out, so its slot is not left half
			// recorded.
			if (!voe_render_pass_begin(gpu, VOE_RENDER_TARGET_WINDOW,
						   &pass_camera)) {
				(void)voe_app_draw_close(app);
				break;
			}

			// EITHER SIDE OF THE WALK, WHICH IS WHAT THESE TWO
			// BRACKET AND ALL THEY BRACKET. The difference is every
			// mesh drawn plus one per panel — the panels' own draws
			// are issued inside the walk, sorted among the
			// see-through meshes, so there is no moment between
			// them to read a count at and that is card 032 working
			// rather than something missing here.
			draws_before_walk = voe_render_frame_draw_count(gpu);

			voe_3d_draw_system_run(world, gpu, arena, frame);

			draws_after_walk = voe_render_frame_draw_count(gpu);

			// The screen-filling surface, after the walk because it
			// is not in the world and has nothing to sort against,
			// and before the end so that it is in this frame at
			// all. It is the one surface here that is not a panel
			// and the only one a resize changes — see src/surface.h.
			// Its failure is looked at after the frame has been
			// ended, for the reason the readout's is.
			elements_ok = voe_dev_surface_draw(gpu, now_size) &&
				      elements_ok;

			// And the interface, on the same terms and for the
			// same reasons: it is not in the world either. It goes
			// after the surface so that it is painted over it,
			// which is what submission order means on this path.
			elements_ok = voe_dev_interface_draw(
					      gpu, interface, arena, now_size,
					      voe_platform_input_pointer(window),
					      voe_platform_input_button_down(
						      window,
						      VOE_PLATFORM_BUTTON_LEFT),
					      voe_platform_input_key_down(
						      window,
						      VOE_PLATFORM_KEY_SHIFT),
					      &interface_elements) &&
				      elements_ok;

			voe_render_pass_end(gpu);
			if (!voe_app_draw_close(app))
				break;

			// THE FRAME IS COMPLETE, SO THIS IS THE ONE MOMENT
			// EITHER NUMBER IS THE WHOLE FRAME'S. Both are read
			// here, after _end, so that what the readout shows and
			// what the console block prints are the same two reads
			// — two numbers about one frame that disagreed would be
			// worse than either alone.
			//
			// THE PAIR IS THE CLAIM AND NEITHER HALF IS. Records
			// submitted against commands recorded is what "many
			// small things in one draw" means; see
			// voe_render_frame_elements_submitted, which says why
			// reading the wrong one of the two makes the claim
			// trivially true. Both survive until the next _begin,
			// which is what lets them be read after the frame has
			// been submitted.
			voe_base_samples_add(
				&timing.draws,
				(double)voe_render_frame_draw_count(gpu));
			timing.drawn_elements =
				voe_render_frame_elements_submitted(gpu);
			timing.draws_measured = true;

			// The element capacity is smaller than the three
			// surfaces need, which is this file's mistake in the
			// same way the readout's transient room would be.
			if (!elements_ok) {
				VOE_BASE_ERROR("dev",
					       "could not submit or draw an element surface — see the refusal above");
				break;
			}
			// A readout that could not be built means the transient
			// room above is too small for it, which is this file's
			// mistake and worth stopping over rather than a refusal
			// line on stderr every frame for as long as it runs.
			if (!readout_ok) {
				VOE_BASE_ERROR("dev",
					       "could not build the readout: %s",
					       voe_base_error_string(error));
				break;
			}
			if (!readout_reported) {
				printf("readout    %u glyphs — %u of %u transient vertices, %u of %u indices, 1 of %u ranges\n",
				       readout_glyphs, readout_glyphs * 4u,
				       4u * MAX_TRANSIENT_GLYPHS,
				       readout_glyphs * 6u,
				       6u * MAX_TRANSIENT_GLYPHS,
				       (unsigned)MAX_TRANSIENT_GEOMETRIES);
				// THREE ELEMENT SURFACES, AND THE TWO NUMBERS
				// AT THE END ARE THE SAME PAIR THE READOUT
				// SHOWS. They are read from the same two calls
				// after the same _end, so the console and the
				// screen cannot disagree — and the whole frame
				// is what is printed rather than the surfaces
				// alone, because the panels are drawn inside
				// the walk, sorted among the see-through
				// meshes, and there is no moment between them
				// to read a count at. That is card 032 working
				// rather than a limitation: a panel drawn in a
				// pass of its own would be easier to count and
				// would be the bug.
				//
				// The walk's own cost is printed beside it,
				// which is every mesh plus one per panel.
				printf("interface  %u element records for a panel, a heading, two buttons and three number boxes, in ONE draw command\n",
				       interface_elements);
				printf("elements   %u rectangles of %u colours and %u letters on the world panel, %u on the badge, %u on the screen-filling surface\n",
				       (unsigned)VOE_DEV_ELEMENTS_RECTANGLES,
				       (unsigned)VOE_DEV_ELEMENTS_RECTANGLES,
				       (unsigned)VOE_DEV_ELEMENTS_GLYPHS,
				       (unsigned)VOE_DEV_BADGE_ELEMENTS,
				       (unsigned)VOE_DEV_SURFACE_ELEMENTS);
				// This frame's exact numbers, read straight from
				// the device rather than out of the period
				// metric beside it: both survive until the next
				// _begin, and one frame's commands against one
				// frame's records is the pair ADR-0092's claim
				// is made of. The averaged version of the same
				// number is in every timing block below.
				printf("draws      %u commands for %u element records; the window's walk was %u of them\n",
				       voe_render_frame_draw_count(gpu),
				       timing.drawn_elements,
				       draws_after_walk - draws_before_walk);
				fflush(stdout);
				readout_reported = true;
			}
		}

		after_draw = voe_platform_clock_now();
		voe_base_samples_add(&timing.draw, after_draw - after_update);

		// The card's own measurement of a frame two frames back, when
		// there is one. Asked after the draw because that is what moved
		// it on; a card that cannot time never answers and the gpu line
		// says so rather than reading nought.
		if (voe_render_frame_gpu_time(gpu, &gpu_seconds)) {
			voe_base_samples_add(&timing.gpu, gpu_seconds);
			timing.gpu_timed = true;
		}

		// The mode is asked for every period rather than remembered,
		// because a rebuild is what puts a requested mode in force and
		// that happens inside the draw above.
		if (after_draw - timing.started >= REPORT_SECONDS) {
			present = voe_render_present_get(gpu);
			report(&timing, after_draw - timing.started, present);
			timing.started = after_draw;
		}
	}

stop:
	// The font holds GPU resources, so it goes before the device `app`
	// closes; the arena goes last of the three because the app struct is in
	// it.
	voe_text_font_destroy(font);
	voe_app_destroy(app);
	voe_base_arena_destroy(arena);
	printf("closed\n");
	return 0;
}
