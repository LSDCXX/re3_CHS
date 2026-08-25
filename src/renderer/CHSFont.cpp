// Runtime-generated Chinese font atlas (Windows / GDI).
//
// Replaces the static CHINESE.TXD + Chinese.dat pair with a fully dynamic
// glyph cache: any codepoint the font can render is rasterized into a
// 64x64 grid atlas page on first use, so the game can never run out of
// glyphs (no 4096-slot limit).
//
// Font settings are read from [Fonts] in re3.ini:
//   NormalFonts = comma separated candidate faces for the normal set
//   NormalBold  = 1 to prefer a real bold face, 0 for regular weight
//   SlantFontFile = .ttf/.ttc file (relative to the exe dir) for FONT_BANK
//   GlyphHeight = rasterized glyph height in pixels (32..128)
//
// Two glyph sets mirror the original normal/slant pair:
//   normal: regular CJK font (default Microsoft YaHei Bold)
//   slant : cursive font from SlantFontFile, falls back to normal if the
//           file cannot be loaded.
// Glyphs are placed using the font's own metrics (GLYPHMETRICS), so
// punctuation keeps its designed position inside its advance cell.
#include "common.h"

#ifdef _WIN32
#include "CHSFont.h"

#include <windows.h>
#include <wchar.h>
#include <cwctype>
#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <cstring>
#include <cstdarg>

// DirectWrite backend (optional; see CHSFont_dwrite.h).
#include "CHSFont_dwrite.h"

#define MINI_CASE_SENSITIVE
#include "ini.h"

// stb_truetype: declarations only (implementation is in CHSFont_stb.cpp).
#include "skeleton/imgui/imstb_truetype.h"

#ifdef _MSC_VER
#pragma comment(lib, "gdi32.lib")
#endif

#define CHS_GRID_SIZE    64   // grid cells per page (row and column)
#define CHS_SLOT_PIXELS  64   // pixels per glyph cell (page 4096 / 64)
#define CHS_PAGE_PIXELS  4096 // page texture size
#define CHS_PAGE_SLOTS   (CHS_GRID_SIZE * CHS_GRID_SIZE)

// Defaults, overridable via [Fonts] in re3.ini
#define CHS_DEFAULT_NORMAL_FONTS "Microsoft YaHei,SimSun,SimHei,NSimSun,DengXian,KaiTi"
#define CHS_DEFAULT_SLANT_FONT   L"models\\YingZhangXingShu.ttf"
#define CHS_DEFAULT_GLYPH_HEIGHT 56

// One font entry: either a family name, or a font file path (recognized by
// a .ttf/.ttc/.otf extension). File entries are loaded privately and their
// family name is resolved from the file's name table.
struct ChsFontItem
{
	bool isFile;
	std::wstring faceName; // family name (resolved from the file when isFile)
};

static std::vector<ChsFontItem> gNormalItems; // normal set candidates
static bool gNormalBold = true;               // prefer real bold face (GDI normal set)
static bool gSlantBold = true;                // bold for the slant (GDI) set
static ChsFontItem gSlantItem;                // slant font entry (path or family name)
// [Fonts] NormalWeight / SlantWeight / RareWeight (100-900): independent
// per-set weights for the DWrite wght axis. 0 = unset (bold-file logic).
static int                      gNormalWeight = 0;
static int                      gSlantWeight = 0;
static int                      gRareWeight = 0;
static int gGlyphHeight = CHS_DEFAULT_GLYPH_HEIGHT;
// raw string values as written in re3.ini, kept so re3's save keeps the
// [Fonts] section intact (byte-for-byte, including file paths)
static std::string gRawNormalFonts = CHS_DEFAULT_NORMAL_FONTS;
static std::string gRawSlantFont = "models\\YingZhangXingShu.ttf";
static std::string gRawRareFont = ""; // RareFontFile: supplementary-plane font, empty = disabled
// so Shutdown can unload them all (deduplicated)
static std::vector<std::pair<std::wstring, std::wstring>> gLoadedFonts;
// stb_truetype state for supplementary-plane glyphs (rare font)
// stb_truetype supplementary-plane fonts (RareFontFile list); per-codepoint fallback.
struct RareFont { stbtt_fontinfo info; std::vector<uint8> data; };
static std::vector<RareFont> gRareFonts;

CSprite2d CHSFont::SpriteC[CHS_MAX_PAGES];
int CHSFont::NumPages = 0;

static bool gInited = false;
static HDC gHdc[2] = { nil, nil };
static HFONT gFont[2] = { nil, nil };
static int32 gBaseY[2] = { 48, 48 }; // baseline inside the 64px cell, from font metrics
static int32 gPageCount[2] = { 0, 0 };  // pages created per set
static int32 gCurrentPageSlots[2] = { 0, 0 }; // used slots in the current page of each set
// key = codepoint | (slant ? 0x10000 : 0)
static std::unordered_map<uint32, CharPos> gSlotMap;
// Per-slot color flag: true when the glyph in that atlas cell is a COLOR
// glyph (COLR/CPAL emoji). The batch renderer then skips vertex-color
// modulation for that cell (white RGB, alpha kept) so the emoji shows its
// real colors. Indexed by (page * CHS_PAGE_SLOTS + row * CHS_GRID_SIZE + col).
static bool gSlotColor[CHS_MAX_PAGES][CHS_PAGE_SLOTS] = { false };
static CharPos gFallbackSlot = { 0, 63, 63 };

// Text rendering backend selector ([Fonts] TextRenderer):
//   GDI    - GDI GetGlyphOutlineW dynamic atlas (pre-DWrite path)
//   DWrite - DirectWrite backend (default, most capable)
//   TXD    - static CHINESE.TXD atlas (original scheme; no dynamic features)
#define CHS_TEXT_GDI      0
#define CHS_TEXT_DWRITE   1
#define CHS_TEXT_TXD      2
static int gTextRenderer = CHS_TEXT_DWRITE; // default: DirectWrite
static bool gDwOk = false;                  // DirectWrite engine ready
// During GXT/string preloading keep one atlas page locked and upload it once.
// The original per-glyph lock/unlock can transfer a 4096x4096 texture for
// every new character, which is the dominant source of subtitle stalls.
static bool gAtlasWriteBatch = false;
static RwRaster *gLockedAtlasRaster = nil;
static uint8 *gLockedAtlasPixels = nil;

// Forward declaration: InitRareFont is defined later but LoadFontConfig calls it.
static bool InitRareFont(const std::wstring &absPath);

// Debug log written next to the game exe; helps diagnose font loading.
static void
ChsLog(const char *fmt, ...)
{
	FILE *f = fopen("chsfont.log", "a");
	if(!f)
		return;
	va_list va;
	va_start(va, fmt);
	vfprintf(f, fmt, va);
	va_end(va);
	fprintf(f, "\n");
	fclose(f);
}

// Log a wide string safely (narrow vfprintf %ls fails on non-ASCII).
static void
ChsLogW(const wchar_t *s)
{
	if(!s) {
		ChsLog("  (null)");
		return;
	}
	int len = WideCharToMultiByte(CP_UTF8, 0, s, -1, nil, 0, nil, nil);
	if(len <= 0) {
		ChsLog("  (wide->utf8 failed)");
		return;
	}
	std::vector<char> tmp(len);
	WideCharToMultiByte(CP_UTF8, 0, s, -1, tmp.data(), len, nil, nil);
	ChsLog("%s", tmp.data());
}

