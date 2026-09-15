#include "common.h"

#include "Sprite2d.h"
#include "TxdStore.h"
#include "Font.h"
#include "Timer.h"
#include "Frontend.h"
#ifdef BUTTON_ICONS
#include "FileMgr.h"
#endif
#ifdef CHINESE
#include "Game.h"
#include "CHSFont.h"
#include "Text.h"
#include <vector>
#endif

#ifdef CHINESE
static CSprite2d SpriteChinese[FONT_CHN_MAX];
static const float CHS_DISPLAY_SCALE = 1.10f;
// GTA III's FONT_BANK quad is 20 units high. Slanted vehicle/zone names use
// that font, so make their square CJK glyphs match its native on-screen height
// instead of inheriting the smaller 16-unit general CJK cell.
static const float CHS_BANK_DISPLAY_SCALE = 20.0f / 16.0f;

// reVC accumulates glyph quads and submits one draw per atlas texture instead
// of drawing every character immediately. Keep shadow and main passes separate
// so every shadow is behind every foreground glyph, matching its font buffer.
struct ChsBatchItem
{
	CSprite2d *sprite;
	CRect rect;
	CRGBA color;
	float u0, v0, u1, v1;
	float shear;
};

static std::vector<ChsBatchItem> gChsShadowBatch;
static std::vector<ChsBatchItem> gChsMainBatch;
static RwIm2DVertex gChsBatchVertices[6 * 64];

static void
RenderChsBatchItems(const std::vector<ChsBatchItem> &items)
{
	for(size_t seed = 0; seed < items.size(); seed++) {
		CSprite2d *sprite = items[seed].sprite;
		bool rendered = false;
		for(size_t previous = 0; previous < seed; previous++) {
			if(items[previous].sprite == sprite) {
				rendered = true;
				break;
			}
		}
		if(rendered)
			continue;

		sprite->SetRenderState();
		int32 numVertices = 0;
		for(size_t i = seed; i < items.size(); i++) {
			const ChsBatchItem &item = items[i];
			if(item.sprite != sprite)
				continue;
			CSprite2d::SetVertices(&gChsBatchVertices[numVertices], item.rect,
				item.color, item.color, item.color, item.color,
				item.u0, item.v0, item.u1, item.v0,
				item.u1, item.v1, item.u0, item.v1);
			// FONT_BANK already contains slanted Latin glyphs. Dynamic CJK uses the
			// normal system face, so reproduce that lean geometrically. Anchor the
			// top edge to preserve right justification and move the lower edge left;
			// the amount scales with the actual on-screen glyph height.
			if(item.shear != 0.0f) {
				RwIm2DVertexSetScreenX(&gChsBatchVertices[numVertices + 1], item.rect.left - item.shear);
				RwIm2DVertexSetScreenX(&gChsBatchVertices[numVertices + 2], item.rect.right - item.shear);
				RwIm2DVertexSetScreenX(&gChsBatchVertices[numVertices + 4], item.rect.right - item.shear);
			}
			numVertices += 6;
			if(numVertices == ARRAY_SIZE(gChsBatchVertices)) {
				RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, gChsBatchVertices, numVertices);
				numVertices = 0;
			}
		}
		if(numVertices != 0)
			RwIm2DRenderPrimitive(rwPRIMTYPETRILIST, gChsBatchVertices, numVertices);
	}
}

static void
RenderChsBatches(void)
{
	if(gChsShadowBatch.empty() && gChsMainBatch.empty())
		return;
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE);
	RwRenderStateSet(rwRENDERSTATETEXTUREFILTER, (void *)rwFILTERLINEAR);
	RenderChsBatchItems(gChsShadowBatch);
	RenderChsBatchItems(gChsMainBatch);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)FALSE);
	gChsShadowBatch.clear();
	gChsMainBatch.clear();
}

static CharPos sTable[0x10000];
static const CharPos sFallbackCharPos = { 0, 63, 63 };

static bool
IsZeroWidthChineseChar(wchar c)
{
	return c == 0x200C || c == 0x200D || (c >= 0xFE00 && c <= 0xFE0F);
}

static uint32
DecodeChineseCodepoint(const wchar *text, int &units)
{
	uint32 first = (uint16)text[0];
	units = 1;
	if (first >= 0xD800 && first <= 0xDBFF) {
		uint32 second = (uint16)text[1];
		if (second >= 0xDC00 && second <= 0xDFFF) {
			units = 2;
			return 0x10000 + ((first - 0xD800) << 10) + (second - 0xDC00);
		}
	}
	return first;
}

static const CharPos &
GetCharPos(uint32 chr, bool slant)
{
	#ifdef _WIN32
	if (CHSFont::Inited())
		return CHSFont::GetSlot(chr, slant);
	#endif
	if (chr >= 0x10000)
		return sFallbackCharPos;
	return sTable[chr];
}

static bool
ReadTable(void)
{
	for (int i = 0; i < 0x10000; i++)
		sTable[i] = sFallbackCharPos;
	int hfile = CFileMgr::OpenFile("data/Chinese.dat", "rb");
	if (hfile) {
		struct LegacyCharPos { uint8 rowIndex, columnIndex; };
		LegacyCharPos legacyTable[0x10000];
		if (CFileMgr::Read(hfile, (char *)legacyTable, sizeof(legacyTable)) != sizeof(legacyTable)) {
			CFileMgr::CloseFile(hfile);
			return false;
		}
		for (int i = 0; i < 0x10000; i++) {
			sTable[i].page = 0;
			sTable[i].rowIndex = legacyTable[i].rowIndex;
			sTable[i].columnIndex = legacyTable[i].columnIndex;
		}
		CFileMgr::CloseFile(hfile);
		return true;
	}
	return false;
}

static bool IsPunctuation(wchar c)
{
	return (c == ' ' || c == ',' || c == '.' || c == '!' || c == '?' || c == ':' || c == ';' || c == '"' || c == '\'');
}

static wchar FindNewCharacter(wchar c)
{
	return c;
}

static const float FontSizeIncrementers[MAX_FONTS] = { 0.0f, 0.0f, 0.0f };
#endif

void
AsciiToUnicode(const char *src, wchar *dst)
{
	while((*dst++ = (unsigned char)*src++) != '\0');
}

void
UnicodeStrcat(wchar *dst, wchar *append)
{
	UnicodeStrcpy(&dst[UnicodeStrlen(dst)], append);
}

void
UnicodeStrcpy(wchar *dst, const wchar *src)
{
	while((*dst++ = *src++) != '\0');
}

int
UnicodeStrlen(const wchar *str)
{
	int len;
	for(len = 0; *str != '\0'; len++, str++);
	return len;
}

CFontDetails CFont::Details;
bool16 CFont::NewLine;
CSprite2d CFont::Sprite[MAX_FONTS];

#ifdef CHINESE
int32 CFont::chineseSlot = -1;
static bool gChineseFontSystemInitialised = false;
static bool gChineseFontsLoaded = false;
static bool gStaticChineseFontsLoaded = false;
static int16 gChineseFontStyleOverride = -1;
static float gChineseSyntheticSlant = 0.0f;
#endif

#ifdef MORE_LANGUAGES
uint8 CFont::LanguageSet = FONT_LANGSET_EFIGS;
int32 CFont::Slot = -1;
#define JAP_TERMINATION (0x8000 | '~')

