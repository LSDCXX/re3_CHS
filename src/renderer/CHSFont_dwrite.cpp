// DirectWrite grayscale rasterizer backend for CHSFont (see CHSFont_dwrite.h).
//
// Implementation notes:
// - Single TU for this backend; the rest of the CHSFont pipeline is
//   untouched. All comments are English on purpose (GBK sources).
// - The engine binds IDWriteFontFace objects to the same HDCs CHSFont
//   already selects fonts into, so the face selection logic in CHSFont.cpp
//   (NormalFonts / SlantFontFile / bold check) is reused as-is.
// - Rasterization uses IDWriteBitmapRenderTarget (a 32-bit top-down DIB with
//   a memory DC), which renders glyph coverage WITHOUT ClearType sub-pixel
//   colors when a grayscale rendering params object is used. The glyph is
//   drawn in white over a black (transparent) DIB, so the red channel IS the
//   gray coverage - same data the GDI GetGlyphOutlineW GGO_GRAY8 path
//   produces, and it is written into the atlas as R=G=B=A like GDI/stb do.
// - Geometry mapping is deliberately identical to the GDI path in
//   CHSFont.cpp:RenderGlyph:
//       penX  = (64 - advancePx) / 2             (advance cell centered)
//       x0    = penX + blackBox.left             (glyph offset in the cell)
//       y0    = blackBox.top                     (absolute cell row)
//   DrawGlyphRun is given the baseline origin at (0, baseY) - the caller's
//   gBaseY[set] - so the glyph body lands inside the 64x64 DIB and the
//   reported black box is relative to the DIB top-left corner, which the
//   caller treats as the atlas cell top-left. Row placement is therefore a
//   direct absolute cell coordinate (same net result as GDI's baseline
//   math, without needing a per-path baseline conversion here).
// - No third-party dependency: everything is dwrite.dll (Windows 7+).

// project-wide types (nil, uint32, uint8) live in common.h; must come first.
#include "common.h"

#ifdef _WIN32
#include "CHSFont_dwrite.h"
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <vector>

// Color glyphs (COLR/CPAL) and variable-font axes live on the newer DWrite
// interfaces: IDWriteFontFace2/IDWriteFactory2 (dwrite_2.h), and
// IDWriteFontFace3/5, IDWriteFontResource, IDWriteFontFaceReference,
// IDWriteFactory3 (dwrite_3.h). dwrite.h alone only has the v1 interfaces.
#include <dwrite_2.h>
#include <dwrite_3.h>

#ifdef _MSC_VER
#pragma comment(lib, "dwrite.lib")
#endif

#define CHS_DW_CELL 64 // pixel size of one atlas glyph cell (same as CHS_SLOT_PIXELS)

// Debug log (append) written next to the game exe; helps diagnose DWrite
// failures. This TU cannot use CHSFont.cpp's static ChsLog, so it logs to
// its own file. English-only on purpose (GBK sources).
static void DwLog(const char *fmt, ...)
{
#ifndef CHS_FONT_VERBOSE_LOG
	(void)fmt;
	return; // opt-in only: per-glyph disk logging causes visible subtitle stalls
#else
	char buf[512];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	FILE *f = fopen("chsfont_dw.log", "a");
	if(f) {
		fputs(buf, f);
		fputc('\n', f);
		fclose(f);
	}
#endif
}

static IDWriteFactory          *sFactory = nil;
static IDWriteGdiInterop       *sGdi = nil;
static IDWriteFontFace         *sFace[2] = { nil, nil };
static IDWriteBitmapRenderTarget *sRT[2] = { nil, nil };
static IDWriteRenderingParams  *sParams = nil; // forced grayscale
static bool                     sOk = false;

// Supplementary-plane (>)0xFFFF) fallback chain loaded from RareFontFile.
// Each entry is one font file; for a given codepoint the FIRST face whose
// GetGlyphIndices yields a non-zero glyph wins (same semantics as the stb
// RareFontFile fallback in CHSFont_stb.cpp).
#define CHS_MAX_RARE_FACES 16
static IDWriteFontFace         *sRareFace[CHS_MAX_RARE_FACES] = { nil, nil, nil, nil, nil, nil, nil, nil,
                                                                 nil, nil, nil, nil, nil, nil, nil, nil };
static int                      sRareCount = 0;
// Weight used for variable rare fonts (wght axis). Default 400 (regular);
// CHSFont may override via re3.ini. Applied at face creation.
static float                    sRareWeight = 400.0f;


void ChsDwShutdown(void)
{
	if(sParams) { sParams->Release(); sParams = nil; }
	if(sRT[1]) { sRT[1]->Release(); sRT[1] = nil; }
	if(sRT[0]) { sRT[0]->Release(); sRT[0] = nil; }
	for(int i = 0; i < sRareCount; i++) {
		if(sRareFace[i]) { sRareFace[i]->Release(); sRareFace[i] = nil; }
	}
	sRareCount = 0;
	if(sFace[1]) { sFace[1]->Release(); sFace[1] = nil; }
	if(sFace[0]) { sFace[0]->Release(); sFace[0] = nil; }
	if(sGdi) { sGdi->Release(); sGdi = nil; }
	if(sFactory) { sFactory->Release(); sFactory = nil; }
	sOk = false;
}

