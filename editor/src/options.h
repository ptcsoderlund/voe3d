// The command line, read once before anything is opened, so a mistyped
// argument costs nothing and says so straight away. `voe_editor [<folder>]
// [--capture <path> [--size <W>x<H>]]` is the whole of it, and one usage line
// on stderr is what every way of getting it wrong answers with: there is one
// form to state, and stating it twice in different words would be two forms to
// keep in step.
//
// ONE ARGUMENT NOT STARTING WITH `--` IS THE FOLDER; A SECOND ONE IS USAGE.
// There is only ever one project to open, so a second bare argument is not
// something this program can mean anything by. What a folder, or none, opens is
// main.c's and project.h's.
//
// `--capture <path>` WRITES ONE PICTURE AND LEAVES, and `--size <W>x<H>` says
// how big that picture is. The size defaults to the one the caller put in
// `*out` — the size the window would have opened at — and means nothing without
// `--capture`, so it is refused there rather than quietly ignored. A missing
// value and any unknown argument are the same refusal.
//
// THE SIZE IS PARSED HERE AND NOT BY THE C LIBRARY (ADR-0159: Windows' runtime
// deprecates `sscanf` under -Werror and rule 8 forbids silencing that
// tree-wide). Two runs of digits with an `x` between and nothing after, so
// "1280x720nonsense" is still a mistake and not a 1280x720. The parser is
// stricter than `%d` about a leading blank or a leading '+' — neither was ever
// part of the promised form `<W>x<H>` and nothing relied on them.
//
// `char *argv[]` IS main's OWN SIGNATURE AND THE ONE DEVIATION FROM RULE 6 IN
// THIS FOLDER (DEVIATION: rule 6, an array of pointers is spelled as the array
// it is, because the C runtime calls main with this signature and nothing in
// this program chose it). It is handed straight to the one call below, which is
// where the argument list is read and the only place it is.
#pragma once

#include <stdbool.h>

// What the command line said.
typedef struct {
	const char *folder;  // the one bare argument, or NULL
	const char *capture; // --capture's path, or NULL
	int wide;            // --size's, or what the caller put here
	int high;
} voe_editor_options;

// Reads the arguments into `*out`, which arrives holding the size to use when
// `--size` is absent and is left holding it then. False means the one usage
// line has been printed on stderr and the caller returns 2.
[[nodiscard]] bool voe_editor_options_read(int argc, char *argv[],
					   voe_editor_options *out);
