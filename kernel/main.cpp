#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>

#include "frame_buffer_config.hpp"
#include "memory_map.hpp"
#include "graphics.hpp"
#include "mouse.hpp"
#include "font.hpp"
#include "console.hpp"
#include "pci.hpp"
#include "logger.hpp"
#include "usb/memory.hpp"
#include "usb/device.hpp"
#include "usb/classdriver/mouse.hpp"
#include "usb/xhci/xhci.hpp"
#include "usb/xhci/trb.hpp"
#include "interrupt.hpp"
#include "asmfunc.h"
#include "queue.hpp"
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
char pixel_writer_buf[sizeof(RGBResv8BitPerColorPixelWriter)];
PixelWriter* pixel_writer;

char console_buf[sizeof(Console)];
Console* console;

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

//StartLAPICTimer();
//console->PutString(s);
//auto elapsed = LAPICTimerElapsed();
//StopLAPICTimer();

// sprintf represent that set format string to first parameter.
//sprintf(s, "[%9d]", elapsed);
console->PutString(s);
return result;
}

char memory_manager_buf[sizeof(BitmapMemoryManager)];
BitmapMemoryManager* memory_manager;

unsigned int mouse_layer_id;
Vector2D<int> screen_size;
Vector2D<int> mouse_position;

void MouseObserver(uint8_t buttons, int8_t displacement_x, int8_t displacement_y) {
	// this is static variable, so only create once.
	static unsigned int mouse_drag_layer_id = 0;
	static uint8_t previous_buttons = 0;

	const auto oldpos = mouse_position;
	auto newpos = mouse_position + Vector2D<int>{displacement_x, displacement_y};
	newpos = ElementMin(newpos, screen_size + Vector2D<int>{-1, -1});
	mouse_position = ElementMax(newpos, {0, 0});

	const auto posdiff = mouse_position - oldpos;

	layer_manager->Move(mouse_layer_id, mouse_position);
	
	const bool previous_left_pressed = (previous_buttons & 0x01);
	const bool left_pressed = (buttons & 0x01);

	if (!previous_left_pressed && left_pressed) {
		auto layer = layer_manager->FindLayerByPosition(mouse_position, mouse_layer_id);

		if (layer && layer->IsDraggable()) {
			mouse_drag_layer_id = layer->ID();
		}
	} else if (previous_left_pressed && left_pressed) {
		if (mouse_drag_layer_id > 0) {
			layer_manager->MoveRelative(mouse_drag_layer_id, posdiff);
		}
	} else if (previous_left_pressed && !left_pressed) {
		mouse_drag_layer_id = 0;
	}

	previous_buttons = buttons;
	//layer_manager->MoveRelative(mouse_layer_id, {displacement_x, displacement_y});
	//StartLAPICTimer();
	//layer_manager->Draw();
	//auto elapsed = LAPICTimerElapsed();
	//StopLAPICTimer();
	//printk("MouseObserver: elapsed = %u\n", elapsed);
}

void SwitchEhciToXhci(const pci::Device& xhc_dev) {
bool intel_ehc_exist = false;

for (int i = 0; i < pci::num_device; ++i) {
	// base, sub, interface is 0x0cu, 0x03u, 0x20u means that this fucntion is EHCI controller. u means unsigned
	if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x20u) && 0x8086 == pci::ReadVendorId(pci::devices[i])) {
		intel_ehc_exist = true;
		break;
	}
}

if (!intel_ehc_exist) {
	return;
}

// PCI configuration space is 255(0xff). pci:: means pci namespace. 
uint32_t superspeed_ports = pci::ReadConfReg(xhc_dev, 0xdc);
pci::WriteConfReg(xhc_dev, 0xd8, superspeed_ports);
uint32_t ehcitoxhci_ports = pci::ReadConfReg(xhc_dev, 0xd4);
pci::WriteConfReg(xhc_dev, 0xd0, ehcitoxhci_ports);

Log(kDebug, "SwitchEhciToXhci: SS = %02, xHCI = %02x\n", superspeed_ports, ehcitoxhci_ports);
}

usb::xhci::Controller* xhc;

struct Message {
enum Type {
	kInterruptXHCI,
} type;
};

ArrayQueue<Message>* main_queue;

// write __attribute__((interrupt)) to tell compiler that this is interrupt handler.
__attribute__((interrupt))
void IntHandlerXHCI(InterruptFrame* frame) {
	// Message{Message::kInterruptXHCI} is initializer list. Message struct has only one field, so we can initialize with this code.
	main_queue->Push(Message{Message::kInterruptXHCI});
	NotifyEndOfInterrupt();
}

// alignas guarantee that start address of this variable is multiple of 16.
// as you already know, uint8_t is type of content of array.
alignas(16) uint8_t kernel_main_stack[1024 * 1024];