bool ChsDwInit(HDC hdcNormal, HDC hdcSlant)
{
	ChsDwShutdown(); // reset first (idempotent)
	DwLog("[DwBuild] REV6 (per-set NormalWeight / SlantWeight / RareWeight)"); // version marker

	HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory), (IUnknown **)&sFactory);
	if(FAILED(hr) || sFactory == nil) {
		DwLog("[DwInit] DWriteCreateFactory failed: hr=0x%08X", (unsigned)hr);
		return false;
	}

	hr = sFactory->GetGdiInterop(&sGdi);
	if(FAILED(hr) || sGdi == nil) {
		DwLog("[DwInit] GetGdiInterop failed: hr=0x%08X", (unsigned)hr);
		ChsDwShutdown();
		return false;
	}

	// Force grayscale rendering (no ClearType sub-pixel colors): clearType
	// level 0 + FLAT pixel geometry + natural mode gives pure gray coverage,
	// which is exactly what the GDI GGO_GRAY8 output looks like.
	hr = sFactory->CreateCustomRenderingParams(1.0f, 1.0f, 0.0f,
		DWRITE_PIXEL_GEOMETRY_FLAT, DWRITE_RENDERING_MODE_NATURAL, &sParams);
	if(FAILED(hr) || sParams == nil) {
		DwLog("[DwInit] CreateCustomRenderingParams failed: hr=0x%08X", (unsigned)hr);
		ChsDwShutdown();
		return false;
	}

	// Bind one face and one bitmap render target per glyph set (normal +
	// slant). A nil HDC or a failure only leaves that set unbound; the
	// engine still works for the other set.
	HDC hdcs[2] = { hdcNormal, hdcSlant };
	for(int i = 0; i < 2; i++) {
		if(hdcs[i] == nil)
			continue;
		IDWriteFontFace *face = nil;
		if(SUCCEEDED(sGdi->CreateFontFaceFromHdc(hdcs[i], &face)) && face)
			sFace[i] = face;
		else
			DwLog("[DwInit] CreateFontFaceFromHdc failed for set %d: hr=0x%08X",
			      i, (unsigned)hr);
		if(sGdi->CreateBitmapRenderTarget(nil, CHS_DW_CELL, CHS_DW_CELL,
		                                  &sRT[i]) != S_OK) {
			sRT[i] = nil;
			DwLog("[DwInit] CreateBitmapRenderTarget failed for set %d", i);
		} else {
			// Force 1 DIP = 1 pixel so glyphs are drawn at exactly emPx and
			// the reported black box is in real pixels (system DPI scaling
			// would otherwise enlarge the glyph, e.g. x1.5 at 150% DPI,
			// blowing past the 64x64 cell).
			sRT[i]->SetPixelsPerDip(1.0f);
			DwLog("[DwInit] render target created for set %d (pixelsPerDip=1)", i);
		}
	}

	sOk = sFactory != nil && sGdi != nil && sParams != nil &&
	      (sFace[0] != nil || sFace[1] != nil);
	DwLog("[DwInit] done: sOk=%d face0=%d face1=%d rt0=%d rt1=%d",
	      sOk ? 1 : 0, sFace[0] != nil ? 1 : 0, sFace[1] != nil ? 1 : 0,
	      sRT[0] != nil ? 1 : 0, sRT[1] != nil ? 1 : 0);
	return sOk;
}

bool ChsDwOk(void)
{
	return sOk;
}

// forward: color-layer compositor defined below ChsDwRender
static bool ChsDwRenderColor(uint32 cp, int slot, int baseY,
                             const DWRITE_GLYPH_RUN &run, int emPx,
                             ChsDWOut &out, std::vector<uint8> &colorOut);

// Apply the configured weight axis to a variable font face (wght axis).
// DWrite's official path: face -> IDWriteFontFace5 -> GetFontResource ->
// CreateFontFace(simulations, axisValues, count, &newFace5). The returned
// face has the axis baked in. Falls back silently when the interfaces are
// unavailable (old Windows) or the font has no variations.
static IDWriteFontFace *
ChsDwApplyWeight(IDWriteFontFace *src, float weight)
{
	if(src == nil)
		return nil;
	IDWriteFontFace5 *f5 = nil;
	if(FAILED(src->QueryInterface(__uuidof(IDWriteFontFace5), (void **)&f5)) || f5 == nil)
		return src; // old system: keep the plain face
	BOOL hasVar = f5->HasVariations();
	IDWriteFontResource *res = nil;
	HRESULT hr = f5->GetFontResource(&res);
	IDWriteFontFace5 *newFace = nil;
	if(hasVar && SUCCEEDED(hr) && res) {
		DWRITE_FONT_AXIS_VALUE axis;
		axis.axisTag = DWRITE_FONT_AXIS_TAG_WEIGHT;
		axis.value = weight;
		if(SUCCEEDED(res->CreateFontFace(DWRITE_FONT_SIMULATIONS_NONE,
		                                 &axis, 1, &newFace)) && newFace) {
			// The new face references the same font data; release the
			// resource, keep only the weighted face.
			res->Release();
			f5->Release();
			return newFace;
		}
	}
	if(res) res->Release();
	f5->Release();
	return src;
}

