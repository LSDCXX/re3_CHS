#pragma once

// Optional DirectWrite grayscale rasterizer used by CHSFont.
//
// This is an additional rendering backend, NOT a replacement of the GDI
// path: the existing GDI GetGlyphOutlineW pipeline and the stb_truetype
// supplementary-plane path stay untouched. CHSFont picks the backend via
// the [Fonts] TextRenderer key (default "DWrite", fallback "GDI").
//
// Why DirectWrite: it is a Windows system component (dwrite.dll, built into
// Windows 7+), so no third-party dependency (no FreeType) is introduced.
// The engine binds an IDWriteFontFace to the same HDC CHSFont already
// selects fonts into, so the face selection (NormalFonts / SlantFontFile /
// bold check) is reused 100%: DirectWrite renders exactly the font GDI
// would, and the rest of the pipeline (64x64 atlas cells, slot cache, batch
// drawing, surrogate merging) is unchanged. The rasterized glyph is still a
// gray alpha image written as R=G=B=A, identical to the GDI output.
//
// All comments are English on purpose: the project sources are GBK-encoded.

// project-wide types (nil, uint32, uint8) live in common.h; must come first.
#include "common.h"

#ifdef _WIN32
#include <windows.h>
#include <dwrite.h>
#include <vector>

// Rasterization result of one codepoint. Placement convention:
//   - The engine draws the glyph into an internal 64x64 DIB whose origin is
//     the CELL TOP-LEFT corner; the baseline origin is placed at y = baseY
//     (the value the caller passes).
//   - left/top are therefore the black box position RELATIVE TO THE CELL
//     TOP-LEFT (both >= 0 for a normally placed glyph; the box is already
//     where the atlas cell wants it).
//   - CHSFont uses: x0 = penX + left  (centers the advance cell then adds
//     the glyph's offset inside it) and  y0 = top  (direct absolute cell
//     row placement - the engine already accounted for the baseline).
struct ChsDWOut
{
	// Zero-initialized so every field is deterministic even if a code path
	// forgets to set one (a real bug was a missing out.w/out.h assignment,
	// which made the atlas write loops see stack garbage and write nothing).
	ChsDWOut() : w(0), h(0), left(0), top(0), advance(0), missing(false),
	             isColor(false) {}
	int w;        // alpha bitmap width (pixels)
	int h;        // alpha bitmap height (pixels)
	int left;     // black-box left, relative to the cell top-left (>= 0)
	int top;      // black-box top,  relative to the cell top-left (>= 0)
	int advance;  // advance width in pixels (same meaning as GDI gmCellIncX)
	bool missing; // glyph index == 0: codepoint absent in this face
	// true when the glyph was rasterized in COLOR (COLR/CPAL layers, e.g.
	// emoji). The color buffer then carries RGBA pixels (w*h*4 bytes) and
	// the atlas cell must be written as-is (no vertex-color modulation);
	// false for regular gray glyphs (alpha-only, RGB all equal).
	bool isColor;
};

// Create the DirectWrite factory, get the GDI interop and bind one
// IDWriteFontFace per glyph set from the given HDCs (each must have its
// font selected, exactly as CHSFont::Init leaves them). Idempotent:
// calling it again shuts the previous state down first.
bool ChsDwInit(HDC hdcNormal, HDC hdcSlant);
void ChsDwShutdown(void); // release everything; idempotent
bool ChsDwOk(void);       // engine ready and at least one face bound

// Load the RareFontFile fallback chain (comma-separated file paths, exactly
// as CHSFont reads from re3.ini's [Fonts] RareFontFile). Each file becomes
// an IDWriteFontFace used for supplementary-plane codepoints (>0xFFFF, e.g.
// CJK ext / emoji), mirroring the stb_truetype fallback semantics: for a
// given codepoint, the first face whose GetGlyphIndices returns a non-zero
// glyph wins. Variable fonts get their weight axis set (wght) when present.
// Call once after ChsDwInit; idempotent (previous list is released).
bool ChsDwLoadRareFonts(const char *csvPaths);

// Apply one weight (100-900) to ONE main set (0 = normal, 1 = slant) through
// the variable-font wght axis. Call after ChsDwInit; static faces (e.g. a
// bold-file font) keep their own weight.
void ChsDwSetWeight(int setIndex, float weight);
// Set the weight used by the rare (supplementary-plane) chain. Call BEFORE
// ChsDwLoadRareFonts so every rare face picks it up.
void ChsDwSetRareWeight(float weight);

// Rasterize one codepoint (BMP or supplementary plane) for the given set
// (slant 0/1) at emPx pixels, with the cell baseline at baseY pixels from
// the cell top (the caller passes its gBaseY[set] so the glyph aligns
// exactly with the GDI/stb baseline). For a gray glyph, alphaOut receives
// w*h bytes (row stride = w, value = gray alpha 0..255) and out.isColor is
// false; for a color glyph (COLR/CPAL), colorOut receives w*h*4 RGBA bytes
// (row stride = w*4) and out.isColor is true. Returns false on engine
// failure or missing glyph; the caller then falls back to the GDI/stb path
// (same behavior as today).
bool ChsDwRender(uint32 cp, bool slant, int emPx, int baseY,
                 ChsDWOut &out, std::vector<uint8> &alphaOut,
                 std::vector<uint8> &colorOut);

#endif // _WIN32