static std::string
WideToAnsi(const std::wstring &s)
{
	int len = WideCharToMultiByte(CP_ACP, 0, s.c_str(), -1, nil, 0, nil, nil);
	if(len <= 0)
		return std::string();
	std::vector<char> tmp(len);
	WideCharToMultiByte(CP_ACP, 0, s.c_str(), -1, tmp.data(), len, nil, nil);
	return std::string(tmp.data());
}

static MAT2
MakeIdentityMat2(void)
{
	MAT2 m;
	m.eM11.value = 1; m.eM11.fract = 0;
	m.eM12.value = 0; m.eM12.fract = 0;
	m.eM21.value = 0; m.eM21.fract = 0;
	m.eM22.value = 1; m.eM22.fract = 0;
	return m;
}

// Decode an ini string: re3.ini is saved as ANSI (GBK) by re3, but users may save it as UTF-8.
// Try strict UTF-8 first, fall back to ANSI (system codepage).
static std::wstring
IniToWideStr(const std::string &s)
{
	if(s.empty())
		return std::wstring();
	int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), -1, nil, 0);
	if(len > 0) {
		std::vector<wchar_t> tmp(len);
		MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, tmp.data(), len);
		return std::wstring(tmp.data());
	}
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, nil, 0);
	if(len <= 0)
		return std::wstring();
	std::vector<wchar_t> tmp2(len);
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, tmp2.data(), len);
	return std::wstring(tmp2.data());
}

// "a, b ,c" -> {a, b, c} (UTF-8 -> wide)
static std::vector<std::wstring>
SplitCommaList(const std::string &s)
{
	std::vector<std::wstring> out;
	size_t pos = 0;
	while(pos <= s.size()) {
		size_t comma = s.find(',', pos);
		if(comma == std::string::npos)
			comma = s.size();
		std::string item = s.substr(pos, comma - pos);
		size_t a = item.find_first_not_of(" \t");
		size_t b = item.find_last_not_of(" \t");
		if(a != std::string::npos)
			item = item.substr(a, b - a + 1);
		else
			item.clear();
		if(!item.empty())
			out.push_back(IniToWideStr(item));
		pos = comma + 1;
	}
	return out;
}

// True when the entry looks like a font file path (.ttf/.ttc/.otf, any case).
static bool
IsFontFile(const std::wstring &s)
{
	size_t dot = s.find_last_of(L'.');
	if(dot == std::wstring::npos || dot + 1 >= s.size())
		return false;
	std::wstring ext = s.substr(dot + 1);
	for(size_t i = 0; i < ext.size(); i++)
		ext[i] = (wchar_t)towlower(ext[i]);
	return ext == L"ttf" || ext == L"ttc" || ext == L"otf";
}

static bool ReadFontFaceName(const wchar_t *path, std::wstring &out); // defined below

// exe directory + relative path -> absolute path.
// Absolute paths (drive letter or UNC) are used as-is.
static std::wstring
MakeAbsPath(const std::wstring &rel)
{
	// already absolute: C:\... or \\server\share
	if(rel.size() >= 2 && ((rel[0] >= L'A' && rel[0] <= L'Z') || (rel[0] >= L'a' && rel[0] <= L'z')) && rel[1] == L':')
		return rel;
	if(rel.size() >= 2 && rel[0] == L'\\' && rel[1] == L'\\')
		return rel;
	wchar_t buf[MAX_PATH];
	DWORD n = GetModuleFileNameW(nil, buf, MAX_PATH);
	wchar_t *slash = wcsrchr(buf, L'\\');
	if(slash)
		*(slash + 1) = 0;
	if(n == 0 || slash == nil)
		return rel;
	return std::wstring(buf) + rel;
}
// Privately load a font file and resolve its family name. Deduplicated:
// repeated loads of the same file reuse the earlier face name.
static bool
LoadFontFile(const std::wstring &absPath, std::wstring &faceName)
{
	for(size_t i = 0; i < gLoadedFonts.size(); i++)
		if(gLoadedFonts[i].first == absPath) {
			faceName = gLoadedFonts[i].second;
			return true;
		}

	ChsLog("  loading font file:");
	ChsLogW(absPath.c_str());
	int added = AddFontResourceExW(absPath.c_str(), FR_PRIVATE, 0);
	ChsLog("    AddFontResourceExW -> %d", added);
	if(added <= 0)
		return false;
	if(!ReadFontFaceName(absPath.c_str(), faceName)) {
		ChsLog("    could not parse face name");
		RemoveFontResourceExW(absPath.c_str(), FR_PRIVATE, 0);
		return false;
	}
	gLoadedFonts.push_back(std::make_pair(absPath, faceName));
	ChsLog("    face name:");
	ChsLogW(faceName.c_str());
	return true;
}

// Parse one ini entry: family name, or font file path (auto-loaded).
static ChsFontItem
MakeFontItem(const std::wstring &raw)
{
	ChsFontItem item;
	item.isFile = false; // default; do NOT memset (struct holds std::wstring)
	item.faceName.clear();
	if(IsFontFile(raw)) {
		item.isFile = true;
		LoadFontFile(MakeAbsPath(raw), item.faceName);
	} else {
		item.faceName = raw;
	}
	return item;
}

// Make sure the [Fonts] section with default keys exists in re3.ini
// next to re3.exe (absolute path, immune to the game's chdir-ing).
// Missing keys are written back first; if writing fails or the section is
// still unreadable afterwards, fail hard instead of using an unknown source.
static void
EnsureFontsIni(void)
{
	std::wstring iniPath = MakeAbsPath(L"re3.ini");
	ChsLog("re3.ini path:");
	ChsLogW(iniPath.c_str());

	mINI::INIFile iniFile(WideToAnsi(iniPath));
	mINI::INIStructure cfg;
	iniFile.read(cfg); // reads ALL sections; empty if the file is missing

	bool needWrite = !cfg.has("Fonts");
	if(!needWrite) {
		mINI::INIMap<std::string> &sec = cfg["Fonts"];
		needWrite = !sec.has("NormalFonts") || !sec.has("NormalBold") ||
		            !sec.has("SlantFontFile") || !sec.has("GlyphHeight") ||
		            !sec.has("RareFontFile") || !sec.has("SlantBold") ||
		            !sec.has("TextRenderer") ||
		            !sec.has("NormalWeight") ||
		            !sec.has("SlantWeight") || !sec.has("RareWeight");
	}
	if(needWrite) {
		mINI::INIMap<std::string> &sec = cfg["Fonts"];
		if(!sec.has("NormalFonts"))
			sec.set("NormalFonts", CHS_DEFAULT_NORMAL_FONTS);
		if(!sec.has("NormalBold"))
			sec.set("NormalBold", "1");
		if(!sec.has("SlantFontFile"))
			sec.set("SlantFontFile", "models\\YingZhangXingShu.ttf");
		if(!sec.has("GlyphHeight"))
			sec.set("GlyphHeight", "56");
		if(!sec.has("RareFontFile"))
			sec.set("RareFontFile", "");
		if(!sec.has("SlantBold"))
			sec.set("SlantBold", "1");
		if(!sec.has("TextRenderer"))
			sec.set("TextRenderer", "3"); // 1=TXD 2=GDI 3=DWrite
		if(!sec.has("NormalWeight")) {
			// default follows the bold-file preference: bold => 700, else 400
			std::string nb = sec.has("NormalBold") ? sec.get("NormalBold") : "1";
			sec.set("NormalWeight", nb == "0" ? "400" : "700");
		}
		if(!sec.has("SlantWeight")) {
			std::string sb = sec.has("SlantBold") ? sec.get("SlantBold") : "1";
			sec.set("SlantWeight", sb == "0" ? "400" : "700");
		}
		if(!sec.has("RareWeight"))
			sec.set("RareWeight", "400");

		bool ok = iniFile.write(cfg); // keeps other sections, adds [Fonts]
		ChsLog("  writing [Fonts] defaults -> %s", ok ? "OK" : "FAILED");
		if(!ok) {
			ChsLog("FATAL: cannot write re3.ini");
			MessageBoxA(nil,
				"Cannot write re3.ini (the file next to re3.exe).\r\n"
				"Check file permissions and restart the game.",
				"re3 Chinese Font Error", MB_OK | MB_ICONERROR);
			exit(3);
		}
		// re-read to confirm the section really exists now
		if(!iniFile.read(cfg) || !cfg.has("Fonts")) {
			ChsLog("FATAL: [Fonts] still missing after write");
			MessageBoxA(nil,
				"re3.ini was written but the [Fonts] section could not be read back.\r\n"
				"Restore re3.ini manually and restart the game.",
				"re3 Chinese Font Error", MB_OK | MB_ICONERROR);
			exit(3);
		}
	} else {
		ChsLog("  [Fonts] already present");
	}
}

