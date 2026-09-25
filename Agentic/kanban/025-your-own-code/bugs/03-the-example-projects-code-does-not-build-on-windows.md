# 03 — The example project's code does not build on Windows

## Seen
Running the editor on the example project on Windows, building the project's code fails. The configure
succeeds; the build then stops before compiling anything. From `examples/capsule/Build/build.log`:

```
-- Build files have been written to: C:/Users/PerSoderlund/Dev/GitRepos/Github/voe3d/examples/capsule/Build/editor
[0/2] Re-checking globbed directories...
CMake Error at CMakeFiles/VerifyGlobs.cmake:690 (file):
  Syntax error in cmake code at

    C:/Users/PerSoderlund/Dev/GitRepos/Github/voe3d/examples/capsule/Build/editor/CMakeFiles/VerifyGlobs.cmake:690

  when parsing string

    C:\Users\PerSoderlund\Dev\GitRepos\Github\voe3d\examples\capsule\Code/*.c

  Invalid character escape '\U'.

ninja: error: rebuilding 'build.ninja': subcommand failed
FAILED: [code=1] C:/Users/PerSoderlund/Dev/GitRepos/Github/voe3d/examples/capsule/Build/editor/CMakeFiles/cmake.verify_globs
```

The project's code folder reaches the build as a Windows path with backslashes, and CMake reads `\U` as an
escape. Compiler: Clang 22.1.8; CMake: the one bundled with CLion 2025.2.

## Expected
The project's code builds and loads, and the capsule moves with the keys, the same as on Linux.

## How to reproduce
1. On Windows, build the editor.
2. Open `examples/capsule/` in the editor.
3. Build the project's code (open it, or press refresh).
4. Read `examples/capsule/Build/build.log`: the error above.