extern "C" void KernelMainNewStack(const FrameBufferConfig& frame_buffer_config_ref, const MemoryMap& memory_map_ref){
	// i don't know why create frame_buffer_config instead of directly use frame_buffer_config_ref.
	FrameBufferConfig frame_buffer_config{frame_buffer_config_ref};
	MemoryMap memory_map{memory_map_ref};

	switch(frame_buffer_config.pixel_format) {
		case kPixelRGBResv8BitPerColor:
			// new operator seems to require two parameter, but size_t is not need. new calcurate size of type automatically. so it is enough to set parameter buffer. {frame_buffer_config} is required by constructor. {} is uniform initialization
			pixel_writer = new(pixel_writer_buf)RGBResv8BitPerColorPixelWriter{frame_buffer_config};
			break;
		case kPixelBGRResv8BitPerColor:
			pixel_writer = new(pixel_writer_buf)BGRResv8BitPerColorPixelWriter{frame_buffer_config};
			break;
	}

	DrawDesktop(*pixel_writer);	

	console = new(console_buf) Console{kDesktopFGColor, kDesktopBGColor};
	console->SetWriter(pixel_writer);
	printk("Saiko no Egao de Kirinukeruyo\n");
	SetLogLevel(kWarn);

	//InitializeLAPICTimer();

	SetupSegments();
	// 3-15 bit of segment selector(value that written in segment register) represent index of GDT.
	const uint16_t kernel_cs = 1 << 3;
	const uint16_t kernel_ss = 2 << 3;
	SetDSAll(0);
	SetCSSS(kernel_cs, kernel_ss);

	SetupIdentityPageTable();

	// :: represent that this variable is global variable.	
	::memory_manager = new(memory_manager_buf) BitmapMemoryManager;

	const auto memory_map_base = reinterpret_cast<uintptr_t>(memory_map.buffer);
	uintptr_t available_end = 0;
	for (uintptr_t iter = memory_map_base;
	     iter < memory_map_base + memory_map.map_size;
	     iter += memory_map.descriptor_size) {
		auto desc = reinterpret_cast<const MemoryDescriptor*>(iter);

		// area that doesn't be written in memorymap is probably already reserved.
		if (available_end < desc->physical_start) {
			memory_manager->MarkAllocated(
					FrameID{available_end / kBytesPerFrame},
					(desc->physical_start - available_end) / kBytesPerFrame);
		}

		const auto physical_end = desc->physical_start + desc->number_of_pages * kUEFIPageSize;

		// the reason why we can get type without set is maybe UEFI firmware set it.
		if (IsAvailable(static_cast<MemoryType>(desc->type))) {
			available_end = physical_end;
		} else {
			memory_manager->MarkAllocated(
					FrameID{desc->physical_start / kBytesPerFrame},
					desc->number_of_pages * kUEFIPageSize / kBytesPerFrame);
		}
	
	}

	memory_manager->SetMemoryRange(FrameID{1}, FrameID{available_end / kBytesPerFrame});

	if (auto err = InitializeHeap(*memory_manager)) {
		Log(kError, "failed to allocate pages: %s at %s:%d\n", err.Name(), err.File(), err.Line());
		// exit(1) represent abnormal termination.
		exit(1);
	}

	std::array<Message, 32> main_queue_data;
	ArrayQueue<Message> main_queue{main_queue_data};
	::main_queue = &main_queue;
	
	auto err = pci::ScanAllBus();
	Log(kDebug, "ScanAllBus: %s\n", err.Name());

	for (int i = 0; i < pci::num_device; ++i) {
		const auto& dev = pci::devices[i];
		auto vendor_id = pci::ReadVendorId(dev.bus, dev.device, dev.function);
		auto class_code = pci::ReadClassCode(dev.bus, dev.device, dev.function);

		Log(kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x\n", dev.bus, dev.device, dev.function, vendor_id, class_code, dev.header_type);
	}

	pci::Device* xhc_devptr = nullptr;
	
	for (int i = 0; i < pci::num_device; ++i) {
		if (pci::devices[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
			xhc_devptr = &pci::devices[i];
		
			// if this xhc controller is made by intel, select it
			if(0x8086 == pci::ReadVendorId(*xhc_devptr)) {
				break;
			}
		}
	}

	if(xhc_devptr) {
		Log(kInfo, "xHC has been found: %d.%d.%d\n", xhc_devptr->bus, xhc_devptr->device, xhc_devptr->function);
	}

	// reinterpret_cast<uint64_t>({method}) is address of method.
	SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0), 
				reinterpret_cast<uint64_t>(IntHandlerXHCI), kernel_cs);
	LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

	const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t*>(0xfee00020) >> 24;
	pci::ConfigureMSIFixedDestination(
			*xhc_devptr, bsp_local_apic_id, 
			pci::MSITriggerMode::kLevel, pci::MSIDeliveryMode::kFixed, InterruptVector::kXHCI, 0);

	// in bar0 there is a mmio address 
	const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_devptr, 0);
	Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
	// register that handle xHC is memory mapped I/O. ~ means reverse so this is mask of bottom 4 bit
	const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
	Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);

	// this is declaration of Controller type variable. Controller type is in xhci namespace in usb namespace.
	usb::xhci::Controller xhc{xhc_mmio_base};

	if (0x8086 == pci::ReadVendorId(*xhc_devptr)) {
		SwitchEhciToXhci(*xhc_devptr);
	}

	// variable name "err" is already used, so envelope with '{}' and create independent scope
	{	
		auto err = xhc.Initialize();
		Log(kDebug, "xhc.Initialize: %s\n", err.Name());
	}

	Log(kInfo, "xHC starting\n");
	xhc.Run();

	// align xhc of global variable to local variable xhc's address. 
	::xhc = &xhc;
	// sti is Set Interrupt Flag. it activate interrupt.
	// __asm__("sti");

	usb::HIDMouseDriver::default_observer = MouseObserver;

	for (int i = 1; i <=xhc.MaxPorts(); ++i) {
		auto port = xhc.PortAt(i);
		Log(kDebug, "Port %d: IsConnected=%d\n", i, port.IsConnected());

		if (port.IsConnected()) {
			// this condition means that err == true or not after run ConfigurePort() and set it to err. ConfigurePort is usb::xhci::ConfigurePort(), but can call it because of ADL of c++ function
			if (auto err = ConfigurePort(xhc, port)) {
				Log(kError, "failed to configure port: %s at %s:%d\n", err.Name(), err.File(), err.Line());
				continue;
			}
		}
	}

	screen_size.x = frame_buffer_config.horizontal_resolution;
	screen_size.y = frame_buffer_config.vertical_resolution;

	auto bgwindow = std::make_shared<Window>(screen_size.x, screen_size.y, frame_buffer_config.pixel_format);
	auto bgwriter = bgwindow->Writer();

	DrawDesktop(*bgwriter);

	auto mouse_window = std::make_shared<Window>(kMouseCursorWidth, kMouseCursorHeight, frame_buffer_config.pixel_format);
	mouse_window->SetTransparentColor(kMouseTransparentColor);
	DrawMouseCursor(mouse_window->Writer(), {0, 0});
	mouse_position = {200, 200};

	auto main_window = std::make_shared<Window>(160, 52, frame_buffer_config.pixel_format);
	DrawWindow(*main_window->Writer(), "Hello Window");

	auto console_window = std::make_shared<Window>(Console::kColumns * 8, 
			Console::kRows * 16, frame_buffer_config.pixel_format);
	console->SetWindow(console_window);
	
	FrameBuffer screen;

	if (auto err = screen.Initialize(frame_buffer_config)) {
		Log(kError, "failed to initialize frame buffer: %s at %s:%d\n", err.Name(), err.File(), err.Line());
	}

	layer_manager = new LayerManager;
	layer_manager->SetScreen(&screen);

	auto bglayer_id = layer_manager->NewLayer()
		.SetWindow(bgwindow)
		.Move({0, 0})
		.ID();

	mouse_layer_id = layer_manager->NewLayer()
		.SetWindow(mouse_window)
		.Move(mouse_position)
		.ID();

	auto main_window_layer_id = layer_manager->NewLayer()
		.SetWindow(main_window)
		.SetDraggable(true)
		.Move({300, 100})
		.ID();
	
	console->SetLayerID(layer_manager->NewLayer()
			.SetWindow(console_window)
			.Move({0, 0})
			.ID());

	layer_manager->UpDown(bglayer_id, 0);
	layer_manager->UpDown(console->LayerID(), 1);
	layer_manager->UpDown(main_window_layer_id, 2);
	layer_manager->UpDown(mouse_layer_id, 3);
	layer_manager->Draw({{0, 0}, screen_size});

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

		if (main_queue.Count() == 0) {
			// use \n\t to thw order in one line.
			//__asm__("sti\n\thlt");
			__asm__("sti");
			continue;
		}

		Message msg = main_queue.Front();
		main_queue.Pop();
		__asm__("sti");

		switch (msg.type) {
			case Message::kInterruptXHCI:
				while (xhc.PrimaryEventRing() ->HasFront()) {
					if (auto err = ProcessEvent(xhc)) {
						Log(kError, "Error while ProcessEvent: %s at %s:%d\n",
								err.Name(), err.File(), err.Line());
					}
				}

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
