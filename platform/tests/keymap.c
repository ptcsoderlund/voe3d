// What src/keymap.h promises, checked from outside with a keymap text small
// enough to write by hand: <AC01> a/A, <AC10> odiaeresis/Odiaeresis with a
// type statement beside its symbols[Group1], <AE01> 1/exclam, <SPCE> space
// with one level (so Shift types the same thing), <AD01> a dead key (types
// nothing at either level), <MENU> aliased to <COMP> and given its own
// symbols, and a key holding U20AC. Also that text with no xkb_symbols block
// is refused.
//
// A second keymap text, copied from the lines the compositor this feature
// was measured against actually writes (plan.md), checks the four shapes
// that text uses and the name-spelled keymap above never did: keysyms as
// `0x` values rather than names, a Group index written `1` rather than
// `Group1`, a `type=` statement with no index beside it, and AltGr present
// only as the keysym value 0xfe03 on two keys. <AE12> is deliberately left
// out of that text's xkb_keycodes block — its own symbol list holds two
// values that would otherwise be in range (plusminus, notsign) — so that a
// key with no resolved code still exercises parse_key_body's sentinel path
// instead of writing into some other code by accident; that path is what
// makes it type nothing here, not the values themselves. A third, tiny text
// whose only key's only keysym is out of every range this reader turns into
// a code point checks that a keymap resolving to nothing is refused the same
// as one with no xkb_symbols block at all.
//
// Needs no window and no display.
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

// The lines between the keycodes and symbols blocks below are copied
// verbatim from plan.md, which took them from a real compositor's own
// `pc_se_inet(evdev)` keymap text. Everything around them — the keycodes
// this text needs to resolve most of those keys to an evdev code, and the
// wrapping blocks — is ordinary boilerplate added to make a complete text
// out of the fragment.
static const char numeric_keymap_text[] =
	"xkb_keymap {\n"
	"\n"
	"xkb_keycodes \"evdev\" {\n"
	"\tminimum = 8;\n"
	"\tmaximum = 255;\n"
	"\t<AE01> = 10;\n"
	"\t<AE02> = 11;\n"
	"\t<AE04> = 13;\n"
	"\t<AE05> = 14;\n"
	"\t<AE07> = 16;\n"
	"\t<AE08> = 17;\n"
	"\t<AE09> = 18;\n"
	"\t<AE10> = 19;\n"
	"\t<AE11> = 20;\n"
	"\t<AC01> = 38;\n"
	"\t<AC10> = 47;\n"
	"\t<AC11> = 48;\n"
	"\t<AD11> = 34;\n"
	"\t<BKSP> = 22;\n"
	"\t<LVL3> = 92;\n"
	"\t<RALT> = 108;\n"
	"\t<UNI1> = 208;\n"
	"\talias <ALGR> = <RALT>;\n"
	"};\n"
	"\n"
	"xkb_types \"complete\" { };\n"
	"\n"
	"xkb_compat \"complete\" { };\n"
	"\n"
	"xkb_symbols \"pc+se+inet(evdev)\" {\n"
	"\tkey <AE02> {\t[ 0x32, 0x22, 0x40, 0xb2 ] };\n"
	"\tkey <AE04> {\t[ 0x34, 0xa4, 0x24, 0xbc ] };\n"
	"\tkey <AE05> {\t[ 0x35, 0x25, 0x20ac, 0xad5 ] };\n"
	"\tkey <AE07> {\t[ 0x37, 0x2f, 0x7b, 0xf7 ] };\n"
	"\tkey <AE08> {\t[ 0x38, 0x28, 0x5b, 0xab ] };\n"
	"\tkey <AE09> {\t[ 0x39, 0x29, 0x5d, 0xbb ] };\n"
	"\tkey <AE10> {\t[ 0x30, 0x3d, 0x7d, 0xb0 ] };\n"
	"\tkey <AE11> {\t[ 0x2b, 0x3f, 0x5c, 0xbf ] };\n"
	"\tkey <AE12> {\t[ 0xfe51, 0xfe50, 0xb1, 0xac ] };\n"
	"\tkey <AC01> {\t[ 0x61, 0x41, 0xaa, 0xba ] };\n"
	"\tkey <AC10> {\t[ 0xf6, 0xd6, 0xf8, 0xd8 ] };\n"
	"\tkey <AC11> {\t[ 0xe4, 0xc4, 0xe6, 0xc6 ] };\n"
	"\tkey <AD11> {\t[ 0xe5, 0xc5, 0xfe57, 0xfe58 ] };\n"
	"\tkey <BKSP> {\t[ 0xff08, 0xff08 ] };\n"
	"\tkey <LVL3> {\t[ 0xfe03 ] };\n"
	"\tkey <RALT> {\n"
	"\t\ttype= \"ONE_LEVEL\",\n"
	"\t\tsymbols[1]= [ 0xfe03 ]\n"
	"\t};\n"
	"\tmodifier_map Mod5 { <LVL3> };\n"
	"\tkey <UNI1> {\t[ 0x01000041 ] };\n"
	"};\n"
	"\n"
	"};\n";

