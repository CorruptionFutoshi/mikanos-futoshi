#pragma once

#include "graphics.hpp"

class Console {
	public:
		static const int kRows = 45, kColumns = 80;

		Console(const PixelColor& fg_color, const PixelColor& bg_color);
		void PutString(const char* s);
		void SetWriter(PixelWriter* writer);

	private:
		void Newline();
		void Refresh();

		PixelWriter* writer_;
		const PixelColor fg_color_, bg_color_;
		// set kColumns + 1 to save null character at end of column
		char buffer_[kRows][kColumns + 1];
		int cursor_row_, cursor_column_;
};
