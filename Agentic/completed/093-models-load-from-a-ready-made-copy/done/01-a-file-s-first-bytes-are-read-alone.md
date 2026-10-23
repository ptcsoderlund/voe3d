# 01 — A file's first bytes are read alone
folder: platform
after: none
decisions: 0168, 0470

## Change
The editor's check before Play reads only the head of each cooked copy (0470 point 6). Today the platform
can only read a whole file.

- `platform/include/platform/file.h`: add
  `[[nodiscard]] bool voe_platform_file_read_head(const char *path, uint8_t *buffer, size_t size, size_t *out_count, voe_base_error *error)`.
  It reads up to `size` bytes from the start of the file into the caller's buffer, and `*out_count` is
  how many arrived (fewer for a shorter file). It fails with the same categories as
  `voe_platform_file_read`. A NULL path, buffer or out_count, or a size of 0, aborts, as the header's other
  caller bugs do. Its comment says it is for a reader that checks a head before reading the whole file.
- `platform/src/file_wayland.c`: open, read in a loop until `size` or end of file, close.
- `platform/src/file_win32.c`: the same with CreateFileW, ReadFile and CloseHandle. Convert the path as
  this file's read already does.
- `platform/src/src.md`: the entries of the two files name the head read.
- `platform/tests/file.c`: add a case `read_head_reads_the_start` that writes a 10-byte file and checks
  three reads: 4 bytes give its first 4; 64 bytes give all 10, count 10; a missing path is false with
  UNAVAILABLE.
- `platform/tests/tests.md`: the `file.c` entry names the case.

## Done when
`ctest --test-dir build/debug -R '^platform/file'` passes, with `read_head_reads_the_start` in
`platform/tests/file.c`.