// Apply one weight to ONE main set (0 = normal, 1 = slant) after Init.
// Called by CHSFont when re3.ini [Fonts] NormalWeight / SlantWeight is
// set. Variable fonts honor the wght axis; static faces keep their own
// weight (a bold-file font stays bold).
void ChsDwSetWeight(int setIndex, float weight)
{
	if(setIndex < 0 || setIndex > 1)
		return;
	if(!sFace[setIndex])
		return;
	IDWriteFontFace *w = ChsDwApplyWeight(sFace[setIndex], weight);
	if(w != sFace[setIndex]) {
		sFace[setIndex]->Release();
		sFace[setIndex] = w;
		DwLog("[DwSetWeight] set %d face re-bound to weight %.0f", setIndex, weight);
	}
}

// Set the weight used by the rare (supplementary-plane) chain. Called by
// CHSFont when re3.ini [Fonts] RareWeight is set, BEFORE the chain loads
// so every rare face picks it up.
void ChsDwSetRareWeight(float weight)
{
	sRareWeight = weight;
	DwLog("[DwSetRareWeight] rare weight %.0f", weight);
}

// Load the RareFontFile fallback chain. csvPaths is a comma-separated list
// of font file paths (exactly the [Fonts] RareFontFile value CHSFont reads).
// Each path becomes an IDWriteFontFace stored in sRareFace[]. Existing
// faces are released first (idempotent). Returns the number of faces
// loaded (0 on total failure). A missing/unreadable file is skipped with a
// log entry, not a hard error - the remaining chain still works.
bool ChsDwLoadRareFonts(const char *csvPaths)
{
	for(int i = 0; i < sRareCount; i++)
		if(sRareFace[i]) { sRareFace[i]->Release(); sRareFace[i] = nil; }
	sRareCount = 0;
	if(!sFactory || !csvPaths)
		return sRareCount;

	IDWriteFactory3 *f3 = nil;
	if(FAILED(sFactory->QueryInterface(__uuidof(IDWriteFactory3), (void **)&f3)) || f3 == nil) {
		DwLog("[DwRare] IDWriteFactory3 unavailable - no rare fonts");
		return 0;
	}

	char tmp[1024];
	strncpy(tmp, csvPaths, sizeof(tmp) - 1);
	tmp[sizeof(tmp) - 1] = '\0';
	char *save = nil;
	for(char *tok = strtok_s(tmp, ",", &save);
	    tok && sRareCount < CHS_MAX_RARE_FACES;
	    tok = strtok_s(nil, ",", &save)) {
		// trim spaces
		while(*tok == ' ' || *tok == '\t') tok++;
		char *end = tok + strlen(tok);
		while(end > tok && (end[-1] == ' ' || end[-1] == '\t')) *--end = '\0';
		if(*tok == '\0')
			continue;

		WCHAR wpath[512];
		if(MultiByteToWideChar(CP_UTF8, 0, tok, -1, wpath, 512) == 0 &&
		   MultiByteToWideChar(CP_ACP, 0, tok, -1, wpath, 512) == 0) {
			DwLog("[DwRare] skip un-decodable path: %s", tok);
			continue;
		}

		IDWriteFontFaceReference *ref = nil;
		HRESULT hr = f3->CreateFontFaceReference(wpath, nil, 0, DWRITE_FONT_SIMULATIONS_NONE, &ref);
		if(FAILED(hr) || ref == nil) {
			DwLog("[DwRare] CreateFontFaceReference failed for %s (hr=0x%08X)", tok, (unsigned)hr);
			continue;
		}
		IDWriteFontFace3 *face3 = nil;
		hr = ref->CreateFontFace(&face3);
		ref->Release();
		if(FAILED(hr) || face3 == nil) {
			DwLog("[DwRare] CreateFontFace failed for %s (hr=0x%08X)", tok, (unsigned)hr);
			continue;
		}
		IDWriteFontFace *face = ChsDwApplyWeight(face3, sRareWeight);
		if(face3 != face) face3->Release(); // weighted replacement
		sRareFace[sRareCount++] = face;
		DwLog("[DwRare] loaded %s -> face %d", tok, sRareCount - 1);
	}

	f3->Release();
	DwLog("[DwRare] done: %d faces in chain", sRareCount);
	return sRareCount;
}

// Pick the first rare face that contains the codepoint (glyph index != 0),
// exactly the stb RareFontFile fallback semantics. Returns nil when no
// face in the chain has the glyph.
static IDWriteFontFace *
ChsDwPickRareFace(uint32 cp)
{
	UINT16 glyph = 0;
	for(int i = 0; i < sRareCount; i++) {
		if(sRareFace[i] == nil)
			continue;
		glyph = 0;
		sRareFace[i]->GetGlyphIndices(&cp, 1, &glyph);
		if(glyph != 0)
			return sRareFace[i];
	}
	return nil;
}

