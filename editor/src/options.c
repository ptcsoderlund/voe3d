// The argument list read into voe_editor_options. See the header for the one
// form, for why the size is parsed here rather than by the C library, and for
// why every mistake ends in the same usage line.
#include "options.h"

#include <base/assert.h>

#include <limits.h>
#include <stdio.h>
#include <string.h>

// Reads one run of ASCII '0'-'9' from `text`, writes the value through `out`
// and returns the pointer to the first character that is not a digit. NULL
// when there is no digit at all, and NULL when the value would pass INT_MAX —
// checked before the multiply, not after, so nothing overflows. No sign, no
// leading blank, no `errno`, no locale: --size's promised form is `<W>x<H>`
// and nothing relies on either.
static const char *number(const char *text, int *out)
{
	int value = 0;
	bool any = false;

	VOE_BASE_ASSERT(text != NULL, "reading a number out of nothing");
	VOE_BASE_ASSERT(out != NULL, "reading a number into nothing");

	while (*text >= '0' && *text <= '9') {
		int digit = *text - '0';

		if (value > (INT_MAX - digit) / 10)
			return NULL;
		value = value * 10 + digit;
		any = true;
		text++;
	}
	if (!any)
		return NULL;
	*out = value;
	return text;
}

// One line, on stderr, and the answer that goes with it. Every way of getting
// the command line wrong ends here.
static bool usage(void)
{
	fprintf(stderr,
		"usage: voe_editor [<folder>] [--capture <path> [--size <W>x<H>]]\n");
	return false;
}

bool voe_editor_options_read(int argc, char *argv[], voe_editor_options *out)
{
	bool sized = false;

	VOE_BASE_ASSERT(argv != NULL, "reading no argument list");
	VOE_BASE_ASSERT(out != NULL, "reading the command line into nothing");

	out->folder = NULL;
	out->capture = NULL;

	// `argv[a + 1]` is parsed in full before `a` moves, which the old code
	// did not need to do inside its `&&` chain.
	for (int a = 1; a < argc; a++) {
		if (strcmp(argv[a], "--capture") == 0 && a + 1 < argc) {
			out->capture = argv[++a];
		} else if (strcmp(argv[a], "--size") == 0 && a + 1 < argc) {
			int w, h;
			const char *rest = number(argv[a + 1], &w);

			if (rest != NULL && *rest == 'x')
				rest = number(rest + 1, &h);
			else
				rest = NULL;
			if (rest == NULL || *rest != '\0' || w <= 0 || h <= 0)
				return usage();
			out->wide = w;
			out->high = h;
			sized = true;
			a++;
		} else if (argv[a][0] != '-' && out->folder == NULL) {
			out->folder = argv[a];
		} else {
			return usage();
		}
	}
	// A size with nothing to size: the window's is the window system's
	// answer and not this program's to name (app.h), so the only picture
	// --size could mean is one nobody asked for.
	if (sized && out->capture == NULL)
		return usage();

	return true;
}
