#pragma once

// Shared glyph-slot descriptor used by the Chinese font pipeline.
// page selects the atlas texture, rowIndex/columnIndex the 64x64 grid cell.
struct CharPos
{
	unsigned char page;
	unsigned char rowIndex;
	unsigned char columnIndex;
};

#ifdef _WIN32
#include "Sprite2d.h"

// Two glyph sets, mirroring the original CHINESE.TXD normal/slant pair:
// - normal (FONT_STANDARD/HEADING): regular bold CJK font
// - slant  (FONT_BANK): cursive font loaded from models\YingZhangXingShu.ttf
// Each set has its own pages; page numbers are offset so a CharPos encodes
// the set: page < CHS_SLANT_BASE means normal, page >= CHS_SLANT_BASE slant.
#define CHS_PAGES_PER_SET 16
#define CHS_SLANT_BASE    CHS_PAGES_PER_SET
#define CHS_MAX_PAGES     (CHS_PAGES_PER_SET * 2)

// Runtime-generated Chinese font atlas.
//
// Glyphs are rasterized on demand from fonts (Windows GDI, GetGlyphOutlineW)
// into pages of 64x64 grid cells (each page is a 4096x4096 RGBA texture,
// 4096 glyphs). This never runs out of glyphs as long as the font contains
// the codepoint, so there is no 4096-glyph limit like the static
// CHINESE.TXD approach.
class CHSFont
{
public:
	static CSprite2d SpriteC[CHS_MAX_PAGES];
	static int NumPages;

	static bool Init(void);      // create fonts + first pages; rebuild if already inited
	static void Shutdown(void);  // release everything (idempotent)
	static bool Inited(void);
	// True when [Fonts] TextRenderer selects GDI or DirectWrite.  The
	// language menu uses this before Init() so dynamic fonts are not hidden
	// merely because the legacy Chinese.dat/Chinese.txd pair is absent.
	static bool UsesDynamicRenderer(void);
	// Ensure the [Fonts] section exists in re3.ini (write defaults if
	// missing; fail hard if it cannot be ensured). Callable at any time,
	// independent of Init/Inited.
	static void EnsureConfig(void);
	// cp is a codepoint: BMP 0-0xFFFF direct, supplementary (CJK ext/Emoji) = merged surrogate value.
	static const CharPos &GetSlot(uint32 cp, bool slant); // query, rasterize on first use
	// Populate the normal atlas while a loading screen is active. This moves
	// the expensive first-use rasterization out of subtitle presentation.
	static void Preload(const wchar *textData, int textLength);
	// Whether the glyph in the given atlas slot is a COLOR glyph (COLR/CPAL
	// emoji). The batch renderer uses this to skip vertex-color modulation
	// for that cell so the emoji shows its real colors. cp/slant identify the
	// slot the same way GetSlot does (the slot must already exist).
	static bool IsSlotColor(uint32 cp, bool slant);
};
#endif
