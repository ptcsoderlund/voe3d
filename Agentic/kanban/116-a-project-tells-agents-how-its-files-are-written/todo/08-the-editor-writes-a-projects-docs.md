# 08 — The editor writes a project's Docs
folder: editor
after: 02, 04, 07
decisions: 0168, 0505, 0509, 0511

## Change
- `editor/src/docs.h` and `editor/src/docs.c` are new. They provide
  `[[nodiscard]] bool voe_editor_docs_write(const voe_editor_project *project, voe_base_arena *scratch,
  const char **why);` (`editor/src/project.h` holds `folder` and `world`).
  - **Untitled.** A NULL `folder` writes nothing and returns true.
  - **Folder.** Otherwise create `<folder>/Docs/` when it is missing (`platform/folder.h`).
  - **Files.** Write exactly three files there:
    - `voe3d-scene.md`, from `voe_authoring_reference_scene(project->world, …)`
      (`authoring/reference.h`);
    - `voe3d-files.md`, from `voe_authoring_reference_keys` for the material, parts, landscape and
      project tables (`voe_assets_material_keys`, `_parts_keys`, `_landscape_keys`,
      `voe_authoring_project_keys`), joined in that order, each title naming its extension;
    - `voe3d.md`, the index. It says:
      - which file kind is documented in which file;
      - the `Docs/voe3d*.md` files are the editor's and are rewritten on every open, Refresh and code
        reload, while every other file in `Docs/` is the user's;
      - the editor takes in a file changed on disk (0509), so editing the text files is enough;
      - the check command, written out whole as `<program> --check <project folder>`, with
        `voe_platform_path_program` giving the program and both paths absolute. It also gives the
        command's output, `<path>: <why>` per refused file and then the summary, and its exits:
        0 all well, 1 refused, 2 usage;
      - the engine's headers are at `<VOE_TOOLCHAIN_ENGINE>/<folder>/include/<folder>/` and are
        included as `#include <folder/x.h>`, with each folder's `<folder>.md` as the place to look
        first. It lists the engine folders that have an `include/` (`voe_platform_folder_list`).
        `#include "toolchain.h"` gives `VOE_TOOLCHAIN_ENGINE`, as in `editor/src/splash.c`.
  - **Writing.** Read each file first and write it with `voe_platform_file_write` (`platform/file.h`)
    only when its bytes differ. Nothing else in `Docs/` is read, written or removed.
  - **Instruction files.** The editor creates and names no agent instruction file, and neither file
    contains those names.
- `editor/src/src.md`, `editor/editor.md`: add entries for the new files.

## Meaning
- Open: rewriting `Docs/` on every open is the tech-lead's assumption; this card builds on it.
- Holds: no code mentions `Docs/`. The watch does not skip it, but it sees no `.md` file.
- Holds: the engine headers exist only in the source tree, at `VOE_TOOLCHAIN_ENGINE`.

## Done when
`grep -q voe_authoring_reference_scene editor/src/docs.c && ! grep -n 'CLAUDE\.md\|AGENTS\.md' editor/src/docs.c editor/src/docs.h`
exits 0. Card 09 runs the code.
