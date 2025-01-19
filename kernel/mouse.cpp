#include "mouse.hpp"

#include "graphics.hpp"

namespace {
	const int kMouseCursorWidth = 15;
	const int kMouseCursorHeight = 24;
	const char mouse_cursor_shape[kMouseCursorHeight][kMouseCursorWidth + 1] = {
		"@              ",
		"@@             ",
		"@.@            ",
		"@..@           ",
		"@...@          ",
		"@....@         ",
		"@.....@        ",
		"@......@       ",
		"@.......@      ",
		"@........@     ",
		"@.........@    ",
		"@..........@   ",
		"@...........@  ",
		"@............@ ",
		"@......@@@@@@@@",
		"@......@       ",
		"@....@@.@      ",
		"@...@ @.@      ",
		"@..@   @.@     ",
		"@.@    @.@     ",
		"@@      @.@    ",
		"@       @.@    ",
		"         @.@   ",
		"         @@@   ",
	};		

	void DrawMouseCursor(PixelWriter* pixel_writerptr, Vector2D<int> position) {
		for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
			for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
				if (mouse_cursor_shape[dy][dx] == '@') {
					pixel_writerptr->Write(position.x + dx, position.y + dy, {0, 0, 0});
				} else if (mouse_cursor_shape[dy][dx] == '.') {
					pixel_writerptr->Write(position.x + dx, position.y + dy, {255, 255, 255});
				}
			}
		}
	}

	void EraseMouseCursor(PixelWriter* pixel_writerptr, Vector2D<int> position, PixelColor erase_color) {	
		for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
			for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
				// in c++, '' don't represent empty. ' ' represent empty.
				if(mouse_cursor_shape[dy][dx] != ' ') {
					pixel_writerptr->Write(position.x + dx, position.y + dy, erase_color);
				}
			}	
		}
	}
}

MouseCursor::MouseCursor(PixelWriter* writerptr, PixelColor erase_color, Vector2D<int> initial_position) : pixel_writerptr_{writerptr}, erase_color_{erase_color}, position_{initial_position} {
	DrawMouseCursor(pixel_writerptr_, position_);
}

void MouseCursor::MoveRelative(Vector2D<int> displacement) {
	EraseMouseCursor(pixel_writerptr_, position_, erase_color_);
	position_ += displacement;
	DrawMouseCursor(pixel_writerptr_, position_);
}



