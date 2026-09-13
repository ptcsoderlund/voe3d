// The one call a recoverable problem is reported through. Every folder that has
// something to tell a person about a failure writes it here, and nowhere else,
// so that there is one place the engine's problems come out of.
//
// A report is for a problem the world caused: a file that will not open, a
// device that will not create, a driver that refused. The caller could not have
// prevented it and a person can act on it. It is the message written at the site
// that base/error.h's categories deliberately do not carry. A problem the
// program caused is not a report — it is an assert, and base/assert.h says where
// that line runs. Reporting does not return, stop or recover anything; the
// failing function still returns its failure and its caller still decides.
//
// Two macros, and the level is in the name:
//
//     VOE_BASE_WARNING(module, format, ...)   it went wrong and the engine carried on
//     VOE_BASE_ERROR(module, format, ...)     it failed
//
// FOUR THINGS A WRITER WILL OTHERWISE GET WRONG.
//
// The module is an argument and never text inside the format. Write
// VOE_BASE_ERROR("render", "vkCreateBuffer failed"), not a message that starts
// "render: ". The module is the folder's name, so that a later reader can sort
// the lines by where they came from without parsing the prose.
//
// The message has no trailing newline. The call writes one; a message that
// brings its own prints a blank line after it.
//
// The file and line are captured and not printed. They are passed so that a
// later destination — a panel that jumps to the site — has them without every
// call site changing; they are not printed because the line is written for the
// person running the program, who can act on "the shader cache is unreadable"
// and cannot act on "device.c:412".
//
// A long message is truncated, not allocated for. The whole line — level, module,
// message and newline — is at most 1024 bytes, composed on the stack, and a line
// that would be longer ends in "..." at that length. Reporting that something
// failed must not be able to fail, and allocating is a way to fail.
#pragma once

typedef enum {
	VOE_BASE_LEVEL_WARNING,
	VOE_BASE_LEVEL_ERROR,
} voe_base_report_level;

// Composes "<level>: <module>: <message>" and writes it to stderr as one line.
// Public because the macros expand to it; not meant to be called directly.
[[gnu::format(printf, 5, 6)]]
void voe_base_report_at(voe_base_report_level level, const char *module,
			const char *file, int line, const char *format, ...);

#define VOE_BASE_WARNING(module, ...)                                      \
	voe_base_report_at(VOE_BASE_LEVEL_WARNING, (module), __FILE__,      \
			   __LINE__, __VA_ARGS__)
#define VOE_BASE_ERROR(module, ...)                                        \
	voe_base_report_at(VOE_BASE_LEVEL_ERROR, (module), __FILE__,        \
			   __LINE__, __VA_ARGS__)
