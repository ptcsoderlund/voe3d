// The `.glb` container: a twelve-byte header and a run of chunks. The JSON chunk
// holds the description and the binary chunk holds the vertex data and the
// pictures; this file finds them and hands them to model_gltf.c.
//
// THE LAYOUT, BECAUSE EVERY NUMBER IN IT IS SOMETHING TO CHECK:
//
//     magic     u32   0x46546C67, which is "glTF" little-endian
//     version   u32   2
//     length    u32   the whole file, header included
//     then, repeatedly:
//     length    u32   the chunk's bytes, not counting these eight
//     type      u32   0x4E4F534A "JSON" or 0x004E4942 "BIN\0"
//     data      length bytes, padded to a multiple of four
//
// EVERY LENGTH IS COMPARED AGAINST WHAT IS ACTUALLY THERE BEFORE IT IS USED, AND
// THE ADDITIONS ARE DONE IN SIXTY-FOUR BITS. Two thirty-two-bit lengths that
// each fit in the file can still sum to something that does not, and that is the
// arithmetic a hostile file goes looking for.
//
// THE FILE'S OWN `length` IS BELIEVED ONLY AS FAR AS IT AGREES WITH THE BUFFER.
// A file that says it is longer than the bytes handed over is truncated and is
// refused; one that says it is shorter is read up to what it says, because
// trailing bytes after the last chunk are somebody else's business and not a
// reason to refuse a model.
//
// THE JSON CHUNK MUST BE FIRST AND THERE MUST BE EXACTLY ONE OF EACH KIND. glTF
// says so, and a reader that took the last one it found would disagree with
// every other reader about what a file with two says.
//
// A CHUNK OF AN UNKNOWN TYPE IS SKIPPED, WHICH IS WHAT THE FORMAT ASKS FOR. The
// specification reserves the right to add chunk kinds and tells a reader to
// ignore the ones it does not know; that is different from an extension the
// description says it requires, which is refused in model_gltf.c.
//
// A `.gltf` IS REFUSED BY NAME. The text form is a different container with the
// same description inside it, and its buffers and images live in separate files
// that nothing here can open — so a file whose first bytes are not the magic
// number gets a message saying that rather than a generic complaint.
#include "model_gltf.h"

#include <base/assert.h>

#include <stdio.h>
#include <string.h>

// "glTF", "JSON" and "BIN\0" as little-endian words, which is how they sit in
// the file.
#define GLB_MAGIC 0x46546C67u
#define GLB_CHUNK_JSON 0x4E4F534Au
#define GLB_CHUNK_BIN 0x004E4942u

#define GLB_VERSION 2
#define GLB_HEADER_BYTES 12
#define GLB_CHUNK_HEADER_BYTES 8

// How many JSON values a description may hold. A glTF's description is a few
// values per mesh, material and node, so this is room for a large model and
// still a bound a small file cannot use to ask for a large allocation — the
// tokens are the one allocation whose size does not follow from the file's own
// content. Sixty-four thousand of them is about two megabytes.
#define GLB_MAX_JSON_TOKENS (1u << 16)

// A word out of the file, little-endian whatever this machine is. memcpy and a
// shift rather than a cast, because nothing guarantees a four-byte alignment at
// an offset that came out of a file.
static uint32_t word_at(const uint8_t *bytes, size_t offset)
{
	return (uint32_t)bytes[offset] | (uint32_t)bytes[offset + 1] << 8 |
	       (uint32_t)bytes[offset + 2] << 16 |
	       (uint32_t)bytes[offset + 3] << 24;
}

static bool refuse(voe_base_error *error, voe_base_error code,
		   const char *message)
{
	fprintf(stderr, "assets: glb: %s\n", message);
	if (error != NULL)
		*error = code;
	return false;
}

