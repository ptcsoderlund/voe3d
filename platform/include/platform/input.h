// The keyboard and the mouse. Both belong to a window, both are drained by
// voe_platform_window_poll, and both are read as state rather than received as
// events. Typed text is the one exception with any order in it — see
// voe_platform_input_text below — and it is drained the same way the wheel is,
// not handed out as a queue of events. No gamepad, no clipboard.
//
//     while (!voe_platform_window_should_close(window)) {
//             voe_platform_window_poll(window);
//
//             if (voe_platform_input_key_down(window, VOE_PLATFORM_KEY_W))
//                     walk(forward);
//
//             voe_platform_motion look = voe_platform_input_motion(window);
//             turn(look.x, look.y);
//     }
//
// IT IS POLLED STATE AND NOT A QUEUE OF EVENTS, AND THE CARD ASKED FOR THAT TO
// BE DECIDED RATHER THAN DISCOVERED. A camera wants "is this key held", which is
// a question about now; a queue answers "what happened, in order", which is a
// question about the past. Only one of those has a caller, guidelines.md puts
// data ahead of events, and rule 10 says a thing is written when something calls
// it — so this folder folds every event the OS hands it into the state below and
// keeps none of them.
//
// WHAT THAT COSTS IS EDGES, AND IT IS WORTH KNOWING BEFORE IT SURPRISES SOMEONE.
// A key pressed and released between two polls is a key that never happened
// here. At sixty frames a second a person cannot do it, and a caller that wants
// "was it pressed this frame" compares against what it read last frame — which
// is two bools at the call site and no queue in this folder. Text being typed is
// the one place within a frame where order matters, and voe_platform_input_text
// answers it without a queue: a buffer appended to in the order typed and
// drained whole by the next poll, the same shape the wheel already has. An
// editor recording input across many frames still has no answer here; the card
// that brings one is the card that revisits this.
//
// A KEY IS A PLACE ON THE KEYBOARD, NOT A LETTER. VOE_PLATFORM_KEY_W is the key
// where W is on a US layout and it stays that key on every other layout, because
// what a movement key means is where the finger goes and not which character
// would be typed. Nothing here consults a keymap: Wayland hands over evdev
// scancodes and Windows hands over virtual keys, and both are turned into the
// list below by position. The consequence is deliberate and it is the same one
// every engine has — on AZERTY the movement keys are where Z, Q, S and D are —
// and the fix is remappable bindings at the call site, not a keymap in here.
//
// THE LIST IS SHORT BECAUSE OF RULE 10 AND NOT BECAUSE IT WAS HARD. Every key
// below has a caller today. There is no rest of the alphabet, no F-keys and no
// arrows, because nothing reads them; adding one is a line in the enum and a line
// in each backend's table, and it happens when something wants it. P is the most
// recent to arrive that way and is not a movement key: dev/src/main.c toggles the
// present mode with it.
//
// THE MOUSE IS TWO DIFFERENT QUESTIONS AND BOTH ARE ANSWERED BELOW. A camera
// asks how far the mouse moved, which is voe_platform_input_motion and keeps
// arriving while the pointer is locked; a GUI asks where the pointer is and
// which buttons are down, which is voe_platform_input_pointer and
// voe_platform_input_button_down. They are not two views of one number: a locked
// pointer moves the camera without ever changing position, and a pointer walked
// across the window changes position by exactly what the motion says only when
// nothing is accelerated or clipped. Reach for motion to turn something and for
// the pointer to point at something, and never derive one from the other.
//
// BUTTONS ARE LEVEL, NOT EDGE, THE SAME AS KEYS. voe_platform_input_button_down
// says whether a button is held now, and a caller that wants "went down this
// frame" or "was released over the thing it was pressed on" compares against what
// it read last frame — two bools at the call site, exactly as the Tab toggle in
// dev/src/main.c already does for a key. It is the caller's because the caller
// is the one that knows what a click is: a GUI's click is a release over the
// same widget the press landed on, which this folder cannot know about, and a
// second, edge-shaped API beside the level one would be the second API the
// paragraph above says this folder must not grow.
//
// THE WHEEL IS COUNTED IN NOTCHES, AND WHAT A NOTCH MOVES IS THE PROGRAM'S
// (ADR-0153 point 10). voe_platform_input_wheel is how far the wheel turned, on
// both axes, as a count of detents: one click of an ordinary wheel is 1, and a
// device that reports fractions — a touchpad, a free-spinning wheel — reads as
// fractions. +x shows content further right and +y further down, which is the
// wheel turned towards the person; the two window systems report opposite signs
// and each backend turns its own into this one. It drains exactly as motion
// does: summed until the next poll, zeroed at the top of it. How many lines,
// pixels or millimetres a notch scrolls is decided by whoever reads it, and
// nothing here smooths or accelerates it. Windows counts notches exactly;
// Wayland at the version this folder binds sends a length per notch that
// differs between compositors — src/window_wayland.c says which, and what fixes
// it.
#pragma once