// Read back the DIB pixels of one render target into a gray alpha bitmap.
// The glyph is drawn in white, so every channel equals the coverage; the
// red channel is taken. Returns false if the DIB cannot be locked.
static bool ChsDwReadDIB(int slot, ChsDWOut &out,
                         const RECT &bbox, std::vector<uint8> &alphaOut)
{
	HDC dc = sRT[slot]->GetMemoryDC();
	if(!dc) {
		DwLog("[DwReadDIB] set %d: GetMemoryDC failed", slot);
		return false;
	}
	HGDIOBJ hbmp = GetCurrentObject(dc, OBJ_BITMAP);
	if(!hbmp) {
		DwLog("[DwReadDIB] set %d: GetCurrentObject(OBJ_BITMAP) failed", slot);
		return false;
	}

	DIBSECTION ds;
	ZeroMemory(&ds, sizeof(ds));
	if(GetObject(hbmp, sizeof(ds), &ds) == 0 || ds.dsBm.bmBits == nil) {
		DwLog("[DwReadDIB] set %d: GetObject(DIBSECTION) failed or no bits", slot);
		return false;
	}

	const int w = bbox.right - bbox.left;
	const int h = bbox.bottom - bbox.top;
	if(w <= 0 || h <= 0) { // oversized boxes allowed: caller shrinks em and re-renders
		DwLog("[DwReadDIB] set %d: bad box w=%d h=%d (bbox L%d T%d R%d B%d)", slot,
		      w, h, bbox.left, bbox.top, bbox.right, bbox.bottom);
		return false;
	}

	// Black box offsets relative to the DIB top-left corner, which the
	// caller treats as the atlas cell top-left (the engine drew with the
	// baseline at baseY, so the glyph body is inside the cell).
	out.left = bbox.left;
	out.top = bbox.top;
	// CRITICAL: out.w/out.h drive the caller's atlas write loops
	// (RenderGlyphDWrite: for y < out.h ... for x < out.w ...). Without
	// them the caller reads garbage from the stack-initialized ChsDWOut
	// and writes nothing (or writes out of bounds) - glyphs never reach
	// the atlas even though the read-back buffer is non-empty.
	out.w = w;
	out.h = h;

	// The black box can still extend OUTSIDE the DIB (left/top negative for
	// negative lsb / tall glyphs, right/bottom beyond the cell), so every
	// pixel is range-checked against the DIB: the glyph only drew inside the
	// 64x64 DIB, everything outside is transparent (coverage 0). Negative
	// offsets must never be used as DIB indexes - that is an out-of-bounds
	// read (this was a real crash before the check).
	const BYTE *px = (const BYTE *)ds.dsBm.bmBits;
	const int stride = ds.dsBm.bmWidth * 4; // bytes per row
	const int dibW = ds.dsBm.bmWidth;
	// CreateBitmapRenderTarget makes a TOP-DOWN DIB; GetObject then reports
	// dsBm.bmHeight as NEGATIVE. bmBits points at the first (top) row either
	// way, so the row math below is identical - only the height must be
	// absolutized, otherwise every dibY >= dibH test (e.g. 4 >= -64) holds
	// and the whole bitmap reads back as all-zero alpha (invisible glyphs).
	const int dibH = ds.dsBm.bmHeight < 0 ? -ds.dsBm.bmHeight : ds.dsBm.bmHeight;
	alphaOut.assign((size_t)w * h, 0);
	for(int y = 0; y < h; y++) {
		int dibY = bbox.top + y;
		if(dibY < 0 || dibY >= dibH)
			continue; // outside the DIB: no coverage
		for(int x = 0; x < w; x++) {
			int dibX = bbox.left + x;
			if(dibX < 0 || dibX >= dibW)
				continue; // outside the DIB: no coverage
			const BYTE *p = px + (size_t)dibY * stride + (size_t)dibX * 4;
			// Coverage: some implementations write white-on-black (R=G=B=coverage,
			// A=0 or 255), others write alpha-only (RGB=0, A=coverage). Taking the
			// max of R and A reads the coverage either way - reading only R can
			// produce all-zero glyphs (invisible text) on the latter layout.
			unsigned rv = p[2];
			unsigned av = p[3];
			alphaOut[(size_t)y * w + x] = (uint8)(rv > av ? rv : av);
		}
	}

	// Diagnose: count non-zero coverage pixels. A successful DrawGlyphRun
	// with an all-zero readback means the glyph was rasterized as empty
	// (rendering mode / params issue) rather than placement issue.
	int nonzero = 0;
	int maxv = 0;
	for(size_t i = 0; i < alphaOut.size(); i++) {
		if(alphaOut[i] != 0) {
			nonzero++;
			if(alphaOut[i] > maxv)
				maxv = alphaOut[i];
		}
	}
	DwLog("[DwReadDIB] set %d box %dx%d nonzero=%d max=%d of %d", slot,
	      w, h, nonzero, maxv, (int)alphaOut.size());
	return true;
}

