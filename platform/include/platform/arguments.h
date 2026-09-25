// The program's own command-line arguments, as UTF-8 on both platforms
// (ADR-0247). main hands its argc and argv to this once and uses what comes
// back instead:
//
//     int main(int argc, char *argv[])
//     {
//             voe_platform_arguments arguments =
//                     voe_platform_arguments_read(argc, argv, arena);
//             // arguments.values[1] ... arguments.values[arguments.count - 1]
//     }
//
// values[count] is NULL, as argv's own last entry is, and values[0] is the
// program as it was started.
//
// LINUX HANDS BACK argv ITSELF. Its bytes are what the shell passed, which is
// UTF-8 on every system this engine supports, so nothing is copied and the
// arena is not used.
//
// WINDOWS IGNORES argv AND READS THE PROCESS'S WIDE COMMAND LINE (ADR-0248).
// main's argv there is in the legacy code page, so a project folder named
// outside it would arrive broken. The command line is split by the system's
// own CommandLineToArgvW and each entry made UTF-8 into the arena, which must
// outlive the values. If the split fails, argv is handed back as it is.
//
// RULE 6 DEVIATION: char *argv[] is main's own signature, taken as main
// declares it rather than rewritten into something the caller must cast to.
#pragma once

#include <base/arena.h>

typedef struct {
	int count;
	const char *const *values;
} voe_platform_arguments;

voe_platform_arguments voe_platform_arguments_read(int argc, char *argv[],
						   voe_base_arena *arena);
