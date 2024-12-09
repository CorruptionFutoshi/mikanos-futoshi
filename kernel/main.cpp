#include <cstdint>
#include <cstddef>
#include <cstdio>

#include "frame_buffer_config.hpp"
#include "graphics.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"

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

const PixelColor kDesktopBGColor{213, 203, 198};
const PixelColor kDesktopFGColor{81, 55, 67};

// in c++ there is a placement new declaration in default
// this is called placement new. it allocate memory area specified by parameter.
// void* operator new(size_t size, void* buf){
// 	return buf;
// }

// ~PixelWriter() require this operator. noexcept means this method never throw error
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
	
	// k means constant
	const int kFrameWidth = frame_buffer_config.horizontal_resolution;
	const int kFrameHeight = frame_buffer_config.vertical_resolution;

	FillRectangle(*pixel_writerptr, {0, 0}, {kFrameWidth, kFrameHeight - 50}, kDesktopBGColor);
	FillRectangle(*pixel_writerptr, {0, kFrameHeight - 50}, {kFrameWidth, 50}, {179, 173, 160});
	FillRectangle(*pixel_writerptr, {0, kFrameHeight - 50}, {kFrameWidth / 5, 50}, {165, 143, 134});
	DrawRectangle(*pixel_writerptr, {10, kFrameHeight - 40}, {30, 30}, {230, 234, 227});	

	consoleptr = new(console_buf) Console{*pixel_writerptr, kDesktopFGColor, kDesktopBGColor};
	printk("Saiko no Egao de Kirinukeruyo");
	
	for (int dy = 0; dy < kMouseCursorHeight; ++dy) {
		for (int dx = 0; dx < kMouseCursorWidth; ++dx) {
			if (mouse_cursor_shape[dy][dx] == '@') {
				pixel_writerptr->Write(200 + dx, 100 + dy, {0, 0, 0});
			} else if (mouse_cursor_shape[dy][dx] == '.') {
				pixel_writerptr->Write(200 + dx, 100 + dy, {255, 255, 255});
			}
		}
	}

	auto err = pci::ScanAllBus();
	printk("ScanAllBus: %s\n", err.Name());

	for (int i = 0; i < pci::num_function; ++i) {
		const auto& func = pci::functions[i];
		auto vendor_id = pci::ReadVendorId(func.bus, func.device, func.function);
		auto class_code = pci::ReadClassCode(func.bus, func.device, func.function);

		printk("%d.%d.%d: vend %04x, class %08x, head %02x\n", func.bus, func.device, func.function, vendor_id, class_code, func.header_type);
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
