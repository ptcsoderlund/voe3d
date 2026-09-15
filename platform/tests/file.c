// What platform/file.h promises, checked from outside: the bytes that went in
// come back byte for byte, a file is replaced rather than overwritten in place,
// and a path that cannot be opened fails without creating anything. Needs no
// window and no display, so it runs under ctest on a machine with neither.
//
// THE ORACLE IS THE C LIBRARY AND NOT THIS FOLDER. Reading back with fopen and
// fread is deliberate: a reader of our own would let a matching pair of bugs
// pass, and platform has no reader anyway (rule 10). stdio in a test is not
// stdio in src/.
//
// THE REPLACEMENT CASE IS THE ONE WORTH HAVING. A writer that opens without
// truncating leaves the tail of a longer previous file behind, and the contents
// still start with the right bytes — so only the length catches it, and only
// when the second write is the shorter of the two.
//
// Every file is written into the working directory, which ctest sets to the
// build tree, and removed at the end whether the checks passed or not. The
// failing case prints a reported line to stderr on purpose; that is the call
// saying why, not the test going wrong.
#include <platform/file.h>

#include <testing/test.h>

#include <stdio.h>

#define WRITTEN "voe_platform_file_test.bin"
#define MISSING "voe_platform_file_test_no_such_directory/file.bin"

// Reads the whole file into buffer and hands back its length, or -1 if it is not
// there. room is the size of buffer; a file that does not fit reads as longer
// than it is, which fails the length check rather than overrunning.
static long read_back(const char *path, unsigned char *buffer, size_t room)
{
	// The MSVC C runtime deprecates fopen (it names fopen_s, Annex K, as the
	// replacement; Linux has none) and -Werror turns that into a build error.
	// This is tests/, never src/ — ADR-0159 — so the call is suppressed here
	// rather than the oracle changed.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	FILE *file = fopen(path, "rb");
#pragma clang diagnostic pop
	size_t count;

	if (file == NULL)
		return -1;

	count = fread(buffer, 1, room, file);
	fclose(file);
	return (long)count;
}

int main(void)
{
	// Deliberately includes a zero byte and a 0xff, so a writer that stopped
	// at a terminator or sign-extended a byte shows up here.
	static const uint8_t payload[] = { 0x00, 0x01, 0x7f, 0x80, 0xff,
					   0x41, 0x00, 0xfe };
	static const uint8_t shorter[] = { 0xab, 0xcd };
	static const uint8_t single[] = { 0x5a };
	unsigned char got[64];
	voe_base_error error = VOE_BASE_OK;
	long length;

	// The bytes that went in come back.
	VOE_TEST_CHECK(voe_platform_file_write(WRITTEN, payload,
					       sizeof payload, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_OK);
	length = read_back(WRITTEN, got, sizeof got);
	VOE_TEST_CHECK_INT(length, (long)sizeof payload);
	if (length == (long)sizeof payload) {
		for (size_t i = 0; i < sizeof payload; i++)
			VOE_TEST_CHECK_INT(got[i], payload[i]);
	}

	// A shorter file over a longer one is the shorter one, tail and all.
	VOE_TEST_CHECK(voe_platform_file_write(WRITTEN, shorter,
					       sizeof shorter, &error));
	length = read_back(WRITTEN, got, sizeof got);
	VOE_TEST_CHECK_INT(length, (long)sizeof shorter);
	if (length == (long)sizeof shorter) {
		VOE_TEST_CHECK_INT(got[0], shorter[0]);
		VOE_TEST_CHECK_INT(got[1], shorter[1]);
	}

	// One byte is a file too.
	VOE_TEST_CHECK(voe_platform_file_write(WRITTEN, single, sizeof single,
					       &error));
	length = read_back(WRITTEN, got, sizeof got);
	VOE_TEST_CHECK_INT(length, 1);
	if (length == 1)
		VOE_TEST_CHECK_INT(got[0], single[0]);

	// A directory that is not there is UNAVAILABLE, and nothing is created:
	// neither the file nor the directory it was named in.
	error = VOE_BASE_OK;
	VOE_TEST_CHECK(!voe_platform_file_write(MISSING, payload,
						sizeof payload, &error));
	VOE_TEST_CHECK_INT(error, VOE_BASE_ERROR_UNAVAILABLE);
	VOE_TEST_CHECK_INT(read_back(MISSING, got, sizeof got), -1);

	// The out-parameter is optional, and a caller that does not want to know
	// which way still gets the answer in the return value.
	VOE_TEST_CHECK(voe_platform_file_write(WRITTEN, single, sizeof single,
					       NULL));
	VOE_TEST_CHECK(!voe_platform_file_write(MISSING, single, sizeof single,
						NULL));

	remove(WRITTEN);
	remove(MISSING);
	return voe_test_result();
}
