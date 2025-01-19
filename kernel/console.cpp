#include "console.hpp"

#include <cstring>
#include "font.hpp"

Console::Console(PixelWriter& writer, const PixelColor& fg_color, const PixelColor& bg_color)
	: writer_{writer}, fg_color_{fg_color}, bg_color_{bg_color}, buffer_{}, cursor_row_{0}, cursor_column_{0}{
}

void Console::PutString(const char* s) {
	// in while condition of c, 0 is false. so \0 of null character evaluate as 0 and judge as 0
	// about null character, in c compiler automatically add null character to string literal such as ""
	while(*s) {
		if (*s == '\n') {
			Newline();
		} else if (cursor_column_ < kColumns -1) {
			WriteAscii(writer_, 8 * cursor_column_, 16 * cursor_row_, *s, fg_color_);
			buffer_[cursor_row_][cursor_column_] = *s;
			++cursor_column_;
		}
		// ++ means add size of type to address. in this case type is char, so s += 1 byte. this operator can only use to pointer. 
		++s;
	}
}

void Console::Newline() {
	cursor_column_ = 0;

	if (cursor_row_ < kRows - 1) {
		++cursor_row_;
	} else {
		for (int y = 0; y < 16 * kRows; ++y) {
			for (int x = 0; x < 8 * kColumns; ++x) {
				writer_.Write(x, y, bg_color_);
			}
		}

		for (int row = 0; row < kRows -1; ++row) {
			// first parameter as destination, second parameter as source, third parameter as size
			// buffer_ is two dimensional array. so buffer_[row] means array. and buffer_[row] is pointer because it is array. 
			memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
			WriteString(writer_, 0, 16 * row, buffer_[row], fg_color_);
		}

		// first parameter as destination, second parameter as string to fill up, third parameter as size
		// initialize last line with filling up by 0
		memset(buffer_[kRows - 1], 0, kColumns + 1);
	}
}
