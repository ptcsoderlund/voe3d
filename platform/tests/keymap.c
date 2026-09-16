// What src/keymap.h promises, checked from outside with a keymap text small
// enough to write by hand: <AC01> a/A, <AC10> odiaeresis/Odiaeresis with a
// type statement beside its symbols[Group1], <AE01> 1/exclam, <SPCE> space
// with one level (so Shift types the same thing), <AD01> a dead key (types
// nothing at either level), <MENU> aliased to <COMP> and given its own
// symbols, and a key holding U20AC. Also that text with no xkb_symbols block
// is refused. Needs no window and no display.
#include "../src/keymap.h"

#include <testing/test.h>

static const char keymap_text[] =
	"xkb_keymap {\n"
	"\n"
	"xkb_keycodes \"test\" {\n"
	"\tminimum = 8;\n"
	"\tmaximum = 255;\n"
	"\t<AC01> = 38;\n"
	"\t<AC10> = 47;\n"
	"\t<AE01> = 10;\n"
	"\t<SPCE> = 65;\n"
	"\t<AD01> = 24;\n"
	"\t<TLDE> = 49;\n"
	"\t<COMP> = 135;\n"
	"\talias <MENU> = <COMP>;\n"
	"\tindicator 1 = \"Caps Lock\";\n"
	"};\n"
	"\n"
	"xkb_types \"test\" { };\n"
	"\n"
	"xkb_compat \"test\" { };\n"
	"\n"
	"xkb_symbols \"test\" {\n"
	"\tname[Group1]=\"test\";\n"
	"\n"
	"\tkey <AC01> {\t[ a, A ]\t};\n"
	"\tkey <AC10> {\ttype = \"FOUR_LEVEL\", symbols[Group1] = [ odiaeresis, Odiaeresis ]\t};\n"
	"\tkey <AE01> {\t[ 1, exclam ]\t};\n"
	"\tkey <SPCE> {\t[ space ]\t};\n"
	"\tkey <AD01> {\t[ dead_acute ]\t};\n"
	"\tkey <TLDE> {\t[ U20AC ]\t};\n"
	"\tkey <MENU> {\t[ m, M ]\t};\n"
	"\tmodifier_map Shift { <LFSH> };\n"
	"};\n"
	"\n"
	"xkb_geometry \"test\" { };\n"
	"};\n";

static const char no_symbols_text[] =
	"xkb_keymap {\n"
	"xkb_keycodes \"test\" {\n"
	"\t<AC01> = 38;\n"
	"};\n"
	"};\n";

int main(void)
{
	voe_platform_keymap map;

	VOE_TEST_CHECK(voe_platform_keymap_read(keymap_text,
						sizeof(keymap_text) - 1, &map));

	// <AC01> = 38, evdev 30: a / A.
	VOE_TEST_CHECK_INT(map.typed[30][0], 'a');
	VOE_TEST_CHECK_INT(map.typed[30][1], 'A');

	// <AC10> = 47, evdev 39: odiaeresis / Odiaeresis, with a type statement
	// beside its symbols[Group1].
	VOE_TEST_CHECK_INT(map.typed[39][0], 0xf6);
	VOE_TEST_CHECK_INT(map.typed[39][1], 0xd6);

	// <AE01> = 10, evdev 2: 1 / exclam.
	VOE_TEST_CHECK_INT(map.typed[2][0], '1');
	VOE_TEST_CHECK_INT(map.typed[2][1], '!');

	// <SPCE> = 65, evdev 57: one level, shifted the same.
	VOE_TEST_CHECK_INT(map.typed[57][0], ' ');
	VOE_TEST_CHECK_INT(map.typed[57][1], ' ');

	// <AD01> = 24, evdev 16: a dead key types nothing at either level.
	VOE_TEST_CHECK_INT(map.typed[16][0], 0);
	VOE_TEST_CHECK_INT(map.typed[16][1], 0);

	// <TLDE> = 49, evdev 41: U20AC, one level, shifted the same.
	VOE_TEST_CHECK_INT(map.typed[41][0], 0x20ac);
	VOE_TEST_CHECK_INT(map.typed[41][1], 0x20ac);

	// <MENU> is an alias of <COMP> = 135, evdev 127, and is given its own
	// symbols: the alias resolves to the same code as its target.
	VOE_TEST_CHECK_INT(map.typed[127][0], 'm');
	VOE_TEST_CHECK_INT(map.typed[127][1], 'M');

	VOE_TEST_CHECK(!voe_platform_keymap_read(
		no_symbols_text, sizeof(no_symbols_text) - 1, &map));
	VOE_TEST_CHECK_INT(map.typed[30][0], 0);
	VOE_TEST_CHECK_INT(map.typed[30][1], 0);

	return voe_test_result();
}