int16 CFont::Size[LANGSET_MAX][MAX_FONTS][193] = {
	{
#else
int16 CFont::Size[MAX_FONTS][193] = {
#endif

#if !defined(GTA_PS2) || defined(FIX_BUGS)
		{
		13, 12, 31, 35, 23, 35, 31,  9, 14, 15, 25, 30, 11, 17, 13, 31,
		23, 16, 22, 21, 24, 23, 23, 20, 23, 22, 10, 35, 26, 26, 26, 26,
		30, 26, 24, 23, 24, 22, 21, 24, 26, 10, 20, 26, 22, 29, 26, 25,
		23, 25, 24, 24, 22, 25, 24, 29, 29, 23, 25, 37, 22, 37, 35, 37,
		35, 21, 22, 21, 21, 22, 13, 22, 21, 10, 16, 22, 11, 32, 21, 21,
		23, 22, 16, 20, 14, 21, 20, 30, 25, 21, 21, 33, 33, 33, 33, 35,
		27, 27, 27, 27, 32, 24, 23, 23, 23, 23, 11, 11, 11, 11, 26, 26,
		26, 26, 26, 26, 26, 25, 26, 21, 21, 21, 21, 32, 23, 22, 22, 22,
		22, 11, 11, 11, 11, 22, 22, 22, 22, 22, 22, 22, 22, 26, 21, 24,
		12, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
		26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 18, 26, 26,
		26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
		20
		},

		{
		13,  9, 21, 35, 23, 35, 35, 11, 35, 35, 25, 35, 11, 17, 13, 33,
		28, 14, 22, 21, 24, 23, 23, 21, 23, 22, 10, 35, 13, 35, 13, 33,
		 5, 25, 22, 23, 24, 21, 21, 24, 24,  9, 20, 24, 21, 27, 25, 25,
		22, 25, 23, 20, 23, 23, 23, 31, 23, 23, 23, 37, 33, 37, 35, 37,
		35, 21, 19, 19, 21, 19, 17, 21, 21,  8, 17, 18, 14, 24, 21, 21,
		20, 22, 19, 20, 20, 19, 20, 26, 21, 20, 21, 33, 33, 33, 33, 35,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		16
		},

		{
		15, 14, 16, 25, 19, 26, 22, 11, 18, 18, 27, 26, 13, 19,  9, 27,
		19, 18, 19, 19, 22, 19, 20, 18, 19, 20, 12, 32, 15, 32, 15, 35,
		15, 19, 19, 19, 19, 19, 16, 19, 20,  9, 19, 20, 14, 29, 19, 20,
		19, 19, 19, 19, 21, 19, 20, 32, 20, 19, 19, 33, 31, 39, 37, 39,
		37, 21, 21, 21, 23, 21, 19, 23, 23, 10, 19, 20, 16, 26, 23, 23,
		20, 20, 20, 22, 21, 22, 22, 26, 22, 22, 23, 35, 35, 35, 35, 37,
		19, 19, 19, 19, 29, 19, 19, 19, 19, 19,  9,  9,  9,  9, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 30, 19, 19, 19, 19,
		19, 10, 10, 10, 10, 19, 19, 19, 19, 19, 19, 19, 19, 19, 23, 35,
		12, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 11, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19
		}
#else
		{
		13, 12, 31, 35, 23, 35, 31,  9, 14, 15, 25, 30, 11, 17, 13, 31,
		23, 16, 22, 21, 24, 23, 23, 20, 23, 22, 10, 35, 26, 26, 26, 26,
		30, 26, 24, 23, 24, 22, 21, 24, 26, 10, 20, 26, 22, 29, 26, 25,
		24, 25, 24, 24, 22, 25, 24, 29, 29, 23, 25, 37, 22, 37, 35, 37,
		35, 21, 22, 21, 21, 22, 13, 22, 21, 10, 16, 22, 11, 32, 21, 21,
		23, 22, 16, 20, 14, 21, 20, 30, 25, 21, 21, 33, 33, 33, 33, 35,
		27, 27, 27, 27, 32, 24, 23, 23, 23, 23, 11, 11, 11, 11, 26, 26,
		26, 26, 26, 26, 26, 25, 26, 21, 21, 21, 21, 32, 23, 22, 22, 22,
		22, 11, 11, 11, 11, 22, 22, 22, 22, 22, 22, 22, 22, 26, 21, 24,
		12, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
		26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 18, 26, 26,
		26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
		20
		},

		{
		13,  9, 21, 35, 23, 35, 35, 11, 35, 35, 25, 35, 11, 17, 13, 33,
		28, 14, 22, 21, 24, 23, 23, 21, 23, 22, 10, 35, 13, 35, 13, 33,
		 5, 25, 22, 23, 24, 21, 21, 24, 24,  9, 20, 24, 21, 27, 25, 25,
		22, 25, 23, 20, 23, 23, 23, 31, 23, 23, 23, 37, 33, 37, 35, 37,
		35, 21, 19, 19, 21, 19, 17, 21, 21,  8, 17, 18, 14, 24, 21, 21,
		20, 22, 19, 20, 20, 19, 20, 26, 21, 20, 21, 33, 33, 33, 33, 35,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		16
		},

		{
		15, 14, 16, 25, 19, 26, 22, 11, 18, 18, 27, 26, 13, 19,  9, 27,
		19, 18, 19, 19, 21, 19, 20, 18, 19, 20, 12, 32, 15, 32, 15, 35,
		15, 19, 19, 19, 19, 19, 16, 19, 20,  9, 19, 20, 14, 29, 19, 19,
		19, 19, 19, 19, 21, 19, 20, 32, 20, 19, 19, 33, 31, 39, 37, 39,
		37, 21, 21, 21, 23, 21, 19, 23, 23, 10, 19, 20, 16, 26, 23, 23,
		20, 20, 20, 22, 21, 22, 22, 26, 22, 22, 23, 35, 35, 35, 35, 37,
		19, 19, 19, 19, 29, 19, 19, 19, 19, 19,  9,  9,  9,  9, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 30, 19, 19, 19, 19, 19,
		10, 10, 10, 10, 19, 19, 19, 19, 19, 19, 19, 19, 19, 23, 35, 12,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 11, 19, 19, 19,
		19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
		19
		}
#endif

#ifdef MORE_LANGUAGES
	},
	{
		{ 13, 12, 31, 35, 23, 35, 31, 9, 14, 15, 25, 30, 11, 17,
			13, 31, 23, 16, 22, 21, 24, 23, 23, 20, 23, 22, 10,
			35, 26, 26, 26, 26, 30, 26, 24, 23, 24, 22, 21, 24,
			26, 10, 20, 26, 22, 29, 26, 25, 23, 25, 24, 24, 22,
			25, 24, 29, 29, 23, 25, 37, 22, 37, 35, 37, 35, 21,
			22, 21, 21, 22, 13, 22, 21, 10, 16, 22, 11, 32, 21,
			21, 23, 22, 16, 20, 14, 21, 20, 30, 25, 21, 21, 13,
			33, 13, 13, 13, 24, 22, 22, 19, 26, 21, 30, 20, 23,
			23, 21, 24, 26, 23, 22, 23, 21, 22, 20, 20, 26, 25,
			24, 22, 31, 32, 23, 30, 22, 22, 32, 23, 19, 18, 18,
			15, 22, 19, 27, 19, 20, 20, 18, 22, 24, 20, 19, 19,
			20, 19, 16, 19, 28, 20, 20, 18, 26, 27, 19, 26, 18,
			19, 27, 19, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
			26, 26, 26, 18, 26, 26, 26, 26, 26, 26, 26, 26, 26,
			26, 26, 26, 26, 26, 26, 26, 26, 26, 20 },
		{ 13, 9, 21, 35, 23, 35, 35, 11, 35, 35, 25, 35, 11,
			17, 13, 33, 28, 14, 22, 21, 24, 23, 23, 21, 23, 22,
			10, 35, 13, 35, 13, 33, 5, 25, 22, 23, 24, 21, 21, 24,
			24, 9, 20, 24, 21, 27, 25, 25, 22, 25, 23, 20, 23, 23,
			23, 31, 23, 23, 23, 37, 33, 37, 35, 37, 35, 21, 19,
			19, 21, 19, 17, 21, 21, 8, 17, 18, 14, 24, 21, 21, 20,
			22, 19, 20, 20, 19, 20, 26, 21, 20, 21, 33, 33, 33,
			33, 35, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 16, },
		{ 15, 14, 16, 25, 19,
			26, 22, 11, 18, 18, 27, 26, 13, 19, 9, 27, 19, 18, 19,
			19, 22, 19, 20, 18, 19, 20, 12, 32, 15, 32, 15, 35,
			15, 19, 19, 19, 19, 19, 16, 19, 20, 9, 19, 20, 14, 29,
			19, 20, 19, 19, 19, 19, 21, 19, 20, 32, 20, 19, 19,
			33, 31, 39, 37, 39, 37, 21, 21, 21, 23, 21, 19, 23, 23, 10, 19, 20, 16, 26, 23,
			21, 21, 20, 20, 22, 21, 22, 22, 26, 22, 22, 23, 35,
			35, 35, 35, 37, 19, 19, 19, 19, 19, 19, 29, 19, 19,
			19, 20, 22, 31, 19, 19, 19, 19, 19, 29, 19, 29, 19,
			21, 19, 30, 31, 21, 29, 19, 19, 29, 19, 21, 23, 32,
			21, 21, 30, 31, 22, 21, 32, 33, 23, 32, 21, 21, 32,
			21, 19, 19, 30, 31, 22, 22, 21, 32, 33, 23, 32, 21,
			21, 32, 21, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 11, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19 },
	},

	{
		{
			13, 12, 31, 35, 23, 35, 31, 9, 14, 15, 25, 30, 11, 17, 13, 31,
				23, 16, 22, 21, 24, 23, 23, 20, 23, 22, 10, 35, 26, 26, 26, 26,
				30, 26, 24, 23, 24, 22, 21, 24, 26, 10, 20, 26, 22, 29, 26, 25,
				23, 25, 24, 24, 22, 25, 24, 29, 29, 23, 25, 37, 22, 37, 35, 37,
				35, 21, 22, 21, 21, 22, 13, 22, 21, 10, 16, 22, 11, 32, 21, 21,
				23, 22, 16, 20, 14, 21, 20, 30, 25, 21, 21, 33, 33, 33, 33, 35,
				27, 27, 27, 27, 32, 24, 23, 23, 23, 23, 11, 11, 11, 11, 26, 26,
				26, 26, 26, 26, 26, 25, 26, 21, 21, 21, 21, 32, 23, 22, 22, 22,
				22, 11, 11, 11, 11, 22, 22, 22, 22, 22, 22, 22, 22, 26, 21, 24,
				12, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
				26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 18, 26, 26,
				26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26, 26,
				20
		},

		{
			13,  9, 21, 35, 23, 35, 35, 11, 35, 35, 25, 35, 11, 17, 13, 33,
			28, 14, 22, 21, 24, 23, 23, 21, 23, 22, 10, 35, 13, 35, 13, 33,
			5, 25, 22, 23, 24, 21, 21, 24, 24,  9, 20, 24, 21, 27, 25, 25,
			22, 25, 23, 20, 23, 23, 23, 31, 23, 23, 23, 37, 33, 37, 35, 37,
			35, 21, 19, 19, 21, 19, 17, 21, 21,  8, 17, 18, 14, 24, 21, 21,
			20, 22, 19, 20, 20, 19, 20, 26, 21, 20, 21, 33, 33, 33, 33, 35,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			16
		},

		{
			15, 14, 16, 25, 19, 26, 22, 11, 18, 18, 27, 26, 13, 19,  9, 27,
			19, 18, 19, 19, 22, 19, 20, 18, 19, 20, 12, 32, 15, 32, 15, 35,
			15, 19, 19, 19, 19, 19, 16, 19, 20,  9, 19, 20, 14, 29, 19, 20,
			19, 19, 19, 19, 21, 19, 20, 32, 20, 19, 19, 33, 31, 39, 37, 39,
			37, 21, 21, 21, 23, 21, 19, 23, 23, 10, 19, 20, 16, 26, 23, 23,
			20, 20, 20, 22, 21, 22, 22, 26, 22, 22, 23, 35, 35, 35, 35, 37,
			19, 19, 19, 19, 29, 19, 19, 19, 19, 19,  9,  9,  9,  9, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 30, 19, 19, 19, 19,
			19, 10, 10, 10, 10, 19, 19, 19, 19, 19, 19, 19, 19, 19, 23, 35,
			12, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 11, 19, 19,
			19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
			19
		}
	}
#endif
};

#ifdef MORE_LANGUAGES
int16 Size_jp[] = {
	15, 14, 16, 20, 19, 26, 22, 11, 18, 18, 27, 26, 13,
	19, 20, 27, 19, 15, 19, 19, 21, 19, 20, 18, 19, 15,
	13, 28, 15, 32, 15, 35, 15, 19, 19, 19, 19, 17, 16,
	19, 20, 15, 19, 20, 14, 17, 19, 19, 19, 19, 19, 19,
	19, 19, 20, 25, 20, 19, 19, 33, 31, 39, 37, 39, 37,
	21, 21, 21, 19, 17, 15, 23, 21, 15, 19, 20, 16, 19,
	19, 19, 20, 20, 17, 22, 19, 22, 22, 19, 22, 22, 23,
	35, 35, 35, 35, 37, 19, 19, 19, 19, 29, 19, 19, 19,
	19, 19, 9, 9, 9, 9, 19, 19, 19, 19, 19, 19, 19, 19,
	19, 19, 19, 19, 19, 30, 19, 19, 19, 19, 19, 10, 10,
	10, 10, 19, 19, 19, 19, 19, 19, 19, 19, 19, 23, 35,
	12, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
	19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19,
	19, 19, 19, 11, 19, 19, 19, 19, 19, 19, 19, 19, 19,
	19, 19, 19, 19, 19, 19, 19, 19, 19, 21
};
#endif

wchar foreign_table[128] = {
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0, 176,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0, 177,   0,   0,   0,   0,   0,  95,   0,   0,   0,   0, 175,
	128, 129, 130,   0, 131,   0, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141,
	  0, 173, 142, 143, 144,   0, 145,   0,   0, 146, 147, 148, 149,   0,   0, 150,
	151, 152, 153,   0, 154,   0, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
	  0, 174, 165, 166, 167,   0, 168,   0,   0, 169, 170, 171, 172,   0,   0,   0,
};

#ifdef BUTTON_ICONS
CSprite2d CFont::ButtonSprite[MAX_BUTTON_ICONS];
int CFont::PS2Symbol = BUTTON_NONE;
int CFont::ButtonsSlot = -1;
#endif

void
CFont::Initialise(void)
{
	#ifdef CHINESE
	#ifdef _WIN32
	CHSFont::EnsureConfig();
	#endif
	#endif
	int slot;

	slot = CTxdStore::AddTxdSlot("fonts");
#ifdef MORE_LANGUAGES
	Slot = slot;
	switch (LanguageSet)
	{
	case FONT_LANGSET_EFIGS:
	default:
		CTxdStore::LoadTxd(slot, "MODELS/FONTS.TXD");
		break;
	case FONT_LANGSET_POLISH:
		CTxdStore::LoadTxd(slot, "MODELS/FONTS_P.TXD");
		break;
	case FONT_LANGSET_RUSSIAN:
		CTxdStore::LoadTxd(slot, "MODELS/FONTS_R.TXD");
		break;
	case FONT_LANGSET_JAPANESE:
		CTxdStore::LoadTxd(slot, "MODELS/FONTS_J.TXD");
		break;
	}
#else
	CTxdStore::LoadTxd(slot, "MODELS/FONTS.TXD");
#endif
	CTxdStore::AddRef(slot);
	CTxdStore::PushCurrentTxd();
	CTxdStore::SetCurrentTxd(slot);
	Sprite[0].SetTexture("font2", "font2_mask");
#ifdef MORE_LANGUAGES
	if (IsJapanese()) {
		Sprite[1].SetTexture("FONTJAP", "FONTJAP_mask");
		Sprite[3].SetTexture("FONTJAP", "FONTJAP_mask");
	}
	else
#endif
		Sprite[1].SetTexture("pager", "pager_mask");
	Sprite[2].SetTexture("font1", "font1_mask");
	SetScale(1.0f, 1.0f);
	SetSlantRefPoint(SCREEN_WIDTH, 0.0f);
	SetSlant(0.0f);
	SetColor(CRGBA(255, 255, 255, 0));
	SetJustifyOff();
	SetCentreOff();
#ifdef FIX_BUGS
	SetWrapx(SCREEN_STRETCH_X(DEFAULT_SCREEN_WIDTH));
	SetCentreSize(SCREEN_STRETCH_X(DEFAULT_SCREEN_WIDTH));
#else
	SetWrapx(DEFAULT_SCREEN_WIDTH);
	SetCentreSize(DEFAULT_SCREEN_WIDTH);
#endif
	SetBackgroundOff();
	SetBackgroundColor(CRGBA(128, 128, 128, 128));
	SetBackGroundOnlyTextOff();
	SetPropOn();
	SetFontStyle(FONT_BANK);
	SetRightJustifyWrap(0.0f);
	SetAlphaFade(255.0f);
	SetDropShadowPosition(0);
	CTxdStore::PopCurrentTxd();

#ifdef CHINESE
	chineseSlot = CTxdStore::FindTxdSlot("chsfonts");
	gChineseFontSystemInitialised = true;

	if (FrontEndMenuManager.m_PrefsLanguage == CMenuManager::LANGUAGE_CHINESE)
		LoadChineseFonts();
#endif

#if !defined(GAMEPAD_MENU) && defined(BUTTON_ICONS)
	LoadButtons("MODELS/X360BTNS.TXD");
#endif
}

#ifdef BUTTON_ICONS
void
CFont::LoadButtons(const char* txdPath)
{
	if (int file = CFileMgr::OpenFile(txdPath)) {
		CFileMgr::CloseFile(file);
		if (ButtonsSlot == -1)
			ButtonsSlot = CTxdStore::AddTxdSlot("buttons");
		else {
			for (int i = 0; i < MAX_BUTTON_ICONS; i++)
				ButtonSprite[i].Delete();
			CTxdStore::RemoveTxd(ButtonsSlot);
		}
		CTxdStore::LoadTxd(ButtonsSlot, txdPath);
		CTxdStore::AddRef(ButtonsSlot);
		CTxdStore::PushCurrentTxd();
		CTxdStore::SetCurrentTxd(ButtonsSlot);
		ButtonSprite[BUTTON_CROSS].SetTexture("cross");
		ButtonSprite[BUTTON_CIRCLE].SetTexture("circle");
		ButtonSprite[BUTTON_SQUARE].SetTexture("square");
		ButtonSprite[BUTTON_TRIANGLE].SetTexture("triangle");
		ButtonSprite[BUTTON_L1].SetTexture("l1");
		ButtonSprite[BUTTON_L2].SetTexture("l2");
		ButtonSprite[BUTTON_L3].SetTexture("l3");
		ButtonSprite[BUTTON_R1].SetTexture("r1");
		ButtonSprite[BUTTON_R2].SetTexture("r2");
		ButtonSprite[BUTTON_R3].SetTexture("r3");
		CTxdStore::PopCurrentTxd();
	}
	else {
		if (ButtonsSlot != -1) {
			for (int i = 0; i < MAX_BUTTON_ICONS; i++)
				ButtonSprite[i].Delete();
			CTxdStore::RemoveTxdSlot(ButtonsSlot);
			ButtonsSlot = -1;
		}
	}
}
#endif

#ifdef MORE_LANGUAGES
void
CFont::ReloadFonts(uint8 set)
{
	if (Slot != -1 && LanguageSet != set) {
		Sprite[0].Delete();
		Sprite[1].Delete();
		Sprite[2].Delete();
		if (IsJapanese())
			Sprite[3].Delete();
		CTxdStore::PushCurrentTxd();
		CTxdStore::RemoveTxd(Slot);
		switch (set)
		{
		case FONT_LANGSET_EFIGS:
		default:
			CTxdStore::LoadTxd(Slot, "MODELS/FONTS.TXD");
			break;
		case FONT_LANGSET_POLISH:
			CTxdStore::LoadTxd(Slot, "MODELS/FONTS_P.TXD");
			break;
		case FONT_LANGSET_RUSSIAN:
			CTxdStore::LoadTxd(Slot, "MODELS/FONTS_R.TXD");
			break;
		case FONT_LANGSET_JAPANESE:
			CTxdStore::LoadTxd(Slot, "MODELS/FONTS_J.TXD");
			break;
		}
		CTxdStore::SetCurrentTxd(Slot);
		Sprite[0].SetTexture("font2", "font2_mask");
		if (set == FONT_LANGSET_JAPANESE) {
			Sprite[1].SetTexture("FONTJAP", "FONTJAP_mask");
			Sprite[3].SetTexture("FONTJAP", "FONTJAP_mask");
		}
		else
			Sprite[1].SetTexture("pager", "pager_mask");
		Sprite[2].SetTexture("font1", "font1_mask");
		CTxdStore::PopCurrentTxd();
	}
	LanguageSet = set;
}
#endif

void
CFont::Shutdown(void)
{
#ifdef BUTTON_ICONS
	if (ButtonsSlot != -1) {
		for (int i = 0; i < MAX_BUTTON_ICONS; i++)
			ButtonSprite[i].Delete();
		CTxdStore::RemoveTxdSlot(ButtonsSlot);
		ButtonsSlot = -1;
	}
#endif
#ifdef CHINESE
	UnloadChineseFonts();
	if (chineseSlot != -1) {
		CTxdStore::RemoveTxdSlot(chineseSlot);
		chineseSlot = -1;
	}
	gChineseFontSystemInitialised = false;
#endif

	Sprite[0].Delete();
	Sprite[1].Delete();
	Sprite[2].Delete();
#ifdef MORE_LANGUAGES
	if (IsJapanese())
		Sprite[3].Delete();
	CTxdStore::RemoveTxdSlot(Slot);
	Slot = -1;
#else
	CTxdStore::RemoveTxdSlot(CTxdStore::FindTxdSlot("fonts"));
#endif
}

void
CFont::InitPerFrame(void)
{
	Details.bank = CSprite2d::GetBank(30, Sprite[0].m_pTexture);
	CSprite2d::GetBank(15, Sprite[1].m_pTexture);
	CSprite2d::GetBank(15, Sprite[2].m_pTexture);
#ifdef MORE_LANGUAGES
	if (IsJapanese())
		CSprite2d::GetBank(15, Sprite[3].m_pTexture);
#endif
	SetDropShadowPosition(0);
	NewLine = false;
#ifdef BUTTON_ICONS
	PS2Symbol = BUTTON_NONE;
#endif
}

#ifdef BUTTON_ICONS
void
CFont::DrawButton(float x, float y)
{
	if (x <= 0.0f || x > SCREEN_WIDTH || y <= 0.0f || y > SCREEN_HEIGHT)
		return;

	if (PS2Symbol != BUTTON_NONE) {
		CRect rect;
		rect.left = x;
		rect.top = Details.scaleY + Details.scaleY + y;
		rect.right = Details.scaleY * 17.0f + x;
		rect.bottom = Details.scaleY * 19.0f + y;

		int vertexAlphaState;
		RwRenderStateGet(rwRENDERSTATEVERTEXALPHAENABLE, &vertexAlphaState);
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)TRUE);
		ButtonSprite[PS2Symbol].Draw(rect, CRGBA(255, 255, 255, Details.color.a));
		RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void *)vertexAlphaState);
	}
}
#endif