// Load font settings from [Fonts] in re3.ini and apply them.
static void
LoadFontConfig(void)
{
	// defaults (used for individual missing keys)
	std::vector<std::wstring> defaults = SplitCommaList(CHS_DEFAULT_NORMAL_FONTS);
	gNormalItems.clear();
	for(size_t i = 0; i < defaults.size(); i++)
		gNormalItems.push_back(MakeFontItem(defaults[i]));
	gNormalBold = true;
	gSlantItem = MakeFontItem(CHS_DEFAULT_SLANT_FONT);
	gGlyphHeight = CHS_DEFAULT_GLYPH_HEIGHT;

	EnsureFontsIni();

	mINI::INIFile iniFile(WideToAnsi(MakeAbsPath(L"re3.ini")));
	mINI::INIStructure cfg;
	iniFile.read(cfg);
	if(!cfg.has("Fonts")) {
		// cannot happen after EnsureFontsIni, but stay safe
		ChsLog("FATAL: [Fonts] missing after EnsureFontsIni");
		exit(3);
	}
	mINI::INIMap<std::string> &sec = cfg["Fonts"];

	if(sec.has("NormalFonts")) {
		gRawNormalFonts = sec.get("NormalFonts");
		std::vector<std::wstring> list = SplitCommaList(sec.get("NormalFonts"));
		gNormalItems.clear();
		for(size_t i = 0; i < list.size(); i++)
			gNormalItems.push_back(MakeFontItem(list[i]));
	} else {
		ChsLog("  NormalFonts missing, using default");
	}
	if(sec.has("NormalBold"))
		gNormalBold = sec.get("NormalBold") != "0";
	else
		ChsLog("  NormalBold missing, using default");
	if(sec.has("SlantBold"))
		gSlantBold = sec.get("SlantBold") != "0";
	else
		ChsLog("  SlantBold missing, using default");
	if(sec.has("SlantFontFile")) {
		gRawSlantFont = sec.get("SlantFontFile");
		gSlantItem = MakeFontItem(IniToWideStr(sec.get("SlantFontFile")));
	} else {
		ChsLog("  SlantFontFile missing, using default");
	}
	if(sec.has("GlyphHeight")) {
		int h = atoi(sec.get("GlyphHeight").c_str());
		if(h >= 32 && h <= 128)
			gGlyphHeight = h;
	} else {
		ChsLog("  GlyphHeight missing, using default");
	}
	if(sec.has("RareFontFile")) {
		gRawRareFont = sec.get("RareFontFile");
	} else {
		gRawRareFont.clear();
		ChsLog("  RareFontFile missing, using default (none)");
	}
	// RareFontFile may be a comma-separated list; per-codepoint fallback.
	if(!gRawRareFont.empty()) {
		std::wstring rareW = IniToWideStr(gRawRareFont);
		size_t pos = 0;
		while(pos <= rareW.size()) {
			size_t comma = rareW.find(L',', pos);
			if(comma == std::wstring::npos)
				comma = rareW.size();
			std::wstring item = rareW.substr(pos, comma - pos);
			size_t a = item.find_first_not_of(L" \t");
			size_t b = item.find_last_not_of(L" \t");
			if(a != std::wstring::npos)
				item = item.substr(a, b - a + 1);
			else
				item.clear();
			if(!item.empty())
				InitRareFont(MakeAbsPath(item));
			pos = comma + 1;
		}
	} else {
		gRareFonts.clear();
	}

	gNormalWeight = 0; // reset: stay 0 when a key is absent
	gSlantWeight = 0;
	gRareWeight = 0;
	if(sec.has("NormalWeight")) {
		int w = atoi(sec.get("NormalWeight").c_str());
		if(w >= 100 && w <= 900)
			gNormalWeight = w;
	} else {
		ChsLog("  NormalWeight missing, using default (unset)");
	}
	if(sec.has("SlantWeight")) {
		int w = atoi(sec.get("SlantWeight").c_str());
		if(w >= 100 && w <= 900)
			gSlantWeight = w;
	} else {
		ChsLog("  SlantWeight missing, using default (unset)");
	}
	if(sec.has("RareWeight")) {
		int w = atoi(sec.get("RareWeight").c_str());
		if(w >= 100 && w <= 900)
			gRareWeight = w;
	} else {
		ChsLog("  RareWeight missing, using default (unset)");
	}
	if(sec.has("TextRenderer")) {
		std::string tr = sec.get("TextRenderer");
		// Numeric selection: 1=TXD (static CHINESE.TXD), 2=GDI (GDI dynamic
		// atlas), 3=DWrite (DirectWrite backend, default). The pre-numeric
		// strings ("TXD"/"GDI"/"DWrite") are still accepted so configs from
		// older builds keep working; anything else falls back to DWrite.
		if(tr == "1" || tr == "TXD")
			gTextRenderer = CHS_TEXT_TXD;
		else if(tr == "2" || tr == "GDI")
			gTextRenderer = CHS_TEXT_GDI;
		else
			gTextRenderer = CHS_TEXT_DWRITE; // includes "3"/"DWrite"/unknown
	} else {
		ChsLog("  TextRenderer missing, using default (DWrite)");
	}
}

