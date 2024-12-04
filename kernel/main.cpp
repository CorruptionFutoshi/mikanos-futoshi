#include <cstdint>
#include <cstddef>

#include "frame_buffer_config.hpp"


struct PixelColor {
	uint8_t r, g, b;
};

int WritePixel(const FrameBufferConfig& config, int x, int y, const PixelColor& c){
	const int pixel_position = config.pixels_per_scan_line * y + x;

	if (config.pixel_format == kPixelRGBResv8BitperColor) {
		// in this format, 1 pixel is 4 byte so 4* pixel_position
		uint8_t* p = &config.frame_bufferptr[4 * pixel_position];
		p[0] = c.r;
		p[1] = c.g;
		p[2] = c.b;
	} else if (config.pixel_format == kPixelBGRResv8bitPerColor){
		uint8_t* p = &config.frame_bufferptr[4 * pixel_position];
		p[0] = c.b;
		p[1] = c.g;
		p[2] = c.r;
	} else {
		rerurn -1;
	}

	return 0;
}

extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config){
	for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x){
		for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y){
			WritePixel(frame_buffer_config, x, y, {255, 255, 255});
		}
	}

	for (int x =0; x < 200; ++x) {
		for (int y = 0; y < 100; ++y) {
			WritePixel(frame_buffer_config, 100 + x, 100 + y, {0, 255, 0});
		}
	}

	while (1) __asm__("hlt");
}


	// to manipulate memory address per byte, use uint8_t. if use uint64_t, manipulate per bit. because frame_buffer_size is per byte and frame buffer is basically per byte, so we need to cast frame_buffer_base to represent uint8_t.
	// uint8_t* frame_bufferptr = reinterpret_cast<uint8_t*>(frame_buffer_base);

	// frame_buffer_size have potential to become too big number, so declare as uint64_t
	// for(uint64_t i = 0; i < frame_buffer_size; ++i){
		// pointer + [] means value. in pointer + [], 1 increment means proceed sizeof type. in this case frame_bufferptr is uint8_t, so 1 increment means 8 bit, equall 1 byte, proceed. by the way, in c, (pointer + 1) means pointer + sizeof value type specified by pointer.
	//	frame_bufferptr[i]= i % 256;
	//}
