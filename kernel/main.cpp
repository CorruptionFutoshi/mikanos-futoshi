#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>
#include <deque>
#include <limits>

#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "graphics.hpp"
#include "mouse.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "logger.hpp"
#include "usb/xhci/xhci.hpp"
#include "interrupt.hpp"
#include "asmfunc.h"
#include "segment.hpp"
#include "paging.hpp"
#include "memory_manager.hpp"
#include "window.hpp"
#include "layer.hpp"

// in c++ there is a placement new declaration in default
// this is called placement new. it allocate memory area specified by parameter.
// void* operator new(size_t size, void* buf){
// 	return buf;
// }

// ~PixelWriter() require this operator. noexcept means this method never throw error
// void operator delete(void* obj) noexcept {
// }

// in c++, char is 1 byte. so memory area of array of char that have n contents equalls memory area of n byte 
// char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];

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

console->PutString(s);
return result;
}

std::shared_ptr<Window> main_window;
unsigned int main_window_layer_id;

void InitializeMainWindow() {
	main_window = std::make_shared<Window>(160, 52, screen_config.pixel_format);
	DrawWindow(*main_window->Writer(), "Hello WIndow");

	main_window_layer_id = layer_manager->NewLayer()
		.SetWindow(main_window)
		.SetDraggable(true)
		.Move({300, 100})
		.ID();

	layer_manager->UpDown(main_window_layer_id, std::numeric_limits<int>::max());
}

std::deque<Message>* main_queue;

// alignas guarantee that start address of this variable is multiple of 16.
// as you already know, uint8_t is type of content of array.
alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(const FrameBufferConfig& frame_buffer_config_ref, const MemoryMap& memory_map_ref){
	MemoryMap memory_map{memory_map_ref};

	InitializeGraphics(frame_buffer_config_ref);
	InitializeConsole();

	printk("Saiko no Egao den Kirinukeruyo\n");
	SetLogLevel(kWarn);

	InitializeSegmentation();
	InitializePaging();
	InitializeMemoryManager(memory_map);
	::main_queue = new std::deque<Message>(32);
	InitializeInterrupt(main_queue);
	
	InitializePCI();
	usb::xhci::Initialize();

	InitializeLayer();
	InitializeMainWindow();
	InitializeMouse();
	layer_manager->Draw({{0, 0}, ScreenSize()});

	char str[128];
	unsigned int count = 0;
	
	while (true) {
		++count;
		sprintf(str, "%010u", count);
		FillRectangle(*main_window->Writer(), {24, 28}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
		WriteString(*main_window->Writer(), {24, 28}, str, {0, 0, 0});
		layer_manager->Draw(main_window_layer_id);

		// cli represent that set interrupt flag to 0. it means don't receive interrupt.
		__asm__("cli");

		if (main_queue->size() == 0) {
			// use \n\t to thw order in one line.
			//__asm__("sti\n\thlt");
			__asm__("sti");
			continue;
		}

		Message msg = main_queue->front();
		main_queue->pop_front();
		// sti is Set Interrupt Flag. it activate interrupt.
		__asm__("sti");

		switch (msg.type) {
			case Message::kInterruptXHCI:
				usb::xhci::ProcessEvents();
				break;
			default:
				Log(kError, "Unkenown message type: %d\n", msg.type);
		}
	}
}

// __cxa_pure_virtual() is first value of vtable about pure virtual method. if this method is called, there is a bug. i don't know why we should write this method despite that 
extern "C" void __cxa_pure_virtual() {
	while (1) __asm__("hlt");
}


	// to manipulate memory address per byte, use uint8_t. if use uint64_t, manipulate per bit. because frame_buffer_size is per byte and frame buffer is basically per byte, so we need to cast frame_buffer_base to represent uint8_t.
	// uint8_t* frame_bufferptr = reinterpret_cast<uint8_t*>(frame_buffer_base);

	// frame_buffer_size have potential to become too big number, so declare as uint64_t
	// for(uint64_t i = 0; i < frame_buffer_size; ++i){
		// pointer + [] means value. in pointer + [], 1 increment means proceed sizeof type. in this case frame_bufferptr is uint8_t, so 1 increment means 8 bit, equall 1 byte, proceed. by the way, in c, (pointer + 1) means pointer + sizeof value type specified by pointer.
	//	frame_bufferptr[i]= i % 256;
	//}