void
CFont::PrintChar(float x, float y, wchar c)
{
	if(x <= 0.0f || x > SCREEN_WIDTH ||
#ifdef FIX_BUGS
	   y <= 0.0f || y > SCREEN_HEIGHT)
#else
	   y <= 0.0f || y > SCREEN_WIDTH)
#endif
		return;

	float w = GetCharacterWidth(c) / 32.0f;
	float xoff = c % 16;
	float yoff = c / 16;
#ifdef MORE_LANGUAGES
	if (IsJapaneseFont()) {
		w = 21.0f;
		xoff = (float)(c % 48);
		yoff = c / 48;
	}
#endif

	if(Details.style == FONT_BANK || Details.style == FONT_HEADING){
		if(Details.dropShadowPosition != 0){
			CSprite2d::AddSpriteToBank(
#ifdef FIX_BUGS
				Details.bank + Details.style,
#else
				Details.style,
#endif
#ifdef FIX_BUGS
				CRect(x + SCREEN_SCALE_X(Details.dropShadowPosition),
				      y + SCREEN_SCALE_Y(Details.dropShadowPosition),
				      x + SCREEN_SCALE_X(Details.dropShadowPosition) + 32.0f * Details.scaleX * 1.0f,
				      y + SCREEN_SCALE_Y(Details.dropShadowPosition) + 40.0f * Details.scaleY * 0.5f),
#else
				CRect(x + Details.dropShadowPosition,
				      y + Details.dropShadowPosition,
				      x + Details.dropShadowPosition + 32.0f * Details.scaleX * 1.0f,
				      y + Details.dropShadowPosition + 40.0f * Details.scaleY * 0.5f),
#endif
				Details.dropColor,
				xoff/16.0f,                 yoff/12.8f,
				(xoff+1.0f)/16.0f - 0.001f, yoff/12.8f,
				xoff/16.0f,                 (yoff+1.0f)/12.8f,
				(xoff+1.0f)/16.0f - 0.001f, (yoff+1.0f)/12.8f - 0.0001f);
		}
		CSprite2d::AddSpriteToBank(
#ifdef FIX_BUGS
			Details.bank + Details.style,
#else
			Details.style,
#endif
			CRect(x, y,
			      x + 32.0f * Details.scaleX * 1.0f,
			      y + 40.0f * Details.scaleY * 0.5f),
			Details.color,
			xoff/16.0f,                 yoff/12.8f,
			(xoff+1.0f)/16.0f - 0.001f, yoff/12.8f,
			xoff/16.0f,                 (yoff+1.0f)/12.8f - 0.002f,
			(xoff+1.0f)/16.0f - 0.001f, (yoff+1.0f)/12.8f - 0.002f);
#ifdef MORE_LANGUAGES
	}else if (IsJapaneseFont()) {
		if (Details.dropShadowPosition != 0) {
			CSprite2d::AddSpriteToBank(
#ifdef FIX_BUGS
				Details.bank + Details.style,
#else
				Details.style,
#endif
#ifdef FIX_BUGS
				CRect(x + SCREEN_SCALE_X(Details.dropShadowPosition),
					y + SCREEN_SCALE_Y(Details.dropShadowPosition),
					x + SCREEN_SCALE_X(Details.dropShadowPosition) + 32.0f * Details.scaleX * 1.0f,
					y + SCREEN_SCALE_Y(Details.dropShadowPosition) + 40.0f * Details.scaleY / 2.75f),
#else
				CRect(x + Details.dropShadowPosition,
					y + Details.dropShadowPosition,
					x + Details.dropShadowPosition + 32.0f * Details.scaleX * 1.0f,
					y + Details.dropShadowPosition + 40.0f * Details.scaleY / 2.75f),
#endif
				Details.dropColor,
				xoff * w / 1024.0f, yoff / 25.6f,
				xoff * w / 1024.0f + (1.0f / 48.0f) - 0.001f, yoff / 25.6f,
				xoff * w / 1024.0f, (yoff + 1.0f) / 25.6f,
				xoff * w / 1024.0f + (1.0f / 48.0f) - 0.001f, (yoff + 1.0f) / 25.6f - 0.0001f);
		}
		CSprite2d::AddSpriteToBank(Details.bank + Details.style,
			CRect(x, y,
				x + 32.0f * Details.scaleX * 1.0f,
				y + 40.0f * Details.scaleY / 2.75f),
			Details.color,
			xoff * w / 1024.0f, yoff / 25.6f,
			xoff * w / 1024.0f + (1.0f / 48.0f) - 0.001f, yoff / 25.6f,
			xoff * w / 1024.0f, (yoff + 1.0f) / 25.6f - 0.002f,
			xoff * w / 1024.0f + (1.0f / 48.0f) - 0.001f, (yoff + 1.0f) / 25.6f - 0.0001f);
#endif
	}else
	{
		CSprite2d::AddSpriteToBank(
#ifdef FIX_BUGS
			Details.bank + Details.style,
#else
			Details.style,
#endif
			CRect(x, y,
					x + 32.0f * Details.scaleX * w,
					y + 32.0f * Details.scaleY * 0.5f),
			Details.color,
			xoff/16.0f,               yoff/16.0f,
			(xoff+w)/16.0f,           yoff/16.0f,
			xoff/16.0f,               (yoff+1.0f)/16.0f,
			(xoff+w)/16.0f - 0.0001f, (yoff+1.0f)/16.0f - 0.0001f);
	}
}