bool ChsDwRenderCluster(const uint32 *cps, int count, bool slant, int emPx,
                        int baseY, ChsDWOut &out, std::vector<uint8> &alphaOut,
                        std::vector<uint8> &colorOut)
{
	alphaOut.clear();
	colorOut.clear();
	if(!sOk || cps == nil || count < 2 || count > 8)
		return false;

	int slot = slant ? 1 : 0;
	IDWriteFontFace *face = sFace[slot];
	IDWriteBitmapRenderTarget *rt = sRT[slot];
	if(face == nil || rt == nil)
		return false;

	// Every codepoint must exist in the face; a missing glyph would shape
	// the run to .notdef and wreck the composition. A CJK-only main face
	// has no emoji at all, so when anything is missing, fall back to the
	// rare chain first (leading codepoint wins, same per-codepoint first-hit
	// semantic as the single-glyph path) and use that face for the whole
	// run if it contains every codepoint.
	UINT16 indices[8];
	ZeroMemory(indices, sizeof(indices));
	face->GetGlyphIndices(cps, (UINT32)count, indices);
	bool allPresent = true;
	for(int i = 0; i < count; i++) {
		if(indices[i] == 0) {
			allPresent = false;
			break;
		}
	}
	if(!allPresent) {
		IDWriteFontFace *alt = ChsDwPickRareFace(cps[0]);
		if(alt) {
			UINT16 ai[8];
			ZeroMemory(ai, sizeof(ai));
			alt->GetGlyphIndices(cps, (UINT32)count, ai);
			bool altOk = true;
			for(int i = 0; i < count; i++) {
				if(ai[i] == 0) {
					altOk = false;
					break;
				}
			}
			if(altOk) {
				face = alt;
				rt = sRT[0]; // composed emoji always render on the normal set
				memcpy(indices, ai, sizeof(UINT16) * (size_t)count);
				DwLog("[DwCluster] %d cps via rare chain", count);
			}
		}
	}
	for(int i = 0; i < count; i++) {
		if(indices[i] == 0) {
			out.missing = true;
			DwLog("[DwCluster] U+%04X missing, no ZWJ composition", (unsigned)cps[i]);
			return false;
		}
	}
	out.missing = false;

	DWRITE_FONT_METRICS fm;
	ZeroMemory(&fm, sizeof(fm));
	face->GetMetrics(&fm);
	if(fm.designUnitsPerEm <= 0)
		return false;
	float scale = (float)emPx / (float)fm.designUnitsPerEm;

	// Per-glyph advances; the cluster advance is the sum (~2 em for a
	// two-person emoji), so the caller can advance the pen accordingly.
	DWRITE_GLYPH_METRICS dm[8];
	ZeroMemory(dm, sizeof(dm));
	float advances[8];
	float totalAdv = 0.0f;
	for(int i = 0; i < count; i++) {
		if(FAILED(face->GetDesignGlyphMetrics(&indices[i], 1, &dm[i], FALSE)))
			return false;
		advances[i] = (float)dm[i].advanceWidth * scale;
		totalAdv += advances[i];
	}
	out.advance = (int)totalAdv;
	if(out.advance <= 0)
		out.advance = CHS_DW_CELL * 2;

	DWRITE_GLYPH_RUN run;
	ZeroMemory(&run, sizeof(run));
	run.fontFace = face;
	run.fontEmSize = (FLOAT)emPx;
	run.glyphCount = (UINT32)count;
	run.glyphIndices = indices;
	run.glyphAdvances = advances;
	run.isSideways = FALSE;
	run.bidiLevel = 0;

	// Clear the DIB so the read-back sees a black background (same block
	// as the single-glyph path; the DIB is top-down so bmHeight is
	// negative - absolutize before sizing the clear).
	HDC dc = rt->GetMemoryDC();
	if(dc) {
		HGDIOBJ hbmp = GetCurrentObject(dc, OBJ_BITMAP);
		if(hbmp) {
			DIBSECTION ds;
			ZeroMemory(&ds, sizeof(ds));
			if(GetObject(hbmp, sizeof(ds), &ds) != 0 && ds.dsBm.bmBits) {
				int bh = ds.dsBm.bmHeight;
				if(bh < 0)
					bh = -bh;
				ZeroMemory(ds.dsBm.bmBits,
				           (size_t)ds.dsBm.bmWidth * (size_t)bh * 4);
			}
		}
	}

	// Draw the whole run at the cell baseline; DWrite performs the ZWJ
	// shaping internally and the reported black box covers the composed
	// glyph (relative to the cell top-left).
	RECT bbox;
	ZeroMemory(&bbox, sizeof(bbox));
	HRESULT hr = rt->DrawGlyphRun(0.0f, (FLOAT)baseY, DWRITE_MEASURING_MODE_GDI_NATURAL,
	                              &run, sParams, RGB(255, 255, 255), &bbox);
	if(FAILED(hr))
		return false;
	DwLog("[DwCluster] %d cps bbox L%d T%d R%d B%d adv=%d", count,
	      bbox.left, bbox.top, bbox.right, bbox.bottom, out.advance);

	if(!ChsDwReadDIB(slot, out, bbox, alphaOut))
		return false;
	if(alphaOut.empty())
		return false;
	return true;
}