// Create one new empty atlas page (transparent) for the given glyph set.
static bool
CreateNewPage(bool slant)
{
	int32 set = slant ? 1 : 0;
	if(gPageCount[set] >= CHS_PAGES_PER_SET)
		return false;

	int32 page = (slant ? CHS_SLANT_BASE : 0) + gPageCount[set];
	RwRaster *ras = RwRasterCreate(CHS_PAGE_PIXELS, CHS_PAGE_PIXELS, 32,
		rwRASTERTYPETEXTURE | rwRASTERFORMAT8888);
	if(ras == nil)
		return false;

	uint8 *px = RwRasterLock(ras, 0, rwRASTERLOCKWRITE);
	if(px) {
		// C8888 (32-bit) rasters never have row padding on D3D9/GL
		memset(px, 0, CHS_PAGE_PIXELS * CHS_PAGE_PIXELS * 4);
		RwRasterUnlock(ras);
	}

	RwTexture *tex = RwTextureCreate(ras);
	if(tex == nil) {
		RwRasterDestroy(ras);
		return false;
	}
	RwTextureSetFilterMode(tex, rwFILTERLINEAR);
	RwTextureSetAddressing(tex, rwTEXTUREADDRESSCLAMP);

	CHSFont::SpriteC[page].m_pTexture = tex;
	memset(gSlotColor[page], 0, sizeof(gSlotColor[page])); // fresh page: no color flags
	CHSFont::NumPages++;
	gPageCount[set]++;
	gCurrentPageSlots[set] = 0;
	return true;
}

static bool RenderGlyphStb(uint32 chr, int32 page, int32 row, int32 col); // below
static bool RenderGlyphDWrite(uint32 chr, int32 page, int32 row, int32 col); // below (DirectWrite backend)

static void
FinishAtlasWriteBatch(void)
{
	if(gLockedAtlasRaster != nil)
		RwRasterUnlock(gLockedAtlasRaster);
	gLockedAtlasRaster = nil;
	gLockedAtlasPixels = nil;
}

static uint8 *
LockAtlasForGlyph(RwRaster *ras)
{
	if(!gAtlasWriteBatch)
		return RwRasterLock(ras, 0, rwRASTERLOCKWRITE);
	if(gLockedAtlasRaster == ras)
		return gLockedAtlasPixels;
	FinishAtlasWriteBatch();
	gLockedAtlasRaster = ras;
	gLockedAtlasPixels = RwRasterLock(ras, 0, rwRASTERLOCKWRITE);
	if(gLockedAtlasPixels == nil)
		gLockedAtlasRaster = nil;
	return gLockedAtlasPixels;
}

static void
UnlockAtlasForGlyph(RwRaster *ras)
{
	if(!gAtlasWriteBatch)
		RwRasterUnlock(ras);
}

// Rasterize one codepoint into atlas cell (row, col) of page.
// Pixel layout: R=G=B=A=gray (0..255), so the result is identical for
// BGRA (D3D9) and RGBA (GL) memory order and modulates correctly with the
// vertex color.
static bool
RenderGlyph(uint32 chr, bool slant, int32 page, int32 row, int32 col)
{
	int32 set = slant ? 1 : 0;
	if(gHdc[set] == nil || gFont[set] == nil)
		return false;

	// GDI GetGlyphOutlineW only accepts 16-bit wchar, cannot rasterize supplementary plane codepoints.
	// Supplementary plane (CJK ext / Emoji) goes through stb_truetype + RareFontFile.
	if(chr > 0xFFFF) {
		// Supplementary plane: DWrite rasterizes 32-bit codepoints and color
		// emoji, so try it first, then fall back to stb (as before).
		if(gTextRenderer == CHS_TEXT_DWRITE && gDwOk) {
			if(RenderGlyphDWrite(chr, page, row, col))
				return true;
		}
		return RenderGlyphStb(chr, page, row, col);
	}

	// DirectWrite backend: rasterize BMP glyphs with DirectWrite first, then
	// fall back to GDI/stb when the selected face cannot provide the glyph.
	if(gTextRenderer == CHS_TEXT_DWRITE && gDwOk) {
		if(RenderGlyphDWrite(chr, page, row, col))
			return true;
	}

	MAT2 mat = MakeIdentityMat2();
	GLYPHMETRICS gm;
	memset(&gm, 0, sizeof(gm));

	DWORD size = GetGlyphOutlineW(gHdc[set], (wchar)chr, GGO_GRAY8_BITMAP, &gm, 0, nil, &mat);
	if(size == GDI_ERROR || gm.gmBlackBoxX == 0 || gm.gmBlackBoxY == 0)
		return RenderGlyphStb(chr, page, row, col); // GDI has no glyph, try rare fonts (stb)

	std::vector<uint8> buf(size);
	if(GetGlyphOutlineW(gHdc[set], (wchar)chr, GGO_GRAY8_BITMAP, &gm, size, buf.data(), &mat) == GDI_ERROR)
		return RenderGlyphStb(chr, page, row, col); // second pass failed, try stb

	uint32 srcStride = ((gm.gmBlackBoxX + 3) / 4) * 4; // GGO_GRAY8 rows are 4-byte aligned

	RwRaster *ras = RwTextureGetRaster(CHSFont::SpriteC[page].m_pTexture);
	uint8 *px = LockAtlasForGlyph(ras);
	if(!px)
		return false;

	// C8888 (32-bit) rasters never have row padding on D3D9/GL
	const int32 dstStride = CHS_PAGE_PIXELS * 4;

	int32 ox = col * CHS_SLOT_PIXELS;
	int32 oy = row * CHS_SLOT_PIXELS;

	// Place the glyph exactly as the font file designed it:
	// - the font's own advance cell (gmCellIncX) is centered in the 64px cell,
	//   so full-width CJK glyphs sit centered while punctuation keeps its
	//   designed position inside its advance cell (left for , . ! ? etc.)
	// - the glyph black box is offset from the baseline by gmptGlyphOrigin,
	//   so glyphs and punctuation share the font's real baseline.
	int32 cellIncX = (int32)gm.gmCellIncX;
	if(cellIncX <= 0)
		cellIncX = CHS_SLOT_PIXELS; // defensive: fall back to full cell
	int32 penX = (CHS_SLOT_PIXELS - cellIncX) / 2;
	int32 x0 = penX + (int32)gm.gmptGlyphOrigin.x;
	int32 y0 = gBaseY[set] - (int32)gm.gmptGlyphOrigin.y;

	// guard: GetGlyphOutlineW may return a smaller buffer than the bbox implies
	if(buf.size() < (size_t)gm.gmBlackBoxY * srcStride) {
		UnlockAtlasForGlyph(ras);
		return RenderGlyphStb(chr, page, row, col); // buffer mismatch, try stb
	}

	for(uint32 y = 0; y < gm.gmBlackBoxY; y++) {
		int32 dy = y0 + (int32)y;
		if(dy < 0 || dy >= CHS_SLOT_PIXELS)
			continue;
		for(uint32 x = 0; x < gm.gmBlackBoxX; x++) {
			int32 dx = x0 + (int32)x;
			if(dx < 0 || dx >= CHS_SLOT_PIXELS)
				continue;
			uint8 g = buf[y * srcStride + x]; // 0..64
			uint8 v = (uint8)((g * 255u) / 64u);
			uint8 *dst = px + (oy + dy) * dstStride + (ox + dx) * 4;
			dst[0] = v;
			dst[1] = v;
			dst[2] = v;
			dst[3] = v;
		}
	}
	UnlockAtlasForGlyph(ras);
	return true;
}

