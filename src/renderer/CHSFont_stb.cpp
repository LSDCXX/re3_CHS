// stb_truetype implementation unit.
//
// The single-header library's implementation (STB_TRUETYPE_IMPLEMENTATION)
// is compiled here, once, so the game's CHSFont.cpp only sees the header's
// declarations. re3 does not link the librw imgui target, so these symbols
// are not defined anywhere else in the executable.
#include "common.h"

#ifdef _WIN32
// stb_truetype v1.14's rasterizer asserts on some rare glyph data (e.g.
// variable-font emoji glyphs whose default-instance glyf coordinates fall
// out of range after scaling). Asserting must NEVER crash the game: glyph
// rasterization is best-effort, an aberrant glyph just produces a blank or
// slightly wrong cell. This TU is the only place STB_TRUETYPE_IMPLEMENTATION
// is compiled, so disabling the assert here is scoped to stb itself and
// does not affect any other code.
#ifndef STBTT_assert
#define STBTT_assert(x) ((void)0)
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#include "skeleton/imgui/imstb_truetype.h"
#endif
