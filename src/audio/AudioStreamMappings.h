#pragma once

#include <cstdio>
#include <cstring>
#include <string>

// Audio Samples uses ordered bare filenames, not key=value INI entries.
template <size_t Count, size_t Width>
static bool
ReadAudioStreamMappings(FILE *file, char (&names)[Count][Width], bool (&mapped)[Count], bool (*exists)(const char *))
{
	static char defaults[Count][Width];
	static bool captured = false;
	if(!captured) {
		memcpy(defaults, names, sizeof(defaults));
		captured = true;
	}
	memcpy(names, defaults, sizeof(defaults));
	memset(mapped, 0, sizeof(mapped));
	if(!file) return false;
	bool inSamples = false, changed = false;
	size_t index = 0;
	std::string line;
	for(;;) {
		line.clear();
		int c;
		bool tooLong = false;
		while((c = fgetc(file)) != EOF && c != '\n') {
			if(c == '\r') continue;
			if(line.size() < 4096)
				line += (char)c;
			else
				tooLong = true;
		}
		if(line.compare(0, 3, "\xEF\xBB\xBF") == 0) line.erase(0, 3);
		const size_t start = line.find_first_not_of(" \t");
		if(start != std::string::npos) {
			line = line.substr(start, line.find_last_not_of(" \t") - start + 1);
			if(line[0] == '[') {
				std::string section = line;
				for(char &ch : section)
					if(ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
				inSamples = section == "[samples]";
			} else if(inSamples && line[0] != ';' && line[0] != '#' && index < Count) {
				const size_t entry = index++;
				for(char &ch : line)
					if(ch == '\\') ch = '/';
				// Invalid entries still consume their slot; never shift later IDs.
				if(!tooLong && line.size() < Width && line.size() >= 5 && line[0] != '/' && line.find(':') == std::string::npos &&
				   line.find("..") == std::string::npos && line.find('=') == std::string::npos && line.find('\0') == std::string::npos &&
				   exists(line.c_str())) {
					memcpy(names[entry], line.c_str(), line.size() + 1);
					mapped[entry] = true;
					changed = true;
				}
			}
		}
		if(c == EOF) break;
	}
	return changed;
}