// DirectWrite backend: rasterize one BMP codepoint into atlas cell (row,
// col) using the engine in CHSFont_dwrite.cpp. Placement rules mirror the
// GDI path exactly: advance cell centered, then black-box offsets from the
// glyph origin, baseline at gBaseY[set]. Returns false so the caller can
static bool
RenderGlyphDWrite(uint32 chr, int32 page, int32 row, int32 col)
{
	int32 set = (page >= CHS_SLANT_BASE) ? 1 : 0;
	ChsDWOut out;
	std::vector<uint8> alpha;
	std::vector<uint8> color;
	// Rasterize; if the black box overflows the 64px cell (wide/tall color
	// emoji, CJK-ext glyphs), re-rasterize at a proportionally smaller em
	// size so the whole glyph fits and nothing gets clipped at the cell edge.
	int emPx = gGlyphHeight;
	for(int attempt = 0; attempt < 3; attempt++) {
		if(!ChsDwRender(chr, set != 0, emPx, gBaseY[set], out, alpha, color))
			return false; // engine failure or missing glyph
		if((out.isColor && color.empty()) || (!out.isColor && alpha.empty()))
			return false;
		// fit check: black box must fit in the cell
		if(out.w + 2 <= CHS_SLOT_PIXELS && out.h + 2 <= CHS_SLOT_PIXELS) // +2 anti-alias margin
			break;
		float s = (float)CHS_SLOT_PIXELS / (out.w > out.h ? (float)out.w : (float)out.h);
		if(s >= 1.0f || attempt == 2) { // already fits or this was the last possible try
			// cannot shrink further (or already smallest): keep what we have
			// and let the placement below center/clip gracefully
			break;
		}
		emPx = (int)((float)emPx * s * 0.92f); // 0.92 headroom: avoid landing exactly at 64
		if(emPx < 16) emPx = 16;
		alpha.clear();
		color.clear();
	}

	RwRaster *ras = RwTextureGetRaster(CHSFont::SpriteC[page].m_pTexture);
	uint8 *px = LockAtlasForGlyph(ras);
	if(!px)
		return false;
	const int32 dstStride = CHS_PAGE_PIXELS * 4;
	int32 ox = col * CHS_SLOT_PIXELS;
	int32 oy = row * CHS_SLOT_PIXELS;

	// Placement: center the black box horizontally (penX), glyph offset
	// inside the cell (left), and direct absolute row (top) because the
	// engine was told the cell baseline (gBaseY[set]).
	// COLOR glyphs center on their BLACK BOX width (out.w): emoji advance is
	// often wider than the drawn shape, centering on advance would shove the
	// shape right and clip its right edge at the cell boundary.
	int32 x0;
	 // COLOR: physically center the black box (x0 = (64-w)/2). out.left is
	 // the glyph's absolute position in the cell DIB; adding penX double-
	 // shifts it right and clips the right edge (vertical cut observed).
	 if(out.isColor)
		x0 = (CHS_SLOT_PIXELS - out.w) / 2;
	 else {
		int32 penX = (CHS_SLOT_PIXELS - out.advance) / 2;
		if(penX < 0)
			penX = 0;
		x0 = penX + out.left;
	 }
	 if(x0 < 0)
		x0 = 0;
	// The engine draws with the cell baseline at gBaseY[set] (same baseline as
	// the GDI/stb paths) and reports the black box relative to the cell
	// top-left, so row placement is direct absolute cell coordinates.
	int32 y0 = out.top;

	if(out.isColor) {
		// Color glyph (COLR/CPAL emoji): color buffer is RGBA (r,g,b,a);
		// convert to the D3D9 C8888 (BGRA) byte order and flag the slot.
		for(int32 y = 0; y < out.h; y++) {
			int32 dy = y0 + y;
			if(dy < 0 || dy >= CHS_SLOT_PIXELS)
				continue;
			for(int32 x = 0; x < out.w; x++) {
				int32 dx = x0 + x;
				if(dx < 0 || dx >= CHS_SLOT_PIXELS)
					continue;
				const uint8 *src = color.data() + ((size_t)y * out.w + x) * 4;
				uint8 *dst = px + (oy + dy) * dstStride + (ox + dx) * 4;
				dst[0] = src[2]; // B
				dst[1] = src[1]; // G
				dst[2] = src[0]; // R
				dst[3] = src[3]; // A
			}
		}
		gSlotColor[page][row * CHS_GRID_SIZE + col] = true;
	} else {
		for(int32 y = 0; y < out.h; y++) {
			int32 dy = y0 + y;
			if(dy < 0 || dy >= CHS_SLOT_PIXELS)
				continue;
			for(int32 x = 0; x < out.w; x++) {
				int32 dx = x0 + x;
				if(dx < 0 || dx >= CHS_SLOT_PIXELS)
					continue;
				uint8 v = alpha[(size_t)y * out.w + x];
				uint8 *dst = px + (oy + dy) * dstStride + (ox + dx) * 4;
				dst[0] = v;
				dst[1] = v;
				dst[2] = v;
				dst[3] = v;
			}
		}
		gSlotColor[page][row * CHS_GRID_SIZE + col] = false;
	}
	UnlockAtlasForGlyph(ras);
	return true;
}

// Render supplementary-plane codepoints (>0xFFFF, e.g. CJK ext / Emoji) via stb_truetype.
// Monochrome alpha bitmap, centered in the 64px cell.
static bool
RenderGlyphStb(uint32 chr, int32 page, int32 row, int32 col)
{
	if(gRareFonts.empty()) {
		ChsLog("  stb: rare font NOT ready, cp U+%X", chr);
		return false;
	}

	size_t fi = gRareFonts.size();
	for(size_t i = 0; i < gRareFonts.size(); i++) {
		if(stbtt_FindGlyphIndex(&gRareFonts[i].info, (int)chr) > 0) {
			fi = i;
			break;
		}
	}
	if(fi >= gRareFonts.size()) {
		ChsLog("  stb: U+%X not found in any rare font", chr);
		return false;
	}
	stbtt_fontinfo &font = gRareFonts[fi].info;
	int glyph = stbtt_FindGlyphIndex(&font, (int)chr);
	ChsLog("  stb: U+%X font=%d glyph=%d", chr, (int)fi, glyph);
	if(glyph <= 0)
		return false; // .notdef or missing: render nothing instead of a box
	float scale = stbtt_ScaleForMappingEmToPixels(&font, (float)gGlyphHeight); // match GDI em size (CreateFont -gGlyphHeight)
	int w = 0, h = 0, xoff = 0, yoff = 0;
	uint8 *bmp = stbtt_GetCodepointBitmap(&font, scale, scale, (int)chr, &w, &h, &xoff, &yoff);
	ChsLog("  stb: bmp %dx%d (xoff=%d yoff=%d)", w, h, xoff, yoff);
	if(!bmp || w <= 0 || h <= 0) {
		if(bmp)
			stbtt_FreeBitmap(bmp, nil);
		return false;
	}


	RwRaster *ras = RwTextureGetRaster(CHSFont::SpriteC[page].m_pTexture);
	uint8 *px = LockAtlasForGlyph(ras);
	if(!px) {
		stbtt_FreeBitmap(bmp, nil);
		return false;
	}
	const int32 dstStride = CHS_PAGE_PIXELS * 4;
	int32 ox = col * CHS_SLOT_PIXELS;
	int32 oy = row * CHS_SLOT_PIXELS;
	// Horizontal: center the font's advance cell, then place the glyph at its
	// designed position inside it. Same rule as GDI (penX + glyph offset).
	int advance = 0, lsb = 0;
	stbtt_GetCodepointHMetrics(&font, (int)chr, &advance, &lsb);
	int32 penX = (CHS_SLOT_PIXELS - (int32)(advance * scale)) / 2;
	if(penX < 0)
		penX = 0;
	int32 x0 = penX + xoff;
	// Vertical: align to the same baseline GDI uses (gBaseY[set]); stb yoff is
	// the glyph top offset from the baseline (negative = above baseline).
	int32 setIdx = (page >= CHS_SLANT_BASE) ? 1 : 0;
	int32 y0 = gBaseY[setIdx] + yoff;

	for(int32 y = 0; y < h; y++) {
		int32 dy = y0 + y;
		if(dy < 0 || dy >= CHS_SLOT_PIXELS)
			continue;
		for(int32 x = 0; x < w; x++) {
			int32 dx = x0 + x;
			if(dx < 0 || dx >= CHS_SLOT_PIXELS)
				continue;
					uint8 a = bmp[y * w + x]; // stb single-channel alpha
			uint8 *dst = px + (oy + dy) * dstStride + (ox + dx) * 4;
			dst[0] = a;
			dst[1] = a;
			dst[2] = a;
			dst[3] = a;
		}
	}
	UnlockAtlasForGlyph(ras);
	stbtt_FreeBitmap(bmp, nil);
	return true;
}