#ifdef MORE_LANGUAGES
bool CFont::IsJapanesePunctuation(wchar *str)
{
	return (*str == 0xE7 || *str == 0x124 || *str == 0x126 || *str == 0x128 || *str == 0x104 || *str == ',' || *str == '>' || *str == '!' || *str == 0x99 || *str == '?' || *str == ':');
}

bool CFont::IsAnsiCharacter(wchar *s)
{
	if (*s >= 'A' && *s <= 'Z')
		return true;
	if (*s >= 'a' && *s <= 'z')
		return true;
	if (*s >= '0' && *s <= ':')
		return true;
	if (*s == '(' || *s == ')')
		return true;
	if (*s == 'D' || *s == '$')
		return true;
	return false;
}
#endif

void
CFont::PrintString(float xstart, float ystart, wchar *s)
{
#ifdef CHINESE
	if (CGame::chineseGame) {
		PrintString_Chs(xstart, ystart, s);
		return;
	}
#endif
	CRect rect;
	int numSpaces;
	float lineLength;
	float x, y;
	bool first;
	wchar *start, *t;

	if(*s == '*')
		return;

	if(Details.background){
		GetNumberLines(xstart, ystart, s);
		GetTextRect(&rect, xstart, ystart, s);
		CSprite2d::DrawRect(rect, Details.backgroundColor);
	}

	lineLength = 0.0f;
	numSpaces = 0;
	first = true;
	if(Details.centre || Details.rightJustify)
		x = 0.0f;
	else
		x = xstart;
	y = ystart;
	start = s;

	for(;;){
		for(;;){
			for(;;){
				if(*s == '\0')
					return;
				float xend = Details.centre ? Details.centreSize :
				           Details.rightJustify ? xstart - Details.rightJustifyWrap :
				           Details.wrapX;
#ifdef MORE_LANGUAGES
				if (IsJapaneseFont())
					xend -= SCREEN_SCALE_X(21.0f * 2.0f);
#endif
				if(x + GetStringWidth(s) > xend && !first){
#ifdef MORE_LANGUAGES
					if (IsJapanese() && IsJapanesePunctuation(s))
						s--;
#endif
					float spaceWidth = !Details.justify || Details.centre ? 0.0f :
						(Details.wrapX - lineLength) / numSpaces;
					float xleft = Details.centre ? xstart - x/2 :
					              Details.rightJustify ? xstart - x :
					              xstart;
#ifdef MORE_LANGUAGES
					PrintString(xleft, y, start, s, spaceWidth, xstart);
#else
					PrintString(xleft, y, start, s, spaceWidth);
#endif
					lineLength = 0.0f;
					numSpaces = 0;
					first = true;
					if(Details.centre || Details.rightJustify)
						x = 0.0f;
					else
						x = xstart;
#ifdef MORE_LANGUAGES
					if (IsJapaneseFont())
						y += 32.0f * CFont::Details.scaleY / 2.75f + 2.0f * CFont::Details.scaleY;
					else
#endif
						y += 32.0f * CFont::Details.scaleY * 0.5f + 2.0f * CFont::Details.scaleY;
					start = s;
				}else
					break;
			}
			t = GetNextSpace(s);
			if(t[0] == '\0' ||
			   (t[0] == ' ' && t[1] == '\0'))
				break;
			if(!first)
				numSpaces++;
			first = false;
			x += GetStringWidth(s) + GetCharacterSize(*t - ' ');
#ifdef MORE_LANGUAGES
			if (IsJapaneseFont() && IsAnsiCharacter(s))
				x += 21.0f;
#endif
			lineLength = x;
			s = t+1;
#ifdef MORE_LANGUAGES
			if (IsJapaneseFont() && !*s) {
				x += GetStringWidth(s);
				if (IsAnsiCharacter(s))
					x += 21.0f;
				float xleft = Details.centre ? xstart - x / 2 :
					Details.rightJustify ? xstart - x :
					xstart;
				if (PrintString(xleft, y, start, s, 0.0f, xstart))
				{
					start = s;
					if (!Details.centre && !Details.rightJustify)
						x = xstart;
					else
						x = 0.0f;

					y += 32.0f * CFont::Details.scaleY / 2.75f + 2.0f * CFont::Details.scaleY;
					numSpaces = 0;
					first = true;
					lineLength = 0.0f;
				}
			}
#endif
		}
		if(t[0] == ' ' && t[1] == '\0')
			t[0] = '\0';
		x += GetStringWidth(s);
		s = t;
		float xleft = Details.centre ? xstart - x/2 :
		              Details.rightJustify ? xstart - x :
		              xstart;
#ifdef MORE_LANGUAGES
		if (PrintString(xleft, y, start, s, 0.0f, xstart) && IsJapaneseFont()) {
			start = s;
			if (!Details.centre && !Details.rightJustify)
				x = xstart;
			else
				x = 0.0f;
			y += 32.0f * CFont::Details.scaleY / 2.75f + 2.0f * CFont::Details.scaleY;
			numSpaces = 0;
			first = true;
			lineLength = 0.0f;
		}
#else
		PrintString(xleft, y, start, s, 0.0f);
#endif
	}
}

