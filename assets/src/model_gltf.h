// The half of the model reader that reads glTF's JSON, kept apart from the half
// that reads the `.glb` container. Internal to this folder.
//
// THE SPLIT IS BY WHAT A READER IS CHASING. A file that is not a `.glb` at all —
// a bad magic number, a chunk that does not fit — is model_glb.c's; a file whose
// container is fine and whose description is wrong is this file's.
#pragma once

#include "json.h"

#include <assets/model.h>
#include <base/arena.h>
#include <base/error.h>

#include <stddef.h>
#include <stdint.h>

// `bin` is the `.glb`'s binary chunk, which is the only buffer a glTF read here
// may name — a buffer with a URI in it is refused, because nothing in this
// engine opens a file. It may be NULL with a size of zero for a file whose
// description references no buffer at all.
[[nodiscard]] bool voe_assets_model_from_gltf(const voe_assets_json *json,
					      const uint8_t *bin,
					      size_t bin_size,
					      voe_base_arena *arena,
					      voe_assets_model *model,
					      voe_base_error *error);