bool ChsDwRender(uint32 cp, bool slant, int emPx, int baseY,
                 ChsDWOut &out, std::vector<uint8> &alphaOut,
                 std::vector<uint8> &colorOut)
{
	alphaOut.clear();
	colorOut.clear();
	if(!sOk)
		return false;

	int slot = slant ? 1 : 0;
	IDWriteFontFace *face = sFace[slot];
	IDWriteBitmapRenderTarget *rt = sRT[slot];
	// Supplementary-plane codepoints use the RareFontFile chain (the main
	// faces - Microsoft YaHei etc. - rarely contain CJK ext / emoji).
	bool isRare = cp > 0xFFFF;
	if(isRare) {
		face = ChsDwPickRareFace(cp);
		rt = sRT[0]; // rare faces are always normal (no slant pair)
	}
	if(face == nil || rt == nil)
		return false;

	// Look up the glyph index (0 = codepoint absent in this face, same
	// semantic as GDI returning GDI_ERROR / no black box).
	UINT16 glyph = 0;
	face->GetGlyphIndices(&cp, 1, &glyph);
	if(glyph == 0 && !isRare) {
		// BMP codepoint missing in the main face (e.g. Hangul with a
		// Simplified-Chinese-only font): fall back to the rare chain too,
		// so the user can list a Hangul font (Noto Sans KR, Malgun
		// Gothic) in RareFontFile. Same per-codepoint first-hit semantic.
		IDWriteFontFace *alt = ChsDwPickRareFace(cp);
		if(alt) {
			face = alt;
			rt = sRT[0];
			alt->GetGlyphIndices(&cp, 1, &glyph);
		}
	}
	if(glyph == 0) {
		out.missing = true;
		DwLog("[DwRender] U+%04X:%d glyph index == 0 (missing)", (unsigned)cp, slot);
		return false;
	}
	out.missing = false;

	// Font design metrics -> pixel scale (design units per em).
	DWRITE_FONT_METRICS fm;
	ZeroMemory(&fm, sizeof(fm));
	face->GetMetrics(&fm);
	if(fm.designUnitsPerEm <= 0) {
		DwLog("[DwRender] U+%04X:%d bad designUnitsPerEm %u", (unsigned)cp, slot, fm.designUnitsPerEm);
		return false;
	}
	float scale = (float)emPx / (float)fm.designUnitsPerEm;

	// Advance width in pixels (same meaning as GDI gmCellIncX).
	DWRITE_GLYPH_METRICS dm;
	ZeroMemory(&dm, sizeof(dm));
	face->GetDesignGlyphMetrics(&glyph, 1, &dm, FALSE);
	out.advance = (int)((float)dm.advanceWidth * scale);
	if(out.advance <= 0)
		out.advance = CHS_DW_CELL; // defensive: full cell, same as GDI

	// Clear the DIB so the coverage read-back sees black background.
	// NOTE: the DIB is top-down, so dsBm.bmHeight is negative; absolutize it
	// before sizing the clear (a negative size converts to a huge size_t and
	// ZeroMemory would scribble way past the bitmap).
	HDC dc = rt->GetMemoryDC();
	if(dc) {
		HGDIOBJ hbmp = GetCurrentObject(dc, OBJ_BITMAP);
		if(hbmp) {
			DIBSECTION ds;
			ZeroMemory(&ds, sizeof(ds));
			if(GetObject(hbmp, sizeof(ds), &ds) != 0 && ds.dsBm.bmBits) {
				int bh = ds.dsBm.bmHeight;
				if(bh < 0)
					bh = -bh;
				ZeroMemory(ds.dsBm.bmBits,
				           (size_t)ds.dsBm.bmWidth * (size_t)bh * 4);
			}
		}
	}

	// Build a single glyph run for this face/glyph; used by both the gray
	// path and (per layer) the color path below.
	DWRITE_GLYPH_RUN run;
	ZeroMemory(&run, sizeof(run));
	run.fontFace = face;
	run.fontEmSize = (FLOAT)emPx;
	run.glyphCount = 1;
	run.glyphIndices = &glyph;
	run.isSideways = FALSE;
	run.bidiLevel = 0;

	// Detect color glyphs (COLR/CPAL, e.g. emoji). When the face is a color
	// font, render every color layer separately and composite the RGBA
	// result. Otherwise the regular gray-coverage path is used.
	IDWriteFontFace2 *f2 = nil;
	bool isColor = false;
	if(SUCCEEDED(face->QueryInterface(__uuidof(IDWriteFontFace2), (void **)&f2)) && f2) {
		isColor = f2->IsColorFont() != FALSE;
		f2->Release();
	}
	if(isColor)
		return ChsDwRenderColor(cp, slot, baseY, run, emPx, out, colorOut);

	// ----- gray path -----
	// Draw the glyph with the baseline origin at (0, baseY): the glyph body
	// then lands INSIDE the DIB (the caller's cell baseline = gBaseY[set],
	// same as the GDI/stb paths). The reported black box is relative to the
	// DIB top-left corner, i.e. the cell top-left, so the caller can place
	// it directly into the atlas cell.
	RECT bbox;
	ZeroMemory(&bbox, sizeof(bbox));
	HRESULT hr = rt->DrawGlyphRun(0.0f, (FLOAT)baseY, DWRITE_MEASURING_MODE_GDI_NATURAL,
	                              &run, sParams, RGB(255, 255, 255), &bbox);
	if(FAILED(hr)) {
		DwLog("[DwRender] U+%04X:%d DrawGlyphRun FAILED hr=0x%08X", (unsigned)cp, slot, (unsigned)hr);
		return false;
	}
	DwLog("[DwRender] U+%04X:%d glyph=%d bbox L%d T%d R%d B%d", (unsigned)cp, slot,
	      (int)glyph, bbox.left, bbox.top, bbox.right, bbox.bottom);

	// Read back the coverage bitmap. out.left/top come straight from the
	// reported black box (relative to the cell top-left), matching the
	// caller's placement convention.
	if(!ChsDwReadDIB(slot, out, bbox, alphaOut)) {
		DwLog("[DwRender] U+%04X:%d ReadDIB failed", (unsigned)cp, slot);
		return false;
	}
	if(alphaOut.empty()) {
		DwLog("[DwRender] U+%04X:%d empty alpha output", (unsigned)cp, slot);
		return false;
	}
	return true;
}

