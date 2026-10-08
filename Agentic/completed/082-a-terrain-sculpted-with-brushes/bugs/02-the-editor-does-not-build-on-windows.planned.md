# 02 — The editor does not build on Windows

## Seen
On Windows, building the debug preset stops while linking the editor:

```
[208/209] Linking C executable editor\voe_editor.exe
lld-link: error: undefined symbol: voe_platform_file_move
>>> referenced by ...\voe3d\editor\src\assets_manage.c:342
>>>               editor/CMakeFiles/voe_editor.dir/src/assets_manage.c.obj:(voe_editor_assets_move)

lld-link: error: undefined symbol: voe_platform_trash
>>> referenced by ...\voe3d\editor\src\assets_manage.c:452
>>>               editor/CMakeFiles/voe_editor.dir/src/assets_manage.c.obj:(voe_editor_assets_trash)
clang: error: linker command failed with exit code 1 (use -v to see invocation)
ninja: build stopped: subcommand failed.
```

The Assets panel's move and delete (081) were written for Linux only, under 0339, which 0381 replaces.

## Expected
The editor builds and starts on Windows. The Assets panel does everything there that it does on Linux. Dragging
a file or folder onto a folder or Up moves it. Rename renames. Delete sends the file or folder to the Recycle
Bin, where I can restore it from Explorer.

## How to reproduce
1. On Windows, configure and build the debug preset.
2. Linking `voe_editor.exe` fails with the errors above.
