// The fatal-check path. A VOE3D assert says "the program is already wrong", not
// "something went wrong": it is for a condition the code itself guarantees, and
// a failing one means the guarantee was broken somewhere upstream. It prints and
// aborts. There is no way to catch one and no return value to inspect.
//
// This is deliberately not error handling. A file that will not open and a model
// that will not parse are recoverable failures with a caller who can do
// something about them; how those get reported is not decided yet, and nothing
// here presumes an answer. Running out of memory is not one of them — that is
// fatal, and it goes through here.
//
// Two macros, and the names carry the difference:
//
//     VOE_BASE_ASSERT(expression, message)         always, every build
//     VOE_BASE_DEBUG_ASSERT(expression, message)   compiled out under NDEBUG
//
// The plain name is the one that survives, and that is the whole reasoning. It
// is the name a writer reaches for without thinking, so it has to be the one
// that still holds in the build a user runs. Naming it the other way round — the
// short name compiled out, a longer one kept — makes the safe choice the one you
// have to remember, and a check that quietly evaporates in release is worse than
// no check, because the code reads as if it were guarded.
//
// So VOE_BASE_DEBUG_ASSERT is the one that must argue for itself: reach for it
// when the check costs more than the thing it guards — walking a structure,
// re-deriving a result — or when it is so hot that the branch shows up. It still
// compiles under NDEBUG, so an expression that stops being valid is still a
// build error rather than a surprise the next time someone builds Debug.
//
// The message is a parameter and not optional. The expression says what was
// false; the message says why anyone cared. Whoever reads the abort reads these
// four lines and nothing else, so write the message for them.
#pragma once

// Prints the expression, the file, the line and the message to stderr, then
// aborts. Public because the macros expand to it; not meant to be called
// directly, and a direct call is an unconditional abort.
[[noreturn]] void voe_base_assert_fail(const char *expression, const char *file,
				       int line, const char *message);

#define VOE_BASE_ASSERT(expression, message)                                  \
	do {                                                                  \
		if (!(expression))                                            \
			voe_base_assert_fail(#expression, __FILE__, __LINE__, \
					     (message));                      \
	} while (0)

#ifdef NDEBUG
// The whole check sits in a branch that is never taken, so nothing in it runs,
// while both arguments stay compiled and type-checked exactly as in Debug.
// A branch and not sizeof: Clang does not count a static function named only
// inside sizeof as used, and -Wunneeded-internal-declaration then fails the
// Release build (052 bug 04, ADR-0340). Inside `if (0)` it does count.
#define VOE_BASE_DEBUG_ASSERT(expression, message)                            \
	do {                                                                  \
		if (0)                                                        \
			VOE_BASE_ASSERT(expression, message);                 \
	} while (0)
#else
#define VOE_BASE_DEBUG_ASSERT(expression, message)                            \
	VOE_BASE_ASSERT(expression, message)
#endif