// A keymap with an xkb_symbols block whose only key's only keysym (backspace,
// 0xff08) is out of every range codepoint_from_value knows: refused exactly
// as text with no xkb_symbols block at all is.
static const char nothing_typed_text[] =
	"xkb_keymap {\n"
	"xkb_keycodes \"evdev\" {\n"
	"\t<BKSP> = 22;\n"
	"};\n"
	"xkb_symbols \"evdev\" {\n"
	"\tkey <BKSP> {\t[ 0xff08, 0xff08 ] };\n"
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

	VOE_TEST_CHECK(voe_platform_keymap_read(
		numeric_keymap_text, sizeof(numeric_keymap_text) - 1, &map));

	// <AC01> = 38, evdev 30: a A ordfeminine masculine, all four levels
	// given as 0x values.
	VOE_TEST_CHECK_INT(map.typed[30][0], 'a');
	VOE_TEST_CHECK_INT(map.typed[30][1], 'A');
	VOE_TEST_CHECK_INT(map.typed[30][2], 0xaa);
	VOE_TEST_CHECK_INT(map.typed[30][3], 0xba);

	// <AD11> = 34, evdev 26: aring / Aring.
	VOE_TEST_CHECK_INT(map.typed[26][0], 0xe5);
	VOE_TEST_CHECK_INT(map.typed[26][1], 0xc5);

	// <AC10> = 47, evdev 39: odiaeresis / Odiaeresis.
	VOE_TEST_CHECK_INT(map.typed[39][0], 0xf6);
	VOE_TEST_CHECK_INT(map.typed[39][1], 0xd6);

	// <AC11> = 48, evdev 40: adiaeresis / Adiaeresis.
	VOE_TEST_CHECK_INT(map.typed[40][0], 0xe4);
	VOE_TEST_CHECK_INT(map.typed[40][1], 0xc4);

	// AltGr (level 2) on the digit row: @ $ { [ ] } \.
	VOE_TEST_CHECK_INT(map.typed[3][2], '@'); // <AE02> = 11, evdev 3
	VOE_TEST_CHECK_INT(map.typed[5][2], '$'); // <AE04> = 13, evdev 5
	VOE_TEST_CHECK_INT(map.typed[8][2], '{'); // <AE07> = 16, evdev 8
	VOE_TEST_CHECK_INT(map.typed[9][2], '['); // <AE08> = 17, evdev 9
	VOE_TEST_CHECK_INT(map.typed[10][2], ']'); // <AE09> = 18, evdev 10
	VOE_TEST_CHECK_INT(map.typed[11][2], '}'); // <AE10> = 19, evdev 11
	VOE_TEST_CHECK_INT(map.typed[12][2], '\\'); // <AE11> = 20, evdev 12

	// <AE05> = 14, evdev 6: level 2 is the legacy keysym 0x20ac (euro),
	// out of every range this reader turns into a code point.
	VOE_TEST_CHECK_INT(map.typed[6][2], 0);

	// <BKSP> = 22, evdev 14: 0xff08 is out of every range at both its
	// levels, so the key types nothing despite having a resolved code.
	VOE_TEST_CHECK_INT(map.typed[14][0], 0);
	VOE_TEST_CHECK_INT(map.typed[14][1], 0);
	VOE_TEST_CHECK_INT(map.typed[14][2], 0);
	VOE_TEST_CHECK_INT(map.typed[14][3], 0);

	// <LVL3> = 92, evdev 84, and <RALT> = 108, evdev 100: both hold only
	// the keysym value 0xfe03 (ISO_Level3_Shift), so both are flagged as
	// AltGr and both type nothing at any level.
	VOE_TEST_CHECK(map.level3_shift[84]);
	VOE_TEST_CHECK(map.level3_shift[100]);
	VOE_TEST_CHECK_INT(map.typed[84][0], 0);
	VOE_TEST_CHECK_INT(map.typed[100][0], 0);

	// <UNI1> = 208, evdev 200: 0x01000041 is the 0x0100xxxx form of a
	// Unicode code point, U+0041, and its one entry types shifted too.
	VOE_TEST_CHECK_INT(map.typed[200][0], 'A');
	VOE_TEST_CHECK_INT(map.typed[200][1], 'A');

	VOE_TEST_CHECK(!voe_platform_keymap_read(
		nothing_typed_text, sizeof(nothing_typed_text) - 1, &map));
	VOE_TEST_CHECK_INT(map.typed[14][0], 0);
	VOE_TEST_CHECK(!map.level3_shift[14]);

	return voe_test_result();
}
