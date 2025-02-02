#include "graphics.hpp"

void RGBResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c){
	auto p = PixelAt(pos);
	p[0] = c.r;
	p[1] = c.g;
	p[2] = c.b;
}


void BGRResv8BitPerColorPixelWriter::Write(Vector2D<int> pos, const PixelColor& c){
	auto p = PixelAt(pos);
	p[0] = c.b;
	p[1] = c.g;
	p[2] = c.r;
}

void DrawRectangle(PixelWriter& writer, const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c) {
	for (int dx = 0; dx < size.x; ++dx) {
		writer.Write(pos + Vector2D<int>{dx, 0}, c);
		// - 1 to draw inside specified area. if don't -1, rectangle is 1 pixel bigger than specified area. if position (1,1) and size (3,4), y of above line is 1, y of below line is 4. so y of drawn area is 4(pixel1, pixel2, pixel3, pixel4).
		writer.Write(pos + Vector2D<int>{dx, size.y - 1}, c);
	}

	// start with 1 because place dy =0 is already  drawn. < size.y -1 too
	for (int dy = 1; dy < size.y - 1; ++dy) {
		writer.Write(pos + Vector2D<int>{0, dy}, c);
		writer.Write(pos + Vector2D<int>{size.x - 1, dy}, c);
	}
}

void FillRectangle(PixelWriter& writer, const Vector2D<int>& pos, const Vector2D<int>& size, const PixelColor& c) {
	for (int dy = 0; dy < size.y; ++dy) {
		for (int dx = 0; dx < size.x; ++dx) {
			writer.Write(pos + Vector2D<int>{dx, dy}, c);
		}
	}
}

void DrawDesktop(PixelWriter& writer) {
	const auto width = writer.Width();
	const auto height = writer.Height();

	FillRectangle(writer, {0, 0}, {width, height - 50}, kDesktopBGColor);	
	FillRectangle(writer, {0, height - 50}, {width, 50}, {1, 8, 17});
	FillRectangle(writer, {0, height - 50}, {width / 5, 50}, {80, 80, 80});
	DrawRectangle(writer, {10, height - 40}, {30, 30}, {160, 160, 160});
}
