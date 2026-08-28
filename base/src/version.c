// Placeholder proving the folder builds and that C23 is in force. Delete when
// the first real file lands.
#include <base/version.h>
#include <assert.h>
static_assert(__STDC_VERSION__ >= 202311L, "C23 required");
int voe_base_version(void) { return 1; }
