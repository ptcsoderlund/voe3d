// The one function behind both assert macros. See include/base/assert.h for
// which macro to use and why.
#include <base/assert.h>

#include <stdio.h>
#include <stdlib.h>

// The layout matches testing/test.h's failure message on purpose: an abort and a
// failed check are read by the same person under the same pressure.
void voe_base_assert_fail(const char *expression, const char *file, int line,
			  const char *message)
{
	fprintf(stderr, "ASSERT  %s:%d\n        %s\n        %s\n",
		file, line, expression, message);
	fflush(stderr);
	abort();
}