// Load one supplementary-plane font file (stb_truetype) and append it to the
// fallback list used by RenderGlyphStb.
static bool
InitRareFont(const std::wstring &absPath)
{
	RareFont rf;
	FILE *f = _wfopen(absPath.c_str(), L"rb");
	if(!f) {
		ChsLog("  rare font: cannot open file");
		ChsLogW(absPath.c_str());
		return false;
	}
	fseek(f, 0, SEEK_END);
	long sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	if(sz <= 0) {
		fclose(f);
		return false;
	}
	rf.data.resize((size_t)sz);
	if(fread(rf.data.data(), 1, (size_t)sz, f) != (size_t)sz) {
		fclose(f);
		return false;
	}
	fclose(f);

	gRareFonts.push_back(rf);
	RareFont &fr = gRareFonts.back();
	int offset = stbtt_GetFontOffsetForIndex(fr.data.data(), 0);
	if(offset < 0 || !stbtt_InitFont(&fr.info, fr.data.data(), offset)) {
		ChsLog("  rare font: stb init failed (offset=%d)", offset);
		gRareFonts.pop_back();
		return false;
	}
	ChsLog("  rare font (stb) loaded:");
	ChsLogW(absPath.c_str());
	return true;
}

// Pick the first font from the candidate list that actually exists with the
// requested weight and can rasterize a CJK glyph.
// weight is FW_NORMAL or FW_BOLD; the real weight is verified via
// TEXTMETRIC.tmWeight because GDI may silently fall back to a regular face
// (e.g. Microsoft YaHei Bold lives in a separate msyhbd.ttc file, and
// GetObject would just echo back the requested LOGFONT).
static bool
PickFont(const std::vector<ChsFontItem> &items, int32 weight)
{
	MAT2 mat = MakeIdentityMat2();
	for(size_t i = 0; i < items.size(); i++) {
		if(items[i].faceName.empty())
			continue; // file entry failed to load
		HFONT f = CreateFontW(-gGlyphHeight, 0, 0, 0, weight, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
			DEFAULT_PITCH | FF_DONTCARE, items[i].faceName.c_str());
		if(!f)
			continue;
		SelectObject(gHdc[0], f);

		// verify the face is really bold when we asked for bold
		TEXTMETRIC tm;
		memset(&tm, 0, sizeof(tm));
		if(!GetTextMetrics(gHdc[0], &tm)) {
			DeleteObject(f);
			continue;
		}
		if(weight >= FW_BOLD && tm.tmWeight < FW_SEMIBOLD) {
			DeleteObject(f);
			continue;
		}

		// confirm it can actually rasterize a CJK glyph
		GLYPHMETRICS gm;
		memset(&gm, 0, sizeof(gm));
		DWORD sz = GetGlyphOutlineW(gHdc[0], 0x4F60 /* CJK glyph probe */, GGO_GRAY8_BITMAP, &gm, 0, nil, &mat);
		if(sz == GDI_ERROR || gm.gmBlackBoxX == 0 || gm.gmBlackBoxY == 0) {
			DeleteObject(f);
			continue;
		}

		gFont[0] = f;
		return true;
	}
	return false;
}

// Read the family name (name table, nameID=1) directly from a TTF file.
// More reliable than diffing EnumFontFamiliesExW results, which failed to
// surface the new face for this font.
static bool
ReadFontFaceName(const wchar_t *path, std::wstring &out)
{
	FILE *f = _wfopen(path, L"rb");
	if(!f)
		return false;

	uint8 hdr[12];
	if(fread(hdr, 1, 12, f) != 12) {
		fclose(f);
		return false;
	}
	uint16 numTables = (uint16)((hdr[4] << 8) | hdr[5]);

	uint32 nameOff = 0;
	for(int i = 0; i < numTables; i++) {
		uint8 rec[16];
		if(fread(rec, 1, 16, f) != 16)
			break;
		if(memcmp(rec, "name", 4) == 0) {
			nameOff = ((uint32)rec[8] << 24) | ((uint32)rec[9] << 16) | ((uint32)rec[10] << 8) | rec[11];
			break;
		}
	}
	if(!nameOff || fseek(f, nameOff, SEEK_SET) != 0) {
		fclose(f);
		return false;
	}

	uint8 nh[6];
	if(fread(nh, 1, 6, f) != 6) {
		fclose(f);
		return false;
	}
	uint16 count = (uint16)((nh[2] << 8) | nh[3]);
	uint16 strOff = (uint16)((nh[4] << 8) | nh[5]);

	// prefer Windows Unicode (platform 3, encoding 1, language 0x409)
	bool found = false;
	uint16 len = 0, off = 0;
	for(int i = 0; i < count; i++) {
		uint8 nr[12];
		if(fread(nr, 1, 12, f) != 12)
			break;
		uint16 platform = (uint16)((nr[0] << 8) | nr[1]);
		uint16 encoding = (uint16)((nr[2] << 8) | nr[3]);
		uint16 lang = (uint16)((nr[4] << 8) | nr[5]);
		uint16 nameID = (uint16)((nr[6] << 8) | nr[7]);
		if(nameID == 1 && platform == 3 && (encoding == 1 || encoding == 0) && lang == 0x409) {
			len = (uint16)((nr[8] << 8) | nr[9]);
			off = (uint16)((nr[10] << 8) | nr[11]);
			found = true;
			break;
		}
	}
	// fallback: any platform-3 nameID-1 record
	if(!found) {
		fseek(f, nameOff + 6, SEEK_SET);
		for(int i = 0; i < count; i++) {
			uint8 nr[12];
			if(fread(nr, 1, 12, f) != 12)
				break;
			uint16 platform = (uint16)((nr[0] << 8) | nr[1]);
			uint16 nameID = (uint16)((nr[6] << 8) | nr[7]);
			if(nameID == 1 && platform == 3) {
				len = (uint16)((nr[8] << 8) | nr[9]);
				off = (uint16)((nr[10] << 8) | nr[11]);
				found = true;
				break;
			}
		}
	}
	if(!found) {
		fclose(f);
		return false;
	}

	fseek(f, nameOff + strOff + off, SEEK_SET);
	std::vector<uint8> buf(len);
	if(len == 0 || fread(buf.data(), 1, len, f) != len) {
		fclose(f);
		return false;
	}
	fclose(f);

	// UTF-16BE -> wstring
	out.clear();
	for(uint32 i = 0; i + 1 < len; i += 2)
		out.push_back((wchar_t)((buf[i] << 8) | buf[i + 1]));
	return !out.empty();
}

