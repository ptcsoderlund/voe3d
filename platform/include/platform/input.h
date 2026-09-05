// The keyboard and the mouse. Both belong to a window, both are drained by
// voe_platform_window_poll, and both are read as state rather than received as
// events. That is the whole of it — no gamepad, no text, no clipboard.
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
// is two bools at the call site and no queue in this folder. What genuinely
// needs an ordered history is an editor recording input or text being typed, and
// neither exists; the card that brings one is the card that revisits this, and it
// will be replacing this decision rather than adding a second API beside it.
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
// MOUSE BUTTONS ARE NOT HERE, AND THAT IS THIS CARD REPORTING RATHER THAN
// FORGETTING. Relative motion is the mouse's whole contribution to a camera and
// it is below; nothing in the engine clicks on anything yet, so a button would be
// surface with no caller. The card that puts something on screen to click is the
// card that adds them.
#pragma once

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
	VOE_PLATFORM_KEY_COUNT
} voe_platform_key;

// True while the key is held. False for a key the window never had focus for,
// and false for every key the moment focus is lost — see below.
bool voe_platform_input_key_down(voe_platform_window *window,
				 voe_platform_key key);

// How far the mouse moved, not where it is.
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