int
CFont::GetNumberLines(float xstart, float ystart, wchar *s)
{
#ifdef CHINESE
	if (CGame::chineseGame)
		return GetNumberLines_Chs(xstart, ystart, s);
#endif
	int n;
	float x, y;
	wchar *t;
	n = 0;

#ifdef MORE_LANGUAGES
	bool bSomeJapBool = false;

	if (IsJapanese()) {
		t = s;
		wchar unused;
		while (*t) {
			if (*t == JAP_TERMINATION || *t == '~')
				t = ParseToken(t, &unused, true);
			if (NewLine) {
				n++;
				NewLine = false;
				bSomeJapBool = true;
			}
			t++;
		}
	}

	if (bSomeJapBool) n--;
#endif

	if(Details.centre || Details.rightJustify)
		x = 0.0f;
	else
		x = xstart;
	y = ystart;

	while(*s){
#ifdef FIX_BUGS
		float f = Details.centre ? Details.centreSize :
		           Details.rightJustify ? xstart - Details.rightJustifyWrap :
		           Details.wrapX;
#else
		float f = (Details.centre ? Details.centreSize : Details.wrapX);
#endif

#ifdef MORE_LANGUAGES
		if (IsJapaneseFont())
			f -= SCREEN_SCALE_X(21.0f * 2.0f);
#endif

		if(x + GetStringWidth(s) > f){
#ifdef MORE_LANGUAGES
			if (IsJapanese())
			{
				if (IsJapanesePunctuation(s))
					s--;
			}
#endif
			if(Details.centre || Details.rightJustify)
				x = 0.0f;
			else
				x = xstart;
			n++;
#ifdef MORE_LANGUAGES
			if (IsJapanese())
				y += 32.0f * CFont::Details.scaleY / 2.75f + 2.0f * CFont::Details.scaleY;
			else
#endif
				y += 32.0f * CFont::Details.scaleY * 0.5f + 2.0f * CFont::Details.scaleY;
		}else{
			t = GetNextSpace(s);
			if(*t == '\0'){
				x += GetStringWidth(s);
#ifdef MORE_LANGUAGES
				if (IsJapanese() && IsAnsiCharacter(s))
					x += 21.0f;
#endif
				n++;
				s = t;
			}else{
				x += GetStringWidth(s);
#ifdef MORE_LANGUAGES
				if (IsJapanese() && IsAnsiCharacter(s))
					x += 21.0f;
#endif
				s = t+1;
				x += GetCharacterSize(*t - ' ');
#ifdef MORE_LANGUAGES
				if (IsJapanese() && !*s)
					n++;
#endif
			}
		}
	}

	return n;
}

void
CFont::GetTextRect(CRect *rect, float xstart, float ystart, wchar *s)
{
#ifdef CHINESE
	if (CGame::chineseGame) {
		GetTextRect_Chs(rect, xstart, ystart, s);
		return;
	}
#endif
	int numLines;
	float x, y;
	int16 maxlength;
	wchar *t;

	maxlength = 0;
	numLines = 0;

#ifdef MORE_LANGUAGES
	if (IsJapanese()) {
		numLines = GetNumberLines(xstart, ystart, s);
	}else{
#endif

#ifdef FIX_BUGS
		if(Details.centre || Details.rightJustify)
#else
		if(Details.centre)
#endif
			x = 0.0f;
		else
			x = xstart;
		y = ystart;

#ifdef FIX_BUGS
		float xEnd = Details.centre ? Details.centreSize :
		           Details.rightJustify ? xstart - Details.rightJustifyWrap :
		           Details.wrapX;
#else
		float xEnd = (Details.centre ? Details.centreSize : Details.wrapX);
#endif
		while(*s){
			if(x + GetStringWidth(s) > xEnd){
				if(x > maxlength)
					maxlength = x;
#ifdef FIX_BUGS
				if(Details.centre || Details.rightJustify)
#else
				if(Details.centre)
#endif
					x = 0.0f;
				else
					x = xstart;
				numLines++;
				y += 32.0f * CFont::Details.scaleY * 0.5f + 2.0f * CFont::Details.scaleY;
			}else{
				t = GetNextSpace(s);
				if(*t == '\0'){
					x += GetStringWidth(s);
					if(x > maxlength)
						maxlength = x;
					numLines++;
					s = t;
				}else{
					x += GetStringWidth(s);
					x += GetCharacterSize(*t - ' ');
					s = t+1;
				}
			}
		}
#ifdef MORE_LANGUAGES
	}
#endif

	if(Details.centre){
		if(Details.backgroundOnlyText){
			rect->left = xstart - maxlength/2 - 4.0f;
			rect->right = xstart + maxlength/2 + 4.0f;
#ifdef MORE_LANGUAGES
			if (IsJapaneseFont()) {
				rect->bottom = (32.0f * CFont::Details.scaleY / 2.75f + 2.0f * CFont::Details.scaleY) * numLines + ystart + (4.0f / 2.75f);
				rect->top = ystart - (4.0f / 2.75f);
			} else {
#endif
				rect->bottom = (32.0f * CFont::Details.scaleY * 0.5f + 2.0f * CFont::Details.scaleY) * numLines + ystart + 2.0f;
				rect->top = ystart - 2.0f;
#ifdef MORE_LANGUAGES
			}
#endif
		}else{
			rect->left = xstart - Details.centreSize*0.5f - 4.0f;
			rect->right = xstart + Details.centreSize*0.5f + 4.0f;
#ifdef MORE_LANGUAGES
			if (IsJapaneseFont()) {
				rect->bottom = (32.0f * CFont::Details.scaleY / 2.75f + 2.0f * CFont::Details.scaleY) * numLines + ystart + (4.0f / 2.75f);
				rect->top = ystart - (4.0f / 2.75f);
			} else {
#endif
				rect->bottom = (32.0f * CFont::Details.scaleY * 0.5f + 2.0f * CFont::Details.scaleY) * numLines + ystart + 2.0f;
				rect->top = ystart - 2.0f;
#ifdef MORE_LANGUAGES
			}
#endif
		}
	}else{
		rect->left = xstart - 4.0f;
		rect->right = Details.wrapX;
		rect->bottom = ystart - 4.0f + 4.0f;
#ifdef MORE_LANGUAGES
		if (IsJapaneseFont())
			rect->top = (32.0f * CFont::Details.scaleY / 2.75f + 2.0f * CFont::Details.scaleY) * numLines + ystart + 2.0f + (4.0f / 2.75f);
		else
#endif
			rect->top = (32.0f * CFont::Details.scaleY * 0.5f + 2.0f * CFont::Details.scaleY) * numLines + ystart + 2.0f + 2.0f;
	}
}

#ifdef MORE_LANGUAGES
bool
CFont::PrintString(float x, float y, wchar *start, wchar *&end, float spwidth, float japX)
{
	wchar *s, c, unused;

	if (IsJapanese()) {
		float jx = 0.0f;
		for (s = start; s < end; s++) {
			if (*s == JAP_TERMINATION || *s == '~')
				s = ParseToken(s, &unused, true);
			if (NewLine) {
				NewLine = false;
				break;
			}
			jx += GetCharacterSize(*s - ' ');
		}
		s = start;
		if (Details.centre)
			x = japX - jx / 2.0f;
		else if (Details.rightJustify)
			x = japX - jx;
	}

	for (s = start; s < end; s++) {
		if (*s == '~' || (IsJapanese() && *s == JAP_TERMINATION))
			s = ParseToken(s, &unused);
		if (NewLine && IsJapanese()) {
			NewLine = false;
			end = s;
			return true;
		}
		c = *s - ' ';
		if (Details.slant != 0.0f && !IsJapanese())
			y = (Details.slantRefX - x) * Details.slant + Details.slantRefY;

#ifdef BUTTON_ICONS
		if (PS2Symbol != BUTTON_NONE) {
			DrawButton(x, y);
			x += Details.scaleY * 17.0f;
			PS2Symbol = BUTTON_NONE;
		}
#endif

		PrintChar(x, y, c);
		x += GetCharacterSize(c);
		if (c == 0 && (!NewLine || !IsJapanese()))
			x += spwidth;
	}
	return false;
}
#else
void
CFont::PrintString(float x, float y, wchar *start, wchar *end, float spwidth)
{
	wchar *s, c, unused;

	for(s = start; s < end; s++){
		if(*s == '~')
			s = ParseToken(s, &unused);
		c = *s - ' ';
		if(Details.slant != 0.0f)
			y = (Details.slantRefX - x)*Details.slant + Details.slantRefY;
		PrintChar(x, y, c);
		x += GetCharacterSize(c);
		if(c == 0)
			x += spwidth;
	}
}
#endif

void
CFont::PrintStringFromBottom(float x, float y, wchar *str)
{
#ifdef CHINESE
	if (CGame::chineseGame) {
		y -= (18.0f * Details.scaleY) * GetNumberLines_Chs(x, y, str);
		if (Details.slant != 0.0f)
			y -= ((Details.slantRefX - x) * Details.slant + Details.slantRefY);
		PrintString_Chs(x, y, str);
		return;
	}
#endif

#ifdef MORE_LANGUAGES
	if (IsJapaneseFont())
		y -= (32.0f * CFont::Details.scaleY / 2.75f + 2.0f * CFont::Details.scaleY) * GetNumberLines(x, y, str);
	else
#endif
		y -= (32.0f * CFont::Details.scaleY * 0.5f + 2.0f * CFont::Details.scaleY) * GetNumberLines(x, y, str);
	PrintString(x, y, str);
}