// Load the slant (FONT_BANK) font from its configured entry (family name or
// file path, already loaded by MakeFontItem). Returns false on failure.
static bool
LoadSlantFont(const ChsFontItem &item)
{
	if(item.faceName.empty()) {
		ChsLog("  slant entry unavailable");
		return false;
	}

	ChsLog("  slant face:");
	ChsLogW(item.faceName.c_str());
	MAT2 mat = MakeIdentityMat2();
	HFONT f = CreateFontW(-gGlyphHeight, 0, 0, 0, (gSlantWeight > 0) ? FW_NORMAL : (gSlantBold ? FW_BOLD : FW_NORMAL), FALSE, FALSE, FALSE,
		DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
		DEFAULT_PITCH | FF_DONTCARE, item.faceName.c_str());
	if(!f) {
		ChsLog("  CreateFontW failed");
		return false;
	}
	SelectObject(gHdc[1], f);

	// confirm GDI really selected our face and did not fall back
	wchar_t actual[LF_FACESIZE];
	memset(actual, 0, sizeof(actual));
	GetTextFaceW(gHdc[1], LF_FACESIZE, actual);
	ChsLog("  actual face after select:");
	ChsLogW(actual);
	if(wcsstr(actual, item.faceName.c_str()) == nil) {
		ChsLog("  face mismatch, GDI fell back");
		DeleteObject(f);
		return false;
	}

	GLYPHMETRICS gm;
	memset(&gm, 0, sizeof(gm));
	DWORD sz = GetGlyphOutlineW(gHdc[1], 0x4F60 /* CJK glyph probe */, GGO_GRAY8_BITMAP, &gm, 0, nil, &mat);
	if(sz == GDI_ERROR || gm.gmBlackBoxX == 0 || gm.gmBlackBoxY == 0) {
		ChsLog("  glyph test failed, face may have fallen back");
		DeleteObject(f);
		return false;
	}

	gFont[1] = f;
	ChsLog("  slant font OK:");
	ChsLogW(item.faceName.c_str());
	return true;
}

bool
CHSFont::Init(void)
{
	Shutdown(); // rebuild from scratch

	// fresh log BEFORE loading config so config-side logs are kept
	FILE *lf = fopen("chsfont.log", "w");
	if(lf)
		fclose(lf);

	LoadFontConfig();

	ChsLog("CHSFont::Init (glyph height %d, weights n%d/s%d/r%d)", gGlyphHeight, gNormalWeight, gSlantWeight, gRareWeight);

	// [Fonts] TextRenderer=TXD: keep the dynamic engine OFF entirely (no
	// HDC/fonts/atlas pages, gInited stays false). Returning false is the
	// exact signal Font.cpp tests to fall back to the static CHINESE.TXD
	// atlas (ReadTable + LoadTxd + Sprite_C normal/slant textures).
	if(gTextRenderer == CHS_TEXT_TXD) {
		ChsLog("  TextRenderer=TXD: dynamic engine off, static CHINESE.TXD in use");
		return false;
	}

	for(int32 set = 0; set < 2; set++) {
		gHdc[set] = CreateCompatibleDC(nil);
		if(!gHdc[set]) {
			Shutdown();
			return false;
		}
	}

	// normal set: prefer a real bold face (Microsoft YaHei Bold is a
	// separate msyhbd.ttc file); fall back to regular weight if needed.

	if(gNormalWeight > 0) {
		// The weight comes from the variable-font wght axis (DWrite), so
		// the normal face must be the variable Regular file (e.g. msyh.ttc),
		// NOT the static Bold file (msyhbd.ttc has no axis).
		if(!PickFont(gNormalItems, FW_NORMAL)) {
			Shutdown();
			return false;
		}
	} else if(gNormalBold) {
		if(!PickFont(gNormalItems, FW_BOLD)) {
			if(!PickFont(gNormalItems, FW_NORMAL)) {
				Shutdown();
				return false;
			}
		}
	} else {
		if(!PickFont(gNormalItems, FW_NORMAL)) {
			Shutdown();
			return false;
		}
	}
	wchar_t face[LF_FACESIZE];
	memset(face, 0, sizeof(face));
	GetTextFaceW(gHdc[0], LF_FACESIZE, face);
	ChsLog("normal font:");
	ChsLogW(face);

	// slant set: configured font (family name or file path); optional, fall
	// back to the normal font if it is missing or cannot be loaded.
	if(!LoadSlantFont(gSlantItem)) {
		ChsLog("slant font FAILED, falling back to normal");
		SelectObject(gHdc[1], gFont[0]);
		gFont[1] = gFont[0];
	}

	// baseline position inside the 64px cell, proportional to each font's
	// ascent/descent ratio so glyphs and punctuation share one baseline
	for(int32 set = 0; set < 2; set++) {
		TEXTMETRIC tm;
		memset(&tm, 0, sizeof(tm));
		if(GetTextMetrics(gHdc[set], &tm) && tm.tmAscent + tm.tmDescent > 0)
			gBaseY[set] = CHS_SLOT_PIXELS * tm.tmAscent / (tm.tmAscent + tm.tmDescent);
		else
			gBaseY[set] = (CHS_SLOT_PIXELS * 3) / 4;
	}

	// Start the DirectWrite backend (optional) now that both HDCs have their
	// fonts selected. When disabled or unavailable, gDwOk stays false and the
	// GDI path is used (the pre-DWrite behavior).
	if(gTextRenderer == CHS_TEXT_DWRITE) {
		gDwOk = ChsDwInit(gHdc[0], gHdc[1]);
		// Per-set weights from [Fonts] NormalWeight / SlantWeight / RareWeight:
		// re-bind each main face through the variable wght axis, and set the
		// rare chain weight. Must run BEFORE the chain loads so rare faces
		// pick it up too.
		if(gDwOk && gNormalWeight > 0)
			ChsDwSetWeight(0, (float)gNormalWeight);
		if(gDwOk && gSlantWeight > 0)
			ChsDwSetWeight(1, (float)gSlantWeight);
		if(gDwOk && gRareWeight > 0)
			ChsDwSetRareWeight((float)gRareWeight);
		// Supplementary-plane fallback: hand the same RareFontFile list to
		// DWrite so CJK-ext / emoji codepoints render in color.
		if(gDwOk && !gRawRareFont.empty())
			ChsDwLoadRareFonts(gRawRareFont.c_str());
	} else
		gDwOk = false;
	gSlotMap.clear();
	memset(gSlotColor, 0, sizeof(gSlotColor)); // reset color flags
	gPageCount[0] = gPageCount[1] = 0;
	gCurrentPageSlots[0] = gCurrentPageSlots[1] = 0;
	if(!CreateNewPage(false) || !CreateNewPage(true)) {
		Shutdown();
		return false;
	}
	gInited = true;
	ChsLog("CHSFont::Init done");
	return true;
}

