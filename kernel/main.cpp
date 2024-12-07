#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"

// this is called placement new. it allocate memory area specified by parameter.
void* operator new(size_t size, void* buf){
	return buf;
}

// ~PixelWriter() require this operator
void operator delete(void* obj) noexcept {
}

// in c++, char is 1 byte. so memory area of array of char that have n contents equalls memory area of n byte 
char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writerptr;

char console_buf[sizeof(Console)];
Console* consoleptr;

// ... means variable-length parameters
int printk(const char* format, ...) {
	// variable store variable-length parameters
	va_list ap;
	int result;
	char s[1024];

	// va_start() method represent proccess before using variable-length parameters
	va_start(ap, format);
	result = vsprintf(s, format, ap);
	// va_end() method represent process after using variable-length parameters
	va_end(ap);
	
	consoleptr->PutString(s);
	return result;
}

extern "C" void KernelMain(const FrameBufferConfig& frame_buffer_config){
	switch(frame_buffer_config.pixel_format) {
		case kPixelRGBResv8BitPerColor:
			// new operator seems to require two parameter, but size_t is not need. new calcurate size of type automatically. so it is enough to set parameter buffer. {frame_buffer_config} is required by constructor. {} is uniform initialization
			pixel_writerptr = new(pixel_writer_buf)RGBResv8BitPerColorPixelWriter{frame_buffer_config};
			break;
		case kPixelBGRResv8BitPerColor:
			pixel_writerptr = new(pixel_writer_buf)BGRResv8BitPerColorPixelWriter{frame_buffer_config};
			break;
	}

	for (int x = 0; x < frame_buffer_config.horizontal_resolution; ++x){
		for (int y = 0; y < frame_buffer_config.vertical_resolution; ++y){
			pixel_writerptr->Write(x, y, {255, 255, 255});
		}
	}

	consoleptr = new(console_buf) Console{*pixel_writerptr, {0, 0, 0}, {255, 255, 255}};

	for (int i = 0; i < 27; ++i) {
		printk("printk: %d\n", i);
	}

	WriteString(*pixel_writerptr, 0, 66, "Saiko_no_Egao de Kirinukeruyo", {0, 0, 255});

	while (1) __asm__("hlt");
}


	// to manipulate memory address per byte, use uint8_t. if use uint64_t, manipulate per bit. because frame_buffer_size is per byte and frame buffer is basically per byte, so we need to cast frame_buffer_base to represent uint8_t.
	// uint8_t* frame_bufferptr = reinterpret_cast<uint8_t*>(frame_buffer_base);

	// frame_buffer_size have potential to become too big number, so declare as uint64_t
	// for(uint64_t i = 0; i < frame_buffer_size; ++i){
		// pointer + [] means value. in pointer + [], 1 increment means proceed sizeof type. in this case frame_bufferptr is uint8_t, so 1 increment means 8 bit, equall 1 byte, proceed. by the way, in c, (pointer + 1) means pointer + sizeof value type specified by pointer.
	//	frame_bufferptr[i]= i % 256;
	//}
