#include "console.hpp"

#include <cstring>
#include "font.hpp"
#include "layer.hpp"

Console::Console(const PixelColor& fg_color, const PixelColor& bg_color)
	: writer_{nullptr}, window_{}, fg_color_{fg_color}, bg_color_{bg_color}, buffer_{}, cursor_row_{0}, cursor_column_{0}{
}

void Console::PutString(const char* s) {
	// in while condition of c, 0 is false. so \0 of null character evaluate as 0 and judge as 0
	// about null character, in c compiler automatically add null character to string literal such as ""
	while(*s) {
		if (*s == '\n') {
			Newline();
		} else if (cursor_column_ < kColumns -1) {
			// in c++, we can set value type object as reference type parameter. there is a implicit conversion.
			WriteAscii(*writer_, Vector2D<int>{8 * cursor_column_, 16 * cursor_row_}, *s, fg_color_);
			buffer_[cursor_row_][cursor_column_] = *s;
			++cursor_column_;
		}
		// ++ means add size of type to address. in this case type is char, so s += 1 byte. this operator can only use to pointer. 
		++s;
	}


	if (layer_manager) {
		layer_manager->Draw();
	}
}

void Console::SetWriter(PixelWriter* writer) {
	if (writer == writer_) {
		return;
	}

	writer_ = writer;
	// reset() is method of std::shared_ptr. it repersent that give up ownership and set nullptr.
	window_.reset();
	Refresh();
}

void Console::SetWindow(const std::shared_ptr<Window>& window) {
	if (window == window_) {
		return;
	}

	window_ = window;
	writer_ = window->Writer();
	Refresh();
}

void Console::Newline() {
	cursor_column_ = 0;

	// cursor_row is 1 smaller than filled real row. if cursor_row is kRows - 1, kRows number of row is filled. 
	if (cursor_row_ < kRows - 1) {
		++cursor_row_;
		return;
	} 
	
	if (window_) {
		// specify {0, 16} to declare src from second row.
		// why kRows - 1 is that it's rectangle's height and remove first row.
		Rectangle<int> move_src{{0, 16}, {8 * kColumns, 16 * (kRows - 1)}};
		window_->Move({0, 0}, move_src);
		// why kRows - 1 is that it's first address.
		FillRectangle(*writer_, {0, 16 * (kRows - 1)}, {8 * kColumns, 16}, bg_color_);
	} else {
		FillRectangle(*writer_, {0, 0}, {8 * kColumns, 16 * kRows}, bg_color_);

		for (int row = 0; row < kRows -1; ++row) {
			// first parameter as destination, second parameter as source, third parameter as size
			// buffer_ is two dimensional array. so buffer_[row] means array. and buffer_[row] is pointer because it is array. 
			memcpy(buffer_[row], buffer_[row + 1], kColumns + 1);
			WriteString(*writer_, Vector2D<int>{0, 16 * row}, buffer_[row], fg_color_);
		}

		// first parameter as destination, second parameter as string to fill up, third parameter as size
		// initialize last line with filling up by 0
		memset(buffer_[kRows - 1], 0, kColumns + 1);
	}
}

void Console::Refresh() {
	for (int row = 0; row < kRows; ++row) {
		WriteString(*writer_, Vector2D<int>{0, 16 * row}, buffer_[row], fg_color_);
	}
}