#ifdef XBOX_SUBTITLES
void
CFont::PrintOutlinedString(float x, float y, wchar *str, float outlineStrength, bool fromBottom, CRGBA outlineColor)
{
	CRGBA textColor = Details.color;
	SetColor(outlineColor);
	CVector2D offsets[] = { {1.f, 1.f}, {1.f, -1.f}, {-1.f, 1.f}, {-1.f, -1.f} };
	for(int i = 0; i < ARRAY_SIZE(offsets); i++){
		if (fromBottom)
			PrintStringFromBottom(x + SCREEN_SCALE_X(offsets[i].x * outlineStrength), y + SCREEN_SCALE_Y(offsets[i].y * outlineStrength), str);
		else
			PrintString(x + SCREEN_SCALE_X(offsets[i].x * outlineStrength), y + SCREEN_SCALE_Y(offsets[i].y * outlineStrength), str);
	}
	SetColor(textColor);
	
	if (fromBottom)
		PrintStringFromBottom(x, y, str);
	else
		PrintString(x, y, str);
}
#endif

float
CFont::GetCharacterWidth(wchar c)
{
#ifdef MORE_LANGUAGES
	if (IsJapanese()) {
		if (!Details.proportional)
			return Size[0][Details.style][192];
		if (c <= 94 || Details.style == FONT_HEADING || Details.style == FONT_BANK) {
			switch (Details.style)
			{
			case FONT_JAPANESE:
				return Size_jp[c];
			default:
				return Size[0][Details.style][c];
			}
		}
		if (c < 254 && Details.style == FONT_PAGER)
			return 29.4f;

		switch (Details.style)
		{
		case FONT_JAPANESE:
			return 29.4f;
		case FONT_BANK:
			return 10.0f;
		case FONT_PAGER:
			return 31.5f;
		default:
			return Size[0][Details.style][c];
		}
	}

	else if (Details.proportional)
		return Size[LanguageSet][Details.style][c];
	else
		return Size[LanguageSet][Details.style][192];
#else
	if (Details.proportional)
		return Size[Details.style][c];
	else
		return Size[Details.style][192];
#endif
}

float
CFont::GetCharacterSize(wchar c)
{
#ifdef MORE_LANGUAGES
	if (IsJapanese())
	{
		if (!Details.proportional)
			return Size[0][Details.style][192] * Details.scaleX;
		if (c <= 94 || Details.style == FONT_HEADING || Details.style == FONT_BANK) {
			switch (Details.style)
			{
			case FONT_JAPANESE:
				return Size_jp[c] * Details.scaleX;
			default:
				return Size[0][Details.style][c] * Details.scaleX;
			}
		}
		if (c < 254 && (Details.style == FONT_PAGER))
			return 29.4f * Details.scaleX;

		switch (Details.style)
		{
		case FONT_JAPANESE:
			return 29.4f * Details.scaleX;
		case FONT_BANK:
			return 10.0f * Details.scaleX;
		case FONT_PAGER:
			return 31.5f * Details.scaleX;
		default:
			return Size[0][Details.style][c] * Details.scaleX;
		}
	}
	else if(Details.proportional)
		return Size[LanguageSet][Details.style][c] * Details.scaleX;
	else
		return Size[LanguageSet][Details.style][192] * Details.scaleX;
#else
	if (Details.proportional)
		return Size[Details.style][c] * Details.scaleX;
	else
		return Size[Details.style][192] * Details.scaleX;
#endif
}

float
CFont::GetStringWidth(wchar *s, bool spaces)
{
#ifdef CHINESE
	if (CGame::chineseGame)
		return GetStringWidth_Chs(s, spaces);
#endif
	float w = 0.0f;
#ifdef MORE_LANGUAGES
	if (IsJapanese())
	{
		do
		{
			if ((*s != ' ' || spaces) && *s != '\0') {
				do {
					while (*s == '~' || *s == JAP_TERMINATION) {
						s++;
#ifdef BUTTON_ICONS
						switch (*s) {
						case 'X':
						case 'O':
						case 'Q':
						case 'T':
						case 'K':
						case 'M':
						case 'A':
						case 'J':
						case 'V':
						case 'C':
							w += 17.0f * Details.scaleY;
							break;
						default:
							break;
						}
#endif
						while (!(*s == '~' || *s == JAP_TERMINATION)) s++;
						s++;
					}
					w += GetCharacterSize(*s - ' ');
					++s;
				} while (*s == '~' || *s == JAP_TERMINATION);
			}
		} while (IsAnsiCharacter(s));
	} else
#endif
	{
		for (; (*s != ' ' || spaces) && *s != '\0'; s++) {
			if (*s == '~') {
				s++;
#ifdef BUTTON_ICONS
				switch (*s) {
				case 'X':
				case 'O':
				case 'Q':
				case 'T':
				case 'K':
				case 'M':
				case 'A':
				case 'J':
				case 'V':
				case 'C':
					w += 17.0f * Details.scaleY;
					break;
				default:
					break;
				}
#endif
				while (*s != '~') s++;
#ifndef FIX_BUGS
				s++;
				if (*s == ' ' && !spaces)
					break;
			} 
#else
			} else
#endif
				w += GetCharacterSize(*s - ' ');
		}
	}
	return w;
}

#ifdef MORE_LANGUAGES
float
CFont::GetStringWidth_Jap(wchar* s)
{
	float w = 0.0f;
	for (; *s != '\0';) {
		do {
			while (*s == '~' || *s == JAP_TERMINATION) {
				s++;
				while (!(*s == '~' || *s == JAP_TERMINATION)) s++;
				s++;
			}
			w += GetCharacterSize(*s - ' ');
			++s;
		} while (*s == '~' || *s == JAP_TERMINATION);
	}
	return w;
}
#endif

wchar*
CFont::GetNextSpace(wchar *s)
{
#ifdef CHINESE
	if (CGame::chineseGame)
		return GetNextSpace_Chs(s);
#endif
#ifdef MORE_LANGUAGES
	if (IsJapanese()) {
		do
		{
			if (*s != ' ' && *s != '\0') {
				do {
					while (*s == '~' || *s == JAP_TERMINATION) {
						s++;
						while (!(*s == '~' || *s == JAP_TERMINATION)) s++;
						s++;
					}
					++s;
				} while (*s == '~' || *s == JAP_TERMINATION);
			}
		} while (IsAnsiCharacter(s));
	} else
#endif
	{
		for(; *s != ' ' && *s != '\0'; s++)
			if(*s == '~'){
				s++;
				while(*s != '~') s++;
#ifndef FIX_BUGS
				s++;
				if(*s == ' ')
					break;
#endif
			}
	}
	return s;
}

#ifdef MORE_LANGUAGES
wchar*
CFont::ParseToken(wchar *s, wchar* ss, bool japShit)
{
#ifdef FIX_BUGS
	const uint8 alpha = Details.color.a;
#endif
	s++;
	if ((Details.color.r || Details.color.g || Details.color.b) && !japShit) {
		wchar c = *s;
		if (IsJapanese())
			c &= 0x7FFF;
		switch (c) {
		case 'N':
		case 'n':
			NewLine = true;
			break;
		case 'b': SetColor(CRGBA(128, 167, 243, 255)); break;
		case 'g': SetColor(CRGBA(95, 160, 106, 255)); break;
		case 'h': SetColor(CRGBA(225, 225, 225, 255)); break;
		case 'l': SetColor(CRGBA(0, 0, 0, 255)); break;
		case 'p': SetColor(CRGBA(168, 110, 252, 255)); break;
		case 'r': SetColor(CRGBA(113, 43, 73, 255)); break;
		case 'w': SetColor(CRGBA(175, 175, 175, 255)); break;
		case 'y': SetColor(CRGBA(210, 196, 106, 255)); break;
#ifdef BUTTON_ICONS
		case 'X': PS2Symbol = BUTTON_CROSS; break;
		case 'O': PS2Symbol = BUTTON_CIRCLE; break;
		case 'Q': PS2Symbol = BUTTON_SQUARE; break;
		case 'T': PS2Symbol = BUTTON_TRIANGLE; break;
		case 'K': PS2Symbol = BUTTON_L1; break;
		case 'M': PS2Symbol = BUTTON_L2; break;
		case 'A': PS2Symbol = BUTTON_L3; break;
		case 'J': PS2Symbol = BUTTON_R1; break;
		case 'V': PS2Symbol = BUTTON_R2; break;
		case 'C': PS2Symbol = BUTTON_R3; break;
#endif
		}
	} else if (IsJapanese()) {
		if ((*s & 0x7FFF) == 'N' || (*s & 0x7FFF) == 'n')
			NewLine = true;
	}
#ifdef FIX_BUGS
	// Colour tokens change RGB only; keep the message's current fade alpha.
	Details.color.a = alpha;
#endif
	while ((!IsJapanese() || (*s != JAP_TERMINATION)) && *s != '~') s++;
#ifdef FIX_BUGS
	if (*(++s) == '~')
		s = ParseToken(s, ss, japShit);
	return s;
#else
	return s + 1;
#endif
}
#else
wchar*
CFont::ParseToken(wchar *s, wchar*)
{
#ifdef FIX_BUGS
	const uint8 alpha = Details.color.a;
#endif
	s++;
	if(Details.color.r || Details.color.g || Details.color.b)
		switch(*s){
		case 'N':
		case 'n':
			NewLine = true;
			break;
		case 'b': SetColor(CRGBA(128, 167, 243, 255)); break;
		case 'g': SetColor(CRGBA(95, 160, 106, 255)); break;
		case 'h': SetColor(CRGBA(225, 225, 225, 255)); break;
		case 'l': SetColor(CRGBA(0, 0, 0, 255)); break;
		case 'p': SetColor(CRGBA(168, 110, 252, 255)); break;
		case 'r': SetColor(CRGBA(113, 43, 73, 255)); break;
		case 'w': SetColor(CRGBA(175, 175, 175, 255)); break;
		case 'y': SetColor(CRGBA(210, 196, 106, 255)); break;
#ifdef BUTTON_ICONS
		case 'X': PS2Symbol = BUTTON_CROSS; break;
		case 'O': PS2Symbol = BUTTON_CIRCLE; break;
		case 'Q': PS2Symbol = BUTTON_SQUARE; break;
		case 'T': PS2Symbol = BUTTON_TRIANGLE; break;
		case 'K': PS2Symbol = BUTTON_L1; break;
		case 'M': PS2Symbol = BUTTON_L2; break;
		case 'A': PS2Symbol = BUTTON_L3; break;
		case 'J': PS2Symbol = BUTTON_R1; break;
		case 'V': PS2Symbol = BUTTON_R2; break;
		case 'C': PS2Symbol = BUTTON_R3; break;
#endif
		}
#ifdef FIX_BUGS
	Details.color.a = alpha;
#endif
	while(*s != '~') s++;
	return s+1;
}
#endif

