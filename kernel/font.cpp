#include "font.hpp"

// extern means this variable is already declared as global variable.
extern const uint8_t _binary_hankaku_bin_start;
extern const uint8_t _binary_hankaku_bin_end;
extern const uint8_t _binary_hankaku_bin_size;

const uint8_t* GetFontPtr(char c) {
	// 16 means byte size of a font of hankaku. fontdata line up without any gap. i dont know why &_binary_hankaku_bin_size represent size of font data.(probably it is initialized in other source file, maybe hankaku.o). auto means var of Java
	auto index = 16 * static_cast<unsigned int>(c);

	if (index >= reinterpret_cast<uintptr_t>(&_binary_hankaku_bin_size)) {
		return nullptr;
	}
	
	// i dont know why &_binary_hankaku_bin_start represent start address of font data
	return &_binary_hankaku_bin_start + index;
}

void WriteAscii(PixelWriter& writer, Vector2D<int> pos, char c, const PixelColor& color) {
	const uint8_t* fontptr = GetFontPtr(c);

	if (fontptr == nullptr) {
		return;
	}

	for (int dy = 0; dy < 16; ++dy) {
		for (int dx = 0; dx <8; ++dx) {
			// 0x80u is 0b1000 0000. u means unsighed (no plus or minus) 
			if ((fontptr[dy] << dx) & 0x80u) {
				writer.Write(pos + Vector2D<int>{dx, dy}, color);
			}
		}
	}
}

void WriteString(PixelWriter& writer, Vector2D<int> pos, const char* s, const PixelColor& color) {
	for (int i = 0; s[i] != '\0'; ++i) {
		WriteAscii(writer, pos + Vector2D<int>{8 * i, 0}, s[i], color);
	}
}
