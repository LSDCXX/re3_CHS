#pragma once
#include <cmath>
#include <string>

namespace MenuMapMath {
inline float Bound(float value, float lo, float hi) { return value < lo ? lo : value > hi ? hi : value; }
struct Point { float x, y; };
struct View {
	float x, y, halfSize;
	Point ToScreen(float wx, float wy) const { return {x + wx * halfSize / 2000.0f, y - wy * halfSize / 2000.0f}; }
	Point ToWorld(float sx, float sy) const { return {(sx - x) * 2000.0f / halfSize, (y - sy) * 2000.0f / halfSize}; }
	void Zoom(float factor, Point anchor, float minimum, float maximum) {
		float size = Bound(halfSize * factor, minimum, maximum);
		float ratio = size / halfSize;
		x += (anchor.x - x) * (1.0f - ratio);
		y += (anchor.y - y) * (1.0f - ratio);
		halfSize = size;
	}
};

// Decode UTF-8 INI labels into re3's 16-bit font strings, without wchar_t casts.
inline std::u16string DecodeUTF8(const std::string &text) {
	std::u16string result;
	for(size_t i = 0; i < text.size();) {
		unsigned char c = (unsigned char)text[i++];
		unsigned int cp = c, minimum = 0;
		int count = 0;
		if(c >= 0xC2 && c <= 0xDF) { cp = c & 31; count = 1; minimum = 0x80; }
		else if(c >= 0xE0 && c <= 0xEF) { cp = c & 15; count = 2; minimum = 0x800; }
		else if(c >= 0xF0 && c <= 0xF4) { cp = c & 7; count = 3; minimum = 0x10000; }
		else if(c >= 0x80) { result += u'?'; continue; }
		bool valid = true;
		for(int j = 0; j < count; j++) {
			if(i >= text.size() || ((unsigned char)text[i] & 0xC0) != 0x80) { valid = false; break; }
			cp = (cp << 6) | ((unsigned char)text[i++] & 63);
		}
		if(!valid || cp < minimum || cp > 0xFFFF || (cp >= 0xD800 && cp <= 0xDFFF)) cp = '?';
		result += (char16_t)cp;
	}
	return result;
}
}