void
CFont::DrawFonts(void)
{
	CSprite2d::DrawBank(Details.bank);
	CSprite2d::DrawBank(Details.bank+1);
	CSprite2d::DrawBank(Details.bank+2);
#ifdef MORE_LANGUAGES
	if (IsJapanese())
		CSprite2d::DrawBank(Details.bank+3);
#endif
}

void
CFont::SetScale(float x, float y)
{
	Details.scaleX = x;
	Details.scaleY = y;
}

void
CFont::SetSlantRefPoint(float x, float y)
{
	Details.slantRefX = x;
	Details.slantRefY = y;
}

void
CFont::SetSlant(float s)
{
	Details.slant = s;
}

void
CFont::SetColor(CRGBA col)
{
	Details.color = col;
	if (Details.alphaFade < 255.0f)
		Details.color.a *= Details.alphaFade / 255.0f;
}

void
CFont::SetJustifyOn(void)
{
	Details.justify = true;
	Details.centre = false;
	Details.rightJustify = false;
}

void
CFont::SetJustifyOff(void)
{
	Details.justify = false;
	Details.rightJustify = false;
}

void
CFont::SetCentreOn(void)
{
	Details.centre = true;
	Details.justify = false;
	Details.rightJustify = false;
}

void
CFont::SetCentreOff(void)
{
	Details.centre = false;
}

void
CFont::SetWrapx(float x)
{
	Details.wrapX = x;
}

void
CFont::SetCentreSize(float s)
{
	Details.centreSize = s;
}

void
CFont::SetBackgroundOn(void)
{
	Details.background = true;
}

void
CFont::SetBackgroundOff(void)
{
	Details.background = false;
}

void
CFont::SetBackgroundColor(CRGBA col)
{
	Details.backgroundColor = col;
}

void
CFont::SetBackGroundOnlyTextOn(void)
{
	Details.backgroundOnlyText = true;
}

void
CFont::SetBackGroundOnlyTextOff(void)
{
	Details.backgroundOnlyText = false;
}

void
CFont::SetRightJustifyOn(void)
{
	Details.rightJustify = true;
	Details.justify = false;
	Details.centre = false;
}

void
CFont::SetRightJustifyOff(void)
{
	Details.rightJustify = false;
	Details.justify = false;
	Details.centre = false;
}

void
CFont::SetPropOn(void)
{
	Details.proportional = true;
}

void
CFont::SetPropOff(void)
{
	Details.proportional = false;
}

void
CFont::SetFontStyle(int16 style)
{
	Details.style = style;
}

void
CFont::SetRightJustifyWrap(float wrap)
{
	Details.rightJustifyWrap = wrap;
}

void
CFont::SetAlphaFade(float fade)
{
	Details.alphaFade = fade;
}

void
CFont::SetDropColor(CRGBA col)
{
	Details.dropColor = col;
	if (Details.alphaFade < 255.0f)
		Details.dropColor.a *= Details.alphaFade / 255.0f;
}

void
CFont::SetDropShadowPosition(int16 pos)
{
	Details.dropShadowPosition = pos;
}

wchar
CFont::character_code(uint8 c)
{
	if(c < 128)
		return c;
	return foreign_table[c-128];
}

#ifdef CHINESE
void
CFont::LoadChineseFonts()
{
	if (!gChineseFontSystemInitialised || gChineseFontsLoaded)
		return;

	#ifdef _WIN32
	if (CHSFont::Init()) {
		CHSFont::Preload(TheText.GetLoadedData(), TheText.GetLoadedDataLength());
		gChineseFontsLoaded = true;
		return;
	}
	#endif

	if (chineseSlot == -1)
		chineseSlot = CTxdStore::AddTxdSlot("chsfonts");

	if (!ReadTable())
		return;

	CTxdStore::LoadTxd(chineseSlot, "models/chinese.txd");
	CTxdStore::AddRef(chineseSlot);
	CTxdStore::PushCurrentTxd();
	CTxdStore::SetCurrentTxd(chineseSlot);
	SpriteChinese[FONT_CHN_NORMAL].SetTexture("normal", "normalm");
	SpriteChinese[FONT_CHN_SLANT].SetTexture("slant", "slantm");
	CTxdStore::PopCurrentTxd();

	gStaticChineseFontsLoaded = true;
	gChineseFontsLoaded = true;
}

void
CFont::UnloadChineseFonts()
{
	if (!gChineseFontsLoaded)
		return;

	#ifdef _WIN32
	CHSFont::Shutdown();
	#endif
	if (gStaticChineseFontsLoaded) {
		SpriteChinese[FONT_CHN_NORMAL].Delete();
		SpriteChinese[FONT_CHN_SLANT].Delete();

		if (chineseSlot != -1)
			CTxdStore::RemoveTxd(chineseSlot);
	}

	gStaticChineseFontsLoaded = false;
	gChineseFontsLoaded = false;
}

void
CFont::ReloadChineseFonts()
{
	if (!gChineseFontSystemInitialised)
		return;

	UnloadChineseFonts();
	LoadChineseFonts();
}

void
CFont::PrintCharDispatcher(float x, float y, wchar c)
{
	if (c < 0x80) {
		wchar ascii = c - ' ';
		PrintChar(x, y, ascii);
	} else {
		PrintCHSChar(x, y, c);
	}
}

void
CFont::SetChineseFontStyleOverride(int16 style)
{
	gChineseFontStyleOverride = style;
}

void
CFont::SetChineseSyntheticSlant(float slant)
{
	gChineseSyntheticSlant = slant;
}

void
CFont::PrintCHSChar(float x, float y, uint32 c)
{
	static const float rRowsCount = 1.0f / 64.0f;
	static const float rColumnsCount = 1.0f / 64.0f;
	static const float ufix = 0.001f / 4.0f;
	static const float vfix = 0.001f / 4.0f;

	if (x <= 0.0f || x > SCREEN_WIDTH || y <= 0.0f || y > SCREEN_HEIGHT)
		return;

	// GTA III uses FONT_BANK for most UI text, while VC uses FONT_STANDARD.
	// Treating the numeric value 0 as "slant" therefore selects a different
	// CJK face throughout re3. Normal is the reVC-compatible default; callers
	// may still request the optional slant set explicitly through the override.
	bool slant = gChineseFontStyleOverride == FONT_CHN_SLANT;
	CharPos pos = GetCharPos(c, slant);
	CRect rect;

	float yOffset = Details.scaleY * 2.0f;
	// The atlas cell is square.  Use the vertical scale for both screen axes so
	// CJK glyphs stay square even when the original Latin font uses unrelated
	// horizontal and vertical scales (subtitles and menus do this frequently).
	float displayScale = Details.style == FONT_BANK && gChineseSyntheticSlant != 0.0f ?
		CHS_BANK_DISPLAY_SCALE : CHS_DISPLAY_SCALE;
	float charSize = Details.scaleY * 16.0f * displayScale;
	float shear = charSize * gChineseSyntheticSlant;
	// BANK and HEADING use a 20-unit-high native quad in GTA III; the other
	// atlas path uses 16. Centre CJK inside the active native quad, not inside
	// the 18-unit CHS line advance. This puts menu text in the selection bar's
	// true centre and gives save-slot numbers and adjacent CJK the same centre.
	float nativeCellHeight = Details.style == FONT_BANK || Details.style == FONT_HEADING ? 20.0f : 16.0f;
	yOffset = Details.scaleY * (nativeCellHeight - 16.0f * displayScale) * 0.5f;

	rect.left = x;
	rect.top = y + yOffset;
	rect.right = x + charSize;
	rect.bottom = y + yOffset + charSize;

	float u1 = pos.columnIndex * rColumnsCount;
	float v1 = pos.rowIndex * rRowsCount;
	float u2 = (pos.columnIndex + 1) * rColumnsCount - ufix;
	float v2 = (pos.rowIndex + 1) * rRowsCount - vfix;

	CSprite2d *spr;
	CRGBA color = Details.color;
	#ifdef _WIN32
	if (CHSFont::Inited()) {
		spr = &CHSFont::SpriteC[pos.page];
		if (CHSFont::IsSlotColor(c, slant))
			color = CRGBA(255, 255, 255, Details.color.a);
	} else
	#endif
		spr = slant ? &SpriteChinese[FONT_CHN_SLANT] : &SpriteChinese[FONT_CHN_NORMAL];

	if (Details.dropShadowPosition != 0) {
		float shadowX = SCREEN_SCALE_X(Details.dropShadowPosition);
		float shadowY = SCREEN_SCALE_Y(Details.dropShadowPosition);
		CRect shadowRect(rect.left + shadowX, rect.top + shadowY,
			rect.right + shadowX, rect.bottom + shadowY);
		gChsShadowBatch.push_back({ spr, shadowRect, Details.dropColor, u1, v1, u2, v2, shear });
	}

	// reVC renders its accumulated font vertex buffer with linear filtering.
	// Queue the foreground here; RenderChsBatches submits one draw per page.
	gChsMainBatch.push_back({ spr, rect, color, u1, v1, u2, v2, shear });
}

float
CFont::GetCharacterSize_Chs(wchar c, uint16 fontStyle, bool fontHalfTexture, bool prop, float scaleX, float scaleY)
{
	if (c >= 0x80)
		// Keep the original 29/32 cell advance while deriving it from the
		// square glyph size.  Drawing, centring and wrapping now agree.
		return 14.5f * (fontStyle == FONT_BANK && gChineseSyntheticSlant != 0.0f ?
			CHS_BANK_DISPLAY_SCALE : CHS_DISPLAY_SCALE) * scaleY;

	wchar ascii = c - ' ';

	if (prop) {
		return GetCharacterWidth(ascii) * scaleX;
	} else {
		if (IsPunctuation(c))
			return (GetCharacterWidth(ascii) / 1.6f) * scaleX;
		else
			return GetCharacterWidth(ascii) * scaleX;
	}
}

