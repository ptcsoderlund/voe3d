// Sounds: the bytes of a WAV file in, interleaved float samples out. Like the
// picture decoders beside it, nothing here opens a file; the caller hands over
// the bytes and an arena, and the samples live in that arena.
//
// WHAT IS READ: a RIFF/WAVE file whose `fmt ` chunk says tag 1 (PCM) at 16 bits,
// tag 3 (IEEE float) at 32 bits, or tag 0xFFFE (extensible) whose subformat is
// either of those two; one or two channels; any rate above zero. The walk goes
// over every chunk in the RIFF, so `fmt ` and `data` may come in any order,
// every other chunk (`LIST`, `fact`, `cue `...) is skipped, and a chunk of odd
// size is followed by its pad byte. A final pad byte missing at the end of the
// file is tolerated, as is a RIFF size larger than the file when every chunk
// still fits.
//
// WHAT COMES OUT: floats, interleaved, `frames * channels` of them. A 16-bit
// sample s becomes s / 32768.0f, so -32768 is exactly -1 and 32767 falls just
// short of 1; a 32-bit float sample is copied bit for bit. The rate and channel
// count are the file's own; converting to what the device plays is the mixer's
// job, not the decoder's.
//
// VOE_BASE_ERROR_UNSUPPORTED is a well-formed file this reader will not take:
// another format tag or subformat, another bit depth (8-bit, 24-bit, 64-bit
// float), more than two channels. VOE_BASE_ERROR_MALFORMED is a broken one: no
// `RIFF`/`WAVE` magic, no `fmt ` or no `data`, a `fmt ` too short for its tag,
// a chunk running past the end of the bytes, a `data` size that is not a whole
// number of frames, zero channels or a rate of zero. Either way the failure is
// reported where it is found, and `sound` is left as it was.
//
// WHY ONLY WAV (ADR-0266 point 2): the first sound needs one uncompressed
// format that every tool writes, and WAV is that. A compressed format is a
// decoder of its own and waits for a card that needs one.
//
// CONSTRAINTS: the whole file is in memory, and the samples take at most twice
// the size of the `data` chunk (16-bit to float), so a hostile size cannot ask
// for more than the bytes the caller already holds justify.
#pragma once

#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// A decoded sound. `samples` holds frames * channels floats, the channels of a
// frame side by side (left then right), in the arena passed to the decoder, so
// it is freed by rewinding or destroying that arena (rule 11).
typedef struct {
	uint32_t rate;
	uint32_t channels;
	uint64_t frames;
	float *samples;
} voe_assets_sound;

// Decode a WAV file. False on failure with the category in `error` (which may
// be NULL), and `sound` untouched.
[[nodiscard]] bool voe_assets_wav_decode(const uint8_t *bytes, size_t size,
					 voe_base_arena *arena,
					 voe_assets_sound *sound,
					 voe_base_error *error);