#include <stdint.h>

typedef struct voe_platform_window voe_platform_window;

// Every key this engine reads, and the length of the state each backend keeps.
// VOE_PLATFORM_KEY_COUNT is the count and never a key; asking for it asserts.
//
// SHIFT AND CONTROL ARE ONE KEY EACH AND NOT TWO. Both ends of the keyboard fold
// into the same entry, because everything that reads them means "the modifier is
// held" and nothing has ever meant "the left one specifically". A caller that
// needs to tell them apart is a caller that splits these two lines.
typedef enum {
	VOE_PLATFORM_KEY_W,
	VOE_PLATFORM_KEY_A,
	VOE_PLATFORM_KEY_S,
	VOE_PLATFORM_KEY_D,
	VOE_PLATFORM_KEY_Q,
	VOE_PLATFORM_KEY_E,
	VOE_PLATFORM_KEY_SPACE,
	VOE_PLATFORM_KEY_CONTROL,
	VOE_PLATFORM_KEY_SHIFT,
	VOE_PLATFORM_KEY_TAB,
	VOE_PLATFORM_KEY_ESCAPE,
	VOE_PLATFORM_KEY_P,
	VOE_PLATFORM_KEY_N,
	VOE_PLATFORM_KEY_O,
	VOE_PLATFORM_KEY_BACKSPACE,
	VOE_PLATFORM_KEY_ENTER,
	VOE_PLATFORM_KEY_COUNT
} voe_platform_key;

// True while the key is held. False for a key the window never had focus for,
// and false for every key the moment focus is lost — see below.
bool voe_platform_input_key_down(voe_platform_window *window,
				 voe_platform_key key);

// The UTF-8 bytes typed since the previous voe_platform_window_poll, in the
// order typed, from a fixed buffer the poll empties — the same shape
// voe_platform_input_motion and voe_platform_input_wheel already have, and for
// the same reason: reading it twice in one frame gives the same answer twice,
// and not reading it for a frame throws that frame's typing away.
//
// bytes IS VALID UNTIL THE NEXT POLL AND NOT A MOMENT LONGER. It points into
// the window's own buffer, is not NUL-terminated by size, and size is the
// count of bytes, not code points — a caller wanting characters decodes UTF-8
// itself.
//
// A KEY IS STILL A PLACE, AND THIS IS THE ONE EXCEPTION. Everything above this
// point on this page answers "what moved", and text answers "what would this
// have typed" — which needs the keymap ADR-0161 has this folder read, Wayland's
// own and in-house rather than xkbcommon. See VOE_PLATFORM_KEY_N and its
// neighbours below for the places these bytes come from.
//
// NOTHING IS TYPED WHILE CONTROL IS HELD, so a shortcut built on a key below
// does not also type a character. Losing keyboard focus empties this buffer
// the same way it releases every key. What arrives past the buffer's capacity
// in one frame is dropped whole, never split across two polls.
//
// A CODE POINT THIS FOLDER CANNOT FORM IS DROPPED, SILENTLY. Control
// characters, a lone UTF-16 surrogate, anything past U+10FFFF and anything
// that would not fit whole in what is left of the buffer type nothing rather
// than a partial or replacement character.
typedef struct {
	const char *bytes;
	uint32_t size;
} voe_platform_text;

voe_platform_text voe_platform_input_text(voe_platform_window *window);

// How far the mouse moved, not where it is — for that, see
// voe_platform_input_pointer below.
//
// THE UNIT IS THE WINDOW SYSTEM'S AND THE TWO PLATFORMS DO NOT AGREE ON IT.
// Wayland reports surface-local units with the compositor's pointer acceleration
// already applied; Windows reports raw device counts with none. Both are
// "roughly a pixel" on ordinary hardware and neither is promised to be, so a
// caller turns this into an angle with a sensitivity of its own — which it would
// want anyway — and does not read a number out of it. Pretending the two were
// the same unit is the only thing that would be worse than saying this.
//
// +x is to the right and +y is down, which is the direction both window systems
// report and not a convention this folder chose. A camera that pitches up on a
// mouse moved up negates y at the call site.
typedef struct {
	float x;
	float y;
} voe_platform_motion;

// The motion accumulated since the previous voe_platform_window_poll, zeroed by
// each poll. Reading it twice in one frame gives the same answer twice; not
// reading it for a frame throws that frame's motion away, which is what a caller
// that is not looking around wants.
voe_platform_motion voe_platform_input_motion(voe_platform_window *window);

// Wheel turn accumulated since the previous voe_platform_window_poll, in
// notches. +x shows content further right, +y further down — the wheel turned
// towards the person. Fractional where the device reports fractions.
typedef struct {
	float x;
	float y;
} voe_platform_wheel;

voe_platform_wheel voe_platform_input_wheel(voe_platform_window *window);

