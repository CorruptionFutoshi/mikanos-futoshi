#pragma once

#include <cstdint>
#include "graphics.hpp"

void WriteAscii(PixelWriter& writer, Vector2D<int> pos, char c, const PixelColor& color);

// writer and color is reference, s is pointer
void WriteString(PixelWriter& writer, Vector2D<int> pos, const char* s, const PixelColor& color);
