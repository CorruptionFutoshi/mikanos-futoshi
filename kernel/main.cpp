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
#include "timer.hpp"
#include "acpi.hpp"
#include "keyboard.hpp"
#include "task.hpp"
#include "terminal.hpp"

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

std::shared_ptr<ToplevelWindow> main_window;
unsigned int main_window_layer_id;

void InitializeMainWindow() {
	main_window = std::make_shared<ToplevelWindow>(160, 52, screen_config.pixel_format, "Hello Window");

	main_window_layer_id = layer_manager->NewLayer()
		.SetWindow(main_window)
		.SetDraggable(true)
		.Move({300, 100})
		.ID();

	layer_manager->UpDown(main_window_layer_id, std::numeric_limits<int>::max());
}

std::shared_ptr<ToplevelWindow> text_window;
unsigned int text_window_layer_id;

void InitializeTextWindow() {
	const int win_w = 160;
	const int win_h = 52;

	text_window = std::make_shared<ToplevelWindow>(win_w, win_h, screen_config.pixel_format, "Text Box Test");
	DrawTextbox(*text_window->InnerWriter(), {0, 0}, text_window->InnerSize());

	text_window_layer_id = layer_manager->NewLayer()
		.SetWindow(text_window)
		.SetDraggable(true)
		.Move({500, 100})
		.ID();

	layer_manager->UpDown(text_window_layer_id, std::numeric_limits<int>::max());
}

int text_window_index;

void DrawTextCursor(bool visible) {
	const auto color = visible ? ToColor(0) : ToColor(0xffffff);
	const auto pos = Vector2D<int>{4 + 8 * text_window_index, 5};
	FillRectangle(*text_window->InnerWriter(), pos, {7, 15}, color);
}

void InputTextWindow(char c) {
	if (c == 0) {
		return;
	}

	auto pos = []() { return Vector2D<int>{4 + 8 * text_window_index, 6}; };

	const int max_chars = (text_window->InnerSize().x - 8) / 8 - 1;

	if (c == '\b' && text_window_index > 0) {
		DrawTextCursor(false);
		--text_window_index;
		FillRectangle(*text_window->InnerWriter(), pos(), {8, 16}, ToColor(0xffffff));
		DrawTextCursor(true);
	} else if (c >= ' ' && text_window_index < max_chars) {
		DrawTextCursor(false);
		WriteAscii(*text_window->InnerWriter(), pos(), c, ToColor(0));
		++text_window_index;
		DrawTextCursor(true);
	}

	layer_manager->Draw(text_window_layer_id);
}

std::shared_ptr<ToplevelWindow> task_b_window;
unsigned int task_b_window_layer_id;

void InitializeTaskBWindow() {
	task_b_window = std::make_shared<ToplevelWindow>(160, 52, screen_config.pixel_format, "TaskB Window");

	task_b_window_layer_id = layer_manager->NewLayer()
		.SetWindow(task_b_window)
		.SetDraggable(true)
		.Move({100, 100})
		.ID();

	layer_manager->UpDown(task_b_window_layer_id, std::numeric_limits<int>::max());	
}