// Composite a color glyph (COLR/CPAL) into colorOut (w*h*4 RGBA bytes).
// Every layer is rasterized into the DIB with the gray params (each layer
// is a normal glyph run in the same face, drawn at the same baseline), read
// back as coverage, then multiplied by the layer's color
// (DWRITE_COLOR_GLYPH_RUN.runColor) and alpha-composited (source-over)
// into an RGBA buffer. The black box reported to the caller is the union of
// all layer boxes. COLR layers are ordered back-to-front, so the composite
// order is correct. Returns false on any hard failure (the caller then
// falls back to the gray path / stb as usual).
static bool
ChsDwRenderColor(uint32 cp, int slot, int baseY, const DWRITE_GLYPH_RUN &run,
                 int emPx, ChsDWOut &out, std::vector<uint8> &colorOut)
{
	// TranslateColorGlyphRun lives on IDWriteFactory2 (dwrite_2.h).
	IDWriteFactory2 *f2 = nil;
	if(FAILED(sFactory->QueryInterface(__uuidof(IDWriteFactory2), (void **)&f2)) || f2 == nil) {
		DwLog("[DwRenderColor] U+%04X: IDWriteFactory2 unavailable - gray fallback", (unsigned)cp);
		return false;
	}
	IDWriteColorGlyphRunEnumerator *layers = nil;
	HRESULT hr = f2->TranslateColorGlyphRun(0.0f, (FLOAT)baseY, &run, nil,
	                                        DWRITE_MEASURING_MODE_GDI_NATURAL,
	                                        nil, 0, &layers);
	f2->Release();
	if(FAILED(hr) || layers == nil) {
		DwLog("[DwRenderColor] U+%04X: TranslateColorGlyphRun failed hr=0x%08X - gray fallback",
		      (unsigned)cp, (unsigned)hr);
		return false;
	}

	// One enumeration pass: for each layer, clear the DIB, draw the layer,
	// read back its coverage, and keep {color, box, alpha}. The union box is
	// accumulated at the same time.
	struct ChsColorLayer {
		DWRITE_COLOR_F color;
		RECT box;
		std::vector<uint8> alpha;
	};
	std::vector<ChsColorLayer> layerList;
	RECT unionBox;
	bool haveUnion = false;

	IDWriteBitmapRenderTarget *rt = sRT[slot];
	for(;;) {
		BOOL hasRun = FALSE;
		HRESULT ehr = layers->MoveNext(&hasRun);
		if(FAILED(ehr) || !hasRun)
			break; // enumeration done or error
		const DWRITE_COLOR_GLYPH_RUN *lg = nil;
		if(FAILED(layers->GetCurrentRun(&lg)) || lg == nil)
			break;

		// Clear the DIB before each layer so the read-back is coverage only.
		HDC dc = rt->GetMemoryDC();
		if(dc) {
			HGDIOBJ hbmp = GetCurrentObject(dc, OBJ_BITMAP);
			if(hbmp) {
				DIBSECTION ds;
				ZeroMemory(&ds, sizeof(ds));
				if(GetObject(hbmp, sizeof(ds), &ds) != 0 && ds.dsBm.bmBits) {
					int bh = ds.dsBm.bmHeight;
					if(bh < 0) bh = -bh;
					ZeroMemory(ds.dsBm.bmBits, (size_t)ds.dsBm.bmWidth * (size_t)bh * 4);
				}
			}
		}
		RECT lbbox;
		ZeroMemory(&lbbox, sizeof(lbbox));
		HRESULT dhr = rt->DrawGlyphRun(lg->baselineOriginX, lg->baselineOriginY,
		                               DWRITE_MEASURING_MODE_GDI_NATURAL,
		                               &lg->glyphRun, sParams, RGB(255,255,255), &lbbox);
		if(FAILED(dhr))
			continue; // skip this layer
		ChsDWOut lout;
		std::vector<uint8> lalpha;
		if(!ChsDwReadDIB(slot, lout, lbbox, lalpha))
			continue;

		ChsColorLayer lay;
		lay.color = lg->runColor;
		lay.box = lbbox;
		lay.alpha.swap(lalpha);
		layerList.push_back(lay);
		if(!haveUnion) {
			unionBox = lbbox;
			haveUnion = true;
		} else {
			if(lbbox.left   < unionBox.left)   unionBox.left   = lbbox.left;
			if(lbbox.top    < unionBox.top)    unionBox.top    = lbbox.top;
			if(lbbox.right  > unionBox.right)  unionBox.right  = lbbox.right;
			if(lbbox.bottom > unionBox.bottom) unionBox.bottom = lbbox.bottom;
		}
	}
	layers->Release();

	if(!haveUnion) {
		DwLog("[DwRenderColor] U+%04X: no color layers rasterized", (unsigned)cp);
		return false;
	}
	if(layerList.empty()) {
		DwLog("[DwRenderColor] U+%04X: no layers captured", (unsigned)cp);
		return false;
	}

	// Placement: clip the box to the DIB (cell) before reporting it. The
	// raw union can have negative left/top (glyph overhang past the cell
	// top-left) or right/bottom beyond 64; reporting those makes the
	// caller centering math shove the visible body and clip the right
	// edge. out.* now describes ONLY the pixels actually drawn in the
	// DIB, so the caller can center it safely (right edge never exceeds
	// the cell when out.w is at most 64 and out.left is at least 0).
	int ul2 = unionBox.left;  if(ul2 < 0) ul2 = 0;
	int ut2 = unionBox.top;   if(ut2 < 0) ut2 = 0;
	int ur2 = unionBox.right; if(ur2 > CHS_DW_CELL) ur2 = CHS_DW_CELL;
	int ub2 = unionBox.bottom; if(ub2 > CHS_DW_CELL) ub2 = CHS_DW_CELL;
	out.left = ul2;
	out.top = ut2;
	out.w = ur2 - ul2;
	out.h = ub2 - ut2;
	// Advance: not exposed per color layer; use the face's design metrics
	// (same math as the gray path uses for regular glyphs).
	{
		DWRITE_FONT_METRICS fm;
		ZeroMemory(&fm, sizeof(fm));
		run.fontFace->GetMetrics(&fm);
		float scale = fm.designUnitsPerEm > 0
			? (float)emPx / (float)fm.designUnitsPerEm : 1.0f;
		DWRITE_GLYPH_METRICS dm;
		ZeroMemory(&dm, sizeof(dm));
		// glyph index of the first layer: run.glyphIndices[0]
		run.fontFace->GetDesignGlyphMetrics(run.glyphIndices, 1, &dm, FALSE);
		out.advance = (int)((float)dm.advanceWidth * scale);
	}
	if(out.advance <= 0)
		out.advance = CHS_DW_CELL;
	out.isColor = true;
	if(out.w <= 0 || out.h <= 0) { // oversized union allowed: caller shrinks em and re-renders
		DwLog("[DwRenderColor] U+%04X: bad union box %dx%d", (unsigned)cp, out.w, out.h);
		return false;
	}

	// Composite: each layer's coverage * color, source-over into RGBA.
	colorOut.assign((size_t)out.w * out.h * 4, 0);
	const int uw = out.w, uh = out.h;
	const int ul = ul2, ut = ut2; // CLIPPED origin: layer pixels map 1:1 into the clipped buffer (right edge kept)
	for(const ChsColorLayer &lay : layerList) {
		const int lw = lay.box.right - lay.box.left;
		const int lh = lay.box.bottom - lay.box.top;
		const float sr = lay.color.r, sg = lay.color.g, sb = lay.color.b;
		for(int y = 0; y < lh; y++) {
			int py = lay.box.top + y - ut;
			if(py < 0 || py >= uh) continue;
			for(int x = 0; x < lw; x++) {
				int px = lay.box.left + x - ul;
				if(px < 0 || px >= uw) continue;
				unsigned a = lay.alpha[(size_t)y * lw + x];
				if(a == 0) continue;
				uint8 *dst = colorOut.data() + ((size_t)py * uw + px) * 4;
				float sa = a / 255.0f;
				float dr = dst[0] / 255.0f, dg = dst[1] / 255.0f, db = dst[2] / 255.0f, da = dst[3] / 255.0f;
				float oa = sa + da * (1.0f - sa);
				if(oa > 0.0f) {
					float cr = (sr * sa + dr * da * (1.0f - sa)) / oa;
					float cg = (sg * sa + dg * da * (1.0f - sa)) / oa;
					float cb = (sb * sa + db * da * (1.0f - sa)) / oa;
					dst[0] = (uint8)(cr * 255.0f + 0.5f);
					dst[1] = (uint8)(cg * 255.0f + 0.5f);
					dst[2] = (uint8)(cb * 255.0f + 0.5f);
					dst[3] = (uint8)(oa * 255.0f + 0.5f);
				}
			}
		}
	}

	DwLog("[DwRenderColor] U+%04X: color glyph %dx%d union L%d T%d R%d B%d",
	      (unsigned)cp, out.w, out.h, unionBox.left, unionBox.top, unionBox.right, unionBox.bottom);
	return true;
}

#endif // _WIN32
