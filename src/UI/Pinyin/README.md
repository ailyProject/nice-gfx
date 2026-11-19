# NICE GFX Pinyin engine

`Pinyin.h` is a standalone, header-only C++11 library. It uses fixed-size
composition storage and performs lookup directly against caller-owned tables.
It requires the standard C integer and size declarations. Include the header
in firmware, native host code, or a WebAssembly build. The engine does not
allocate memory, open files, or draw the keyboard.

From CMake, use `add_subdirectory` on this directory, link the
`nice_gfx_pinyin` interface target, and include `<Pinyin.h>`.

`Dictionary` refers to four arrays that stay valid as long as the engine uses
them. The legacy format uses sorted, NUL-terminated lowercase ASCII spellings
and NUL-terminated UTF-8 candidates. Each list has an additional terminating
NUL. Both offset arrays contain byte offsets and have one entry per spelling.

Studio packs dictionaries by setting the high bit of the descriptor count.
Spelling offsets count five-bit symbols with zero as terminator. Candidate
words encode a BMP Hanzi offset from U+4E00 and set the high bit on the final
Hanzi of each candidate. Two consecutive value offsets bound each list.
The offset array holds 16-bit word offsets in pairs in 32-bit words and
includes the final end offset. Supply trusted arrays with this structure.

Use `Composition::push` to add a Latin letter and `find` to get the longest
matching prefix. Call `candidate(match.candidates(), index)` to select a
candidate and `writeCandidate` to emit its UTF-8 bytes. Once the host accepts
the value, call `Composition::consume` with the match's `consumed` count.
The host enforces the field limits and decides when to commit.

The default functions read ordinary memory. For tables in program memory,
instantiate `find`, `candidate`, and `writeCandidate` with a reader policy that supplies
`static read` overloads for pointers to `uint8_t`, `uint16_t`, and `uint32_t`.
