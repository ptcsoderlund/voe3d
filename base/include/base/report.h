// The one call a recoverable problem is reported through. Every folder that has
// something to tell a person about a failure writes it here, and nowhere else,
// so that there is one place the engine's problems come out of.
//
// A report is for a problem the world caused: a file that will not open, a
// device that will not create, a driver that refused. A person can act on it.
// It is the message base/error.h's categories deliberately do not carry. A
// problem the program caused is an assert; base/assert.h says where that line
// runs. Reporting returns, stops and recovers nothing; the failing function
// still returns its failure and its caller still decides.
//
//     VOE_BASE_WARNING(module, format, ...)   it went wrong and the engine carried on
//     VOE_BASE_ERROR(module, format, ...)     it failed
//
// FOUR THINGS A WRITER WILL OTHERWISE GET WRONG. The module is an argument,
// never text inside the format: VOE_BASE_ERROR("render", "vkCreateBuffer
// failed"), not a message that starts "render: ". It is the folder's name, so a
// reader can sort lines by where they came from without parsing the prose.
// The message has no trailing newline; the call writes one. The file and line
// are captured and not printed: a later destination, a panel that jumps to the
// site, has them without every call changing, and the person running the
// program can act on "the shader cache is unreadable", not on "device.c:412".
// A long message is cut, not allocated for: the whole line is at most 1024
// bytes, composed on the stack, and a longer one ends in "..." at that length.
// Reporting that something failed must not be able to fail.
//
// A REPORT NAMES ITS SUBJECTS (ADR-0445). A subject is what the message is
// about, the file and then the part, material or picture in it, so that code
// handed only bytes still names them. The code that knows pushes it, and code
// below never names the path again. A push and its pop pair within one
// function, like an arena's mark and rewind. The line is
// "<level>: <module>: <s1>: <s2>: <message>", subjects in the order pushed,
// all under the same 1024-byte cut.
//
// A PROGRAM CAN READ THE FIRST ERROR BACK (ADR-0160). This file keeps the text
// of the first VOE_BASE_ERROR since the last voe_base_report_error_clear(), per
// thread: thread_local, so a report on one thread never lands in another's
// notice and no lock sits on a path that must not fail. The first, because a
// failure travels upward and the site closest to the cause reports first. It is
// kept the way it is printed, without the level word, the module and the
// newline, so with its subjects before the message, under the same cut.
// Warnings are never kept. A reader clears before the operation whose failure
// it wants to explain and reads voe_base_report_error_first() once that has
// failed; nothing below a program reads it, and a caller that forgets to clear
// reads a stale error from earlier.
#pragma once

typedef enum {
	VOE_BASE_LEVEL_WARNING,
	VOE_BASE_LEVEL_ERROR,
} voe_base_report_level;

// How many subjects one thread can have pushed at once.
enum { VOE_BASE_REPORT_SUBJECTS = 4 };

// Composes "<level>: <module>: <subjects>: <message>" and writes it to stderr
// as one line. Public because the macros expand to it; not meant to be called
// directly.
[[gnu::format(printf, 5, 6)]]
void voe_base_report_at(voe_base_report_level level, const char *module,
			const char *file, int line, const char *format, ...);

#define VOE_BASE_WARNING(module, ...)                                      \
	voe_base_report_at(VOE_BASE_LEVEL_WARNING, (module), __FILE__,      \
			   __LINE__, __VA_ARGS__)
#define VOE_BASE_ERROR(module, ...)                                        \
	voe_base_report_at(VOE_BASE_LEVEL_ERROR, (module), __FILE__,        \
			   __LINE__, __VA_ARGS__)

// Names `subject` in every report on the calling thread until the matching
// voe_base_report_subject_pop(). The string is kept by pointer, not copied, so
// it must stay valid until the pop. A NULL or empty subject, or one more than
// VOE_BASE_REPORT_SUBJECTS, is the caller's bug and asserts.
void voe_base_report_subject_push(const char *subject);

// Drops the subject pushed last on the calling thread. Popping with none
// pushed asserts.
void voe_base_report_subject_pop(void);

// Forgets the kept error, on the calling thread. Call before an operation
// whose failure you want voe_base_report_error_first() to explain.
void voe_base_report_error_clear(void);

// The kept text of the first VOE_BASE_ERROR reported on the calling thread
// since its last voe_base_report_error_clear(), or NULL when none has been.
// Valid until the next clear or the next kept error.
const char *voe_base_report_error_first(void);