void
CHSFont::Shutdown(void)
{
	FinishAtlasWriteBatch();
	gAtlasWriteBatch = false;
	gInited = false;
	gSlotMap.clear();
	memset(gSlotColor, 0, sizeof(gSlotColor)); // reset color flags
	// Page indices are NOT contiguous: normal pages use 0..CHS_PAGES_PER_SET-1
	// and slant pages use CHS_SLANT_BASE..CHS_MAX_PAGES-1, so iterate every
	// slot in the array instead of relying on NumPages (a running count).
	for(int32 i = 0; i < CHS_MAX_PAGES; i++) {
		RwTexture *tex = CHSFont::SpriteC[i].m_pTexture;
		if(tex) {
			// librw's Texture::destroy() also destroys the raster it owns
			// (texture->raster->destroy()), so RwTextureDestroy (via Delete)
			// already frees the raster. Calling RwRasterDestroy here as well
			// would be a double free.
			CHSFont::SpriteC[i].Delete(); // destroys the texture + its raster
		}
		CHSFont::SpriteC[i].m_pTexture = nil;
	}
	CHSFont::NumPages = 0;
	gPageCount[0] = gPageCount[1] = 0;
	gCurrentPageSlots[0] = gCurrentPageSlots[1] = 0;

	for(int32 set = 0; set < 2; set++) {
		// gFont[1] may alias gFont[0]; only delete the real font objects
		if(gFont[set] && !(set == 1 && gFont[1] == gFont[0]))
			DeleteObject(gFont[set]);
		gFont[set] = nil;
		if(gHdc[set]) {
			DeleteDC(gHdc[set]);
			gHdc[set] = nil;
		}
	}
	// unload every font file we loaded privately
	for(size_t i = 0; i < gLoadedFonts.size(); i++)
		RemoveFontResourceExW(gLoadedFonts[i].first.c_str(), FR_PRIVATE, 0);
	gLoadedFonts.clear();
	// release stb rare font state
	gRareFonts.clear();
	// release the DirectWrite backend (optional)
	ChsDwShutdown();
	gDwOk = false;

}

bool
CHSFont::Inited(void)
{
	return gInited;
}

void
CHSFont::EnsureConfig(void)
{
	EnsureFontsIni();
}

const CharPos &
CHSFont::GetSlot(uint32 chr, bool slant)
{
	if(!gInited || chr == 0)
		return gFallbackSlot;

	// key encoding: cp<<1 | slant (cp max 0x10FFFF, no overflow/collision).
	uint32 key = (chr << 1) | (slant ? 1u : 0u);
	std::unordered_map<uint32, CharPos>::iterator it = gSlotMap.find(key);
	if(it != gSlotMap.end())
		return it->second;

	int32 set = slant ? 1 : 0;
	// not cached yet: allocate a cell and rasterize
	if(gCurrentPageSlots[set] >= CHS_PAGE_SLOTS) {
		if(!CreateNewPage(slant))
			return gFallbackSlot;
	}

	int32 page = (slant ? CHS_SLANT_BASE : 0) + gPageCount[set] - 1;
	int32 idx = gCurrentPageSlots[set]++;
	CharPos pos;
	pos.page = (unsigned char)page;
	pos.rowIndex = (unsigned char)(idx / CHS_GRID_SIZE);
	pos.columnIndex = (unsigned char)(idx % CHS_GRID_SIZE);

	// Rasterize the glyph into the cell. Even on failure we keep the (empty)
	// cell registered so we don't retry the failed codepoint every frame.
	RenderGlyph(chr, slant, pos.page, pos.rowIndex, pos.columnIndex);

	return gSlotMap.emplace(key, pos).first->second;
}

void
CHSFont::Preload(const wchar *textData, int textLength)
{
	if(!gInited || textData == nil || textLength <= 0)
		return;

	DWORD started = GetTickCount();
	int before = (int)gSlotMap.size();
	gAtlasWriteBatch = true;
	for(int i = 0; i < textLength; i++) {
		uint32 cp = (uint16)textData[i];
		if(cp >= 0xD800 && cp <= 0xDBFF && i + 1 < textLength) {
			uint32 low = (uint16)textData[i + 1];
			if(low >= 0xDC00 && low <= 0xDFFF) {
				cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
				i++;
			}
		}
		if(cp < 0x80 || cp == 0x200C || cp == 0x200D ||
		   (cp >= 0xFE00 && cp <= 0xFE0F))
			continue;
		GetSlot(cp, false); // subtitles use the normal CJK set
	}
	FinishAtlasWriteBatch();
	gAtlasWriteBatch = false;
	int added = (int)gSlotMap.size() - before;
	if(added > 0)
		ChsLog("CHSFont::Preload: %d glyphs in %lu ms", added,
		       (unsigned long)(GetTickCount() - started));
}

bool
CHSFont::IsSlotColor(uint32 chr, bool slant)
{
	if(!gInited || chr == 0)
		return false;
	uint32 key = (chr << 1) | (slant ? 1u : 0u);
	std::unordered_map<uint32, CharPos>::iterator it = gSlotMap.find(key);
	if(it == gSlotMap.end())
		return false; // slot not rasterized yet: no color info
	const CharPos &pos = it->second;
	if(pos.page >= CHS_MAX_PAGES)
		return false;
	return gSlotColor[pos.page][pos.rowIndex * CHS_GRID_SIZE + pos.columnIndex];
}

// Merge the current [Fonts] values into a mINI structure. Called from
// re3's SaveINISettings before it writes the file back, so the section is
// kept (mINI drops sections that are not present in the structure being
// written) and the values stay in sync with what CHSFont is using.
void
ChsFontSaveIniValues(mINI::INIStructure &cfg)
{
	mINI::INIMap<std::string> &sec = cfg["Fonts"];
	sec.set("NormalFonts", gRawNormalFonts);
	sec.set("NormalBold", gNormalBold ? "1" : "0");
	sec.set("SlantBold", gSlantBold ? "1" : "0");
	sec.set("SlantFontFile", gRawSlantFont);
	sec.set("RareFontFile", gRawRareFont);
	char tmp[16];
	snprintf(tmp, sizeof(tmp), "%d", gGlyphHeight);
	sec.set("GlyphHeight", tmp);
	sec.set("TextRenderer",
	        gTextRenderer == CHS_TEXT_TXD ? "1" :
	        gTextRenderer == CHS_TEXT_GDI ? "2" : "3");
	snprintf(tmp, sizeof(tmp), "%d", gNormalWeight);
	sec.set("NormalWeight", tmp);
	snprintf(tmp, sizeof(tmp), "%d", gSlantWeight);
	sec.set("SlantWeight", tmp);
	snprintf(tmp, sizeof(tmp), "%d", gRareWeight);
	sec.set("RareWeight", tmp);
}

#endif // _WIN32