// Where the pointer is, not how far it moved — for that, see
// voe_platform_input_motion above.
//
// x AND y ARE THE WINDOW'S OWN PIXELS, ORIGIN TOP-LEFT, +x RIGHT AND +y DOWN,
// the same space and the same unit voe_platform_size measures the client area
// in. A pointer at the bottom-right corner reads one less than the size in both
// axes. Windows reports whole pixels; Wayland reports fractions of one, and the
// fraction is kept rather than rounded here, because rounding is a decision the
// caller can make and cannot undo.
//
// over IS WHETHER THERE IS A POINTER TO POINT WITH, AND WHEN IT IS FALSE x AND y
// ARE WHERE IT WAS LAST SEEN. A GUI has to tell "the pointer is at the edge" from
// "the pointer is gone", because the first is a hover and the second ends one —
// so the last position is kept, and over says which of the two this is. Three
// things make it false, and each is a state in which a live-looking number would
// be a lie:
//
//   - THE POINTER LEFT THE WINDOW. Nothing is hovered. It comes back true on the
//     next movement inside.
//   - THE POINTER IS LOCKED. Mouse look has it, the window system has frozen or
//     hidden it, and the number underneath stopped meaning anything the moment
//     the lock took. The camera is reading motion; there is nothing here for a
//     GUI to click on. True again the moment the lock is released: the
//     position underneath is where the window system left the cursor — frozen
//     on Wayland, clipped but tracked on Windows — and it is right again as
//     soon as there is nothing hiding it.
//   - THERE IS NO POINTER. A seat with no mouse on it, or one not yet reported.
//
// Losing keyboard focus is not one of them: focus is the keyboard's, and a
// pointer resting over an unfocused window is still over it and still reports
// where. Both window systems agree on that and so does this.
//
// WHILE A BUTTON IS HELD THE POINTER STAYS OVER THE WINDOW EVEN WHEN IT IS NOT,
// AND x AND y GO OUTSIDE THE CLIENT AREA — NEGATIVE, OR PAST THE SIZE. That is a
// drag, and it is what both window systems do on their own: Wayland keeps the
// pointer on the surface that took the press until the release, and this
// folder asks Windows for the same with a capture. A slider dragged past the
// window's edge keeps following the mouse, and a caller clamps if it wants to.
typedef struct {
	float x;
	float y;
	bool over;
} voe_platform_pointer;

voe_platform_pointer voe_platform_input_pointer(voe_platform_window *window);

// The three buttons something in the engine reads. Not more — rule 10, and the
// key enum's own precedent: a fourth arrives when a card wants one, as a line
// here and a line in each backend. VOE_PLATFORM_BUTTON_COUNT is the count and
// never a button; asking for it asserts.
typedef enum {
	VOE_PLATFORM_BUTTON_LEFT,
	VOE_PLATFORM_BUTTON_RIGHT,
	VOE_PLATFORM_BUTTON_MIDDLE,
	VOE_PLATFORM_BUTTON_COUNT
} voe_platform_button;

// True while the button is held. It goes up, without a release ever arriving,
// when the pointer leaves the window with the button still down — a compositor
// that broke a drag, a capture Windows took away — because the release is then
// delivered to whoever has the pointer now, and up is the safe answer for the
// same reason every key goes up when focus is lost.
bool voe_platform_input_button_down(voe_platform_window *window,
				    voe_platform_button button);

// Ask for the pointer to stop going anywhere, so that a mouse can be moved
// without end. This is what mouse look needs and it is not the same thing as
// hiding a cursor.
//
// IT IS A REQUEST AND THE ANSWER IS _pointer_locked. On Wayland the compositor
// decides, may say no, may take the lock away when the window loses focus and
// may give it back — so nothing here remembers what was asked for and reports it
// as though it happened. Ask for it, then ask what the state actually is, every
// frame, the same way voe_platform_window_decorated is asked.
//
// A LOCK THAT IS REFUSED IS NOT AN ERROR AND MUST NOT STOP ANYTHING. Mouse
// motion arrives whenever the pointer is over the window whether or not the lock
// took, so a camera whose lock was refused still looks around — the cursor will
// simply walk out of the window while it does. That is a worse experience and it
// is not a failure, which is why this returns nothing.
//
// WHAT HAPPENS TO THE ARROW DIFFERS BETWEEN THE TWO PLATFORMS, AND THAT IS
// DELIBERATE RATHER THAN UNFINISHED. Windows hides the cursor and confines it to
// the client area, because ShowCursor un-hides again and nothing has to be drawn.
// Wayland freezes it where it stands and leaves it visible, because hiding it
// there means handing the compositor a cursor image and this engine has none to
// hand over — and a cursor hidden with no way back is worse than a cursor
// sitting still. The card that owns a cursor image is the card that makes these
// two agree.
void voe_platform_input_lock_pointer(voe_platform_window *window, bool lock);

// Whether the pointer is locked right now, which is the window system's answer
// and not the last thing asked for.
bool voe_platform_input_pointer_locked(voe_platform_window *window);