bool voe_assets_model_read_glb(const uint8_t *bytes, size_t size,
			       voe_base_arena *arena, voe_assets_model *model,
			       voe_base_error *error)
{
	voe_assets_json json;
	size_t declared;
	size_t pos = GLB_HEADER_BYTES;
	const uint8_t *json_chunk = NULL;
	size_t json_size = 0;
	const uint8_t *bin_chunk = NULL;
	size_t bin_size = 0;

	VOE_BASE_ASSERT(bytes != NULL, "reading a model from nothing");
	VOE_BASE_ASSERT(arena != NULL, "reading a model without an arena");
	VOE_BASE_ASSERT(model != NULL, "reading a model into nothing");

	if (size < GLB_HEADER_BYTES)
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "a file too short to hold even the header");

	if (word_at(bytes, 0) != GLB_MAGIC) {
		// The text form starts with whitespace or a brace, and that is
		// the mistake worth naming: it is a glTF, it is simply not one
		// this engine reads.
		if (bytes[0] == '{' || bytes[0] == ' ' || bytes[0] == '\n' ||
		    bytes[0] == '\t' || bytes[0] == '\r')
			return refuse(error, VOE_BASE_ERROR_UNSUPPORTED,
				      "this looks like a .gltf: the text form is not read, because its buffers and pictures are separate files and nothing here opens one. Export a .glb");
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "a file that does not start with the glTF magic number");
	}

	if (word_at(bytes, 4) != GLB_VERSION)
		return refuse(error, VOE_BASE_ERROR_UNSUPPORTED,
			      "a .glb container that is not version 2");

	declared = word_at(bytes, 8);
	if (declared < GLB_HEADER_BYTES)
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "a file whose declared length does not cover its own header");
	if (declared > size)
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "a file that claims to be longer than it is, which means it is truncated");

	while (pos < declared) {
		size_t length;
		uint32_t type;
		size_t padded;

		if (declared - pos < GLB_CHUNK_HEADER_BYTES)
			return refuse(error, VOE_BASE_ERROR_MALFORMED,
				      "a chunk header that does not fit in the file");

		length = word_at(bytes, pos);
		type = word_at(bytes, pos + 4);
		pos += GLB_CHUNK_HEADER_BYTES;

		if (length > declared - pos)
			return refuse(error, VOE_BASE_ERROR_MALFORMED,
				      "a chunk that claims more bytes than the file has left");

		if (type == GLB_CHUNK_JSON) {
			if (json_chunk != NULL)
				return refuse(error, VOE_BASE_ERROR_MALFORMED,
					      "a file with two JSON chunks in it");
			if (pos != GLB_HEADER_BYTES + GLB_CHUNK_HEADER_BYTES)
				return refuse(error, VOE_BASE_ERROR_MALFORMED,
					      "a JSON chunk that is not the first chunk");
			json_chunk = bytes + pos;
			json_size = length;
		} else if (type == GLB_CHUNK_BIN) {
			if (bin_chunk != NULL)
				return refuse(error, VOE_BASE_ERROR_MALFORMED,
					      "a file with two binary chunks in it");
			bin_chunk = bytes + pos;
			bin_size = length;
		}

		// Chunks are padded to four bytes and the padding is not part of
		// the length. Rounding up here is what keeps the next header
		// where it belongs; a reader that stepped by the length alone
		// reads a header out of the middle of a chunk.
		padded = (length + 3u) & ~(size_t)3u;
		if (padded > declared - pos)
			return refuse(error, VOE_BASE_ERROR_MALFORMED,
				      "a chunk whose padding runs off the end of the file");
		pos += padded;
	}

	if (json_chunk == NULL)
		return refuse(error, VOE_BASE_ERROR_MALFORMED,
			      "a file with no JSON chunk in it");

	// The chunk is padded with spaces, which JSON treats as whitespace, so
	// the padding needs no removing. The span is handed over as a length and
	// never as a C string: there is no terminator anywhere in here.
	if (!voe_assets_json_parse((const char *)json_chunk, json_size, arena,
				   GLB_MAX_JSON_TOKENS, &json, error))
		return false;

	return voe_assets_model_from_gltf(&json, bin_chunk, bin_size, arena,
					  model, error);
}