float
CFont::GetCharacterSizeNormal(wchar c)
{
	return GetCharacterSize_Chs(c, Details.style, false, Details.proportional, Details.scaleX, Details.scaleY);
}

float
CFont::GetCharacterSizeDrawing(wchar c)
{
	return GetCharacterSize_Chs(c, Details.style, false, Details.proportional, Details.scaleX, Details.scaleY);
}

void
CFont::RenderFontBuffer_Chs(void)
{
	RenderChsBatches();
}

#ifdef BUTTON_ICONS
static int
ChineseButtonToken(const wchar *s)
{
	if (s[0] != '~' || !s[1] || s[2] != '~') return BUTTON_NONE;
	switch (s[1]) {
	case 'X': return BUTTON_CROSS;
	case 'O': return BUTTON_CIRCLE;
	case 'Q': return BUTTON_SQUARE;
	case 'T': return BUTTON_TRIANGLE;
	case 'K': return BUTTON_L1;
	case 'M': return BUTTON_L2;
	case 'A': return BUTTON_L3;
	case 'J': return BUTTON_R1;
	case 'V': return BUTTON_R2;
	case 'C': return BUTTON_R3;
	default: return BUTTON_NONE;
	}
}
#endif

void
CFont::PrintString_Chs(float x, float y, wchar *text)
{
	CRect rect;
	float xBound;
	float yBound = y;
	float strWidth, widthLimit;
	float lastLineWidth = 0.0f;
	float printX;
	wchar *ptext = text;
	wchar *strHead = text;
	bool emptyLine = true;
	short numSpaces = 0;

	if (*text == '*')
		return;
	#ifdef _WIN32
	if(CHSFont::Inited())
		// Any GXT-external characters in this string are generated under one
		// atlas lock/upload, rather than one 64MB upload per character.
		CHSFont::Preload(text, UnicodeStrlen(text));
	#endif

	// PrintString_Chs is the re3 equivalent of one reVC font-buffer pass.
	// It owns the temporary batches so stale quads can never cross strings.
	gChsShadowBatch.clear();
	gChsMainBatch.clear();

	if (Details.background) {
		GetTextRect_Chs(&rect, x, y, text);
		CSprite2d::DrawRect(rect, Details.backgroundColor);
	}

	auto DrawLineWithTokens = [&](float startX, float startY, wchar *start, wchar *end) {
		float curX = startX;
		wchar *cur = start;
		wchar unused;
		while (cur < end) {
			if (*cur == '~') {
#ifdef BUTTON_ICONS
				int button = ChineseButtonToken(cur);
				if (button != BUTTON_NONE) {
					RenderFontBuffer_Chs();
					PS2Symbol = button;
					DrawButton(curX, startY);
					PS2Symbol = BUTTON_NONE;
					curX += Details.scaleY * 17.0f;
					cur += 3;
					continue;
				}
#endif
				// Parse one token at a time: the normal parser recursively consumes
				// adjacent tokens, which would swallow the next button before drawing.
				if (cur[1] && cur[2] == '~') {
					wchar token[] = { '~', cur[1], '~', 0 };
					ParseToken(token, &unused);
					cur += 3;
					continue;
				}
				cur = ParseToken(cur, &unused);
				continue;
			}
			if (IsZeroWidthChineseChar(*cur)) {
				cur++;
				continue;
			}
			int units;
			uint32 cp = DecodeChineseCodepoint(cur, units);
			if (cp < 0x80)
				PrintCharDispatcher(curX, startY, (wchar)cp);
			else
				PrintCHSChar(curX, startY, cp);
			curX += GetCharacterSizeNormal((wchar)cp);
			cur += units;
		}
	};

	if (Details.centre || Details.rightJustify)
		xBound = 0.0f;
	else
		xBound = x;

	while (*ptext != 0) {
		if (ptext[0] == '~' && (ptext[1] == 'n' || ptext[1] == 'N') && ptext[2] == '~') {
			if (Details.centre)
				printX = x - xBound * 0.5f;
			else if (Details.rightJustify)
				printX = x - xBound;
			else
				printX = x;

			DrawLineWithTokens(printX, yBound, strHead, ptext);

			yBound += Details.scaleY * 18.0f;

			if (Details.centre || Details.rightJustify)
				xBound = 0.0f;
			else
				xBound = x;

			lastLineWidth = 0.0f;
			numSpaces = 0;
			emptyLine = true;

			ptext += 3;
			strHead = ptext;
			continue;
		}

		strWidth = GetStringWidth_Chs(ptext, false);

		if (Details.centre)
			widthLimit = Details.centreSize;
		else if (Details.rightJustify)
			widthLimit = x - Details.rightJustifyWrap;
		else
			widthLimit = Details.wrapX;

		if (((xBound + strWidth) <= widthLimit) || emptyLine) {
			ptext = GetNextSpace_Chs(ptext);
			xBound += strWidth;

			if (*ptext != 0) {
				if (*ptext == ' ') {
					if (*(ptext + 1) == 0) {
						*ptext = 0;
					} else {
						if (!emptyLine)
							++numSpaces;

						xBound += GetCharacterSizeNormal(' ');
						++ptext;
					}
				}

				emptyLine = false;
				lastLineWidth = xBound;
			} else {
				if (Details.centre)
					printX = x - xBound * 0.5f;
				else if (Details.rightJustify)
					printX = x - xBound;
				else
					printX = x;

				DrawLineWithTokens(printX, yBound, strHead, ptext);
			}
		} else {
			if (Details.centre)
				printX = x - xBound * 0.5f;
			else if (Details.rightJustify)
				printX = x - xBound;
			else
				printX = x;

			DrawLineWithTokens(printX, yBound, strHead, ptext);

			strHead = ptext;

			if (Details.centre || Details.rightJustify)
				xBound = 0.0f;
			else
				xBound = x;

			yBound += Details.scaleY * 18.0f;
			lastLineWidth = 0.0f;
			numSpaces = 0;
			emptyLine = true;
		}
	}

	RenderFontBuffer_Chs();
}

int
CFont::GetNumberLines_Chs(float xstart, float ystart, wchar *s)
{
	int result = 0;
	float xBound;
	float strWidth, widthLimit;

	if (Details.centre || Details.rightJustify)
		xBound = 0.0f;
	else
		xBound = xstart;

	while (*s != 0) {
		if (s[0] == '~' && (s[1] == 'n' || s[1] == 'N') && s[2] == '~') {
			++result;

			if (Details.centre || Details.rightJustify)
				xBound = 0.0f;
			else
				xBound = xstart;

			s += 3;
			continue;
		}

		strWidth = GetStringWidth_Chs(s, false);

		if (Details.centre)
			widthLimit = Details.centreSize;
		else if (Details.rightJustify)
			widthLimit = xstart - Details.rightJustifyWrap;
		else
			widthLimit = Details.wrapX;

		if ((xBound + strWidth) <= widthLimit) {
			xBound += strWidth;
			s = GetNextSpace_Chs(s);

			if (*s == ' ') {
				xBound += GetCharacterSizeNormal(' ');
				++s;
			} else if (*s == 0) {
				++result;
			}
		} else {
			if (Details.centre || Details.rightJustify)
				xBound = 0.0f;
			else
				xBound = xstart;

			++result;
		}
	}

	return result;
}

void
CFont::GetTextRect_Chs(CRect *rect, float xstart, float ystart, wchar *s)
{
	short numLines = GetNumberLines_Chs(xstart, ystart, s);
	float lineHeight = Details.scaleY * 18.0f;

	if (Details.centre) {
		rect->left = xstart - (Details.centreSize * 0.5f) - 4.0f;
		rect->right = xstart + (Details.centreSize * 0.5f) + 4.0f;
	} else if (Details.rightJustify) {
		rect->left = Details.rightJustifyWrap - 4.0f;
		rect->right = xstart + 4.0f;
	} else {
		rect->left = xstart - 4.0f;
		rect->right = Details.wrapX + 4.0f;
	}

	rect->top = ystart - 2.0f;
	rect->bottom = ystart + lineHeight * numLines + 2.0f;
}

float
CFont::GetStringWidth_Chs(wchar *s, bool spaces)
{
	float result = 0.0f;

	while (*s != '\0') {
		if (IsZeroWidthChineseChar(*s)) {
			++s;
			continue;
		} else if (*s == ' ') {
			if (spaces)
				result += GetCharacterSizeNormal(' ');
			else
				break;
		} else if (*s == '~') {
			if ((s[1] == 'n' || s[1] == 'N') && s[2] == '~')
				break;

			if (result == 0.0f || spaces) {
#ifdef BUTTON_ICONS
				if (ChineseButtonToken(s) != BUTTON_NONE) {
					result += Details.scaleY * 17.0f;
					if (!spaces) return result;
				}
#endif
				do {
					++s;
				} while (*s != '~' && *s != '\0');
			} else {
				break;
			}
		} else if (*s < 0x80) {
			result += GetCharacterSizeNormal(*s);
		} else {
			int units;
			DecodeChineseCodepoint(s, units);
			if (result == 0.0f || spaces)
				result += GetCharacterSizeNormal(*s);

			if (!spaces)
				break;
			s += units - 1;
		}

		++s;
	}

	return result;
}

wchar *
CFont::GetNextSpace_Chs(wchar *s)
{
	wchar *temp = s;

	while (*temp != ' ' && *temp != '\0') {
		if (*temp == '~') {
			if (temp == s) {
#ifdef BUTTON_ICONS
				if (ChineseButtonToken(temp) != BUTTON_NONE)
					return temp + 3;
#endif
				if ((temp[1] == 'n' || temp[1] == 'N') && temp[2] == '~')
					break;

				do {
					++temp;
				} while (*temp != '~' && *temp != '\0');

				if (*temp == '\0')
					break;

				++temp;
				s = temp;
				continue;
			} else {
				break;
			}
		} else if (*temp >= 0x80) {
			if (temp == s)
				++temp;
			break;
		}

		++temp;
	}

	return temp;
}
#endif
