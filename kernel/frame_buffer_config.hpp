// .hpp file is header file.  it is used to accessed by other file. it is not general that source file access other source file in c
// #pragma once is include guard. 
#pragma once

#include <stdint.h>

enum PixelFormat {
	kPixelRGBResv8BitPerColor,
	kPixelBGRResv8BitPerColor,
};

struct FrameBufferConfig {
	uint8_t* frame_bufferptr;
	uint32_t pixels_per_scan_line;
	uint32_t horizontal_resolution;
	uint32_t vertical_resolution;
	enum PixelFormat pixel_format;
};