void TaskB(uint64_t task_id, int64_t data) {
	printk("TaskB: task_id=%d, data=%d\n", task_id, data);
	char str[128];
	int count = 0;

	__asm__("cli");
	Task& task = task_manager->CurrentTask();
	__asm__("sti");

	while(true) {
		++count;
		sprintf(str, "%010d", count);
		FillRectangle(*task_b_window->InnerWriter(), {20, 4}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
		WriteString(*task_b_window->InnerWriter(), {20, 4}, str, {0, 0, 0});

		Message msg{Message::kLayer, task_id};
		msg.arg.layer.layer_id = task_b_window_layer_id;
		msg.arg.layer.op = LayerOperation::Draw;
		__asm__("cli");
		task_manager->SendMessage(1, msg);
		__asm__("sti");

		while (true) {
			__asm__("cli");
			auto msg = task.ReceiveMessage();

			if (!msg) {
				task.Sleep();
				__asm__("sti");
				continue;
			}

			if (msg->type == Message::kLayerFinish) {
				break;
			}
		}
	}
}

// alignas guarantee that start address of this variable is multiple of 16.
// as you already know, uint8_t is type of content of array.
alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(const FrameBufferConfig& frame_buffer_config_ref,
	       			   const MemoryMap& memory_map_ref, const acpi::RSDP& acpi_table){
	MemoryMap memory_map{memory_map_ref};

	InitializeGraphics(frame_buffer_config_ref);
	InitializeConsole();

	printk("Saiko no Egao den Kirinukeruyo\n");
	SetLogLevel(kWarn);

	InitializeSegmentation();
	InitializePaging();
	InitializeMemoryManager(memory_map);
	InitializeInterrupt();
	
	InitializePCI();

	InitializeLayer();
	InitializeMainWindow();
	InitializeTextWindow();
	InitializeTaskBWindow();
	layer_manager->Draw({{0, 0}, ScreenSize()});

	acpi::Initialize(acpi_table);
	InitializeLAPICTimer();

	const int kTextboxCursorTimer = 1;
	const int kTimer05Sec = static_cast<int>(kTimerFreq * 0.5);
	// first parameter of Timer constructor is timeout;
	timer_manager->AddTimer(Timer{kTimer05Sec, kTextboxCursorTimer});
	bool textbox_cursor_visible = false;

	InitializeTask();
	Task& main_task = task_manager->CurrentTask();
	const uint64_t taskb_id = task_manager->NewTask()
		.InitContext(TaskB, 45)
		.Wakeup()
		.ID();

	const uint64_t task_terminal_id = task_manager->NewTask()
		.InitContext(TaskTerminal, 0)
		.Wakeup()
		.ID();

	usb::xhci::Initialize();
	InitializeKeyboard();
	InitializeMouse();
	active_layer->Activate(task_b_window_layer_id);

	char str[128];
	
	while (true) {
		__asm__("cli");
		const auto tick = timer_manager->CurrentTick();
		__asm__("sti");

		sprintf(str, "%010lu", tick);
		FillRectangle(*main_window->InnerWriter(), {20, 4}, {8 * 10, 16}, {0xc6, 0xc6, 0xc6});
		WriteString(*main_window->InnerWriter(), {20, 4}, str, {0, 0, 0});
		layer_manager->Draw(main_window_layer_id);

		// cli represent that set interrupt flag to 0. it means don't receive interrupt.
		__asm__("cli");
	
		auto msg = main_task.ReceiveMessage();

		// nullptr of std::optional type is treated as false.
		if (!msg) {
			main_task.Sleep();
			__asm__("sti");
			// sti represent that set interrupt flag to 1. allow interrupt.
			// use \n\t to two order in one line.
			// __asm__("sti\n\thlt");
			continue;
		}

		// sti is Set Interrupt Flag. it activate interrupt.
		__asm__("sti");

		switch (msg->type) {
			case Message::kInterruptXHCI:
				usb::xhci::ProcessEvents();
				break;
			case Message::kTimerTimeout:
				if (msg->arg.timer.value == kTextboxCursorTimer) {
					__asm__("cli");
					timer_manager->AddTimer(Timer{msg->arg.timer.timeout + kTimer05Sec, kTextboxCursorTimer});
					__asm__("sti");
					textbox_cursor_visible = !textbox_cursor_visible;
					DrawTextCursor(textbox_cursor_visible);
					layer_manager->Draw(text_window_layer_id);
					__asm__("cli");
					task_manager->SendMessage(task_terminal_id, *msg);
					__asm__("sti");
				}

				break;
			case Message::kKeyPush:
				if (auto act = active_layer->GetActive(); act == text_window_layer_id) {
					InputTextWindow(msg->arg.keyboard.ascii);
				} else if (act == task_b_window_layer_id) {
					if (msg->arg.keyboard.ascii == 's') {
						printk("sleep TaskB: %s\n", task_manager->Sleep(taskb_id).Name());
					} else if (msg->arg.keyboard.ascii == 'w') {
						printk("wakeup TaskB: %s\n", task_manager->Wakeup(taskb_id).Name());
					}
				} else {
					printk("key push not handled: keycode %02x, ascii %02x\n", 
							msg->arg.keyboard.keycode, msg->arg.keyboard.ascii);
				}

				break;
			case Message::kLayer:
				ProcessLayerMessage(*msg);
				__asm__("cli");
				task_manager->SendMessage(msg->src_task, Message{Message::kLayerFinish});
				__asm__("sti");
				break;
			default:
				Log(kError, "Unkenown message type: %d\n", msg->type);
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
