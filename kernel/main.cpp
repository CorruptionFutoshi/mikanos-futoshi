#include <cstdint>
#include <cstddef>
#include <cstdio>

#include <numeric>
#include <vector>

#include "frame_buffer_config.hpp"
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

const PixelColor kDesktopBGColor{213, 203, 198};
const PixelColor kDesktopFGColor{81, 55, 67};

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

char mouse_cursor_buf[sizeof(MouseCursor)];
MouseCursor* mouse_cursor;

void MouseObserver(int8_t displacement_x, int8_t displacement_y) {
	mouse_cursor->MoveRelative({displacement_x, displacement_y});
}

void SwitchEhciToXhci(const pci::Function& xhc_func) {
	bool intel_ehc_exist = false;

	for (int i = 0; i < pci::num_function; ++i) {
		// base, sub, interface is 0x0cu, 0x03u, 0x20u means that this fucntion is EHCI controller. u means unsigned
		if (pci::functions[i].class_code.Match(0x0cu, 0x03u, 0x20u) && 0x8086 == pci::ReadVendorId(pci::functions[i])) {
			intel_ehc_exist = true;
			break;
		}
	}

	if (!intel_ehc_exist) {
		return;
	}

	// PCI configuration space is 255(0xff). pci:: means pci namespace. 
	uint32_t superspeed_ports = pci::ReadConfReg(xhc_func, 0xdc);
	pci::WriteConfReg(xhc_func, 0xd8, superspeed_ports);
	uint32_t ehcitoxhci_ports = pci::ReadConfReg(xhc_func, 0xd4);
	pci::WriteConfReg(xhc_func, 0xd0, ehcitoxhci_ports);

	Log(kDebug, "SwitchEhciToXhci: SS = %02, xHCI = %02x\n", superspeed_ports, ehcitoxhci_ports);
}

// write __attribute__((interrupt)) to tell compiler that this is interrupt handler.
__attribute__((interrupt))
void IntHandlerXHCI(InterruptFrame* frame) {
	while (xhc->PrimaryEventRing()->HasFront()) {
		if (auto err = ProcessEvent(*xhc)) {
			Log(kError, "Error while ProcessEvent: %s at %s:%d\n",
			    err.Name(), err.File(), err.Line());
		}
	}
	NotifyEndOfInterrupt();
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
	SetLogLevel(kInfo);

	mouse_cursor = new(mouse_cursor_buf) MouseCursor {
		pixel_writerptr, kDesktopBGColor, {300, 200}
	};
	
	auto err = pci::ScanAllBus();
	Log(kDebug, "ScanAllBus: %s\n", err.Name());

	for (int i = 0; i < pci::num_function; ++i) {
		const auto& func = pci::functions[i];
		auto vendor_id = pci::ReadVendorId(func.bus, func.device, func.function);
		auto class_code = pci::ReadClassCode(func.bus, func.device, func.function);

		Log(kDebug, "%d.%d.%d: vend %04x, class %08x, head %02x\n", func.bus, func.device, func.function, vendor_id, class_code, func.header_type);
	}

	pci::Function* xhc_funcptr = nullptr;
	
	for (int i = 0; i < pci::num_function; ++i) {
		if (pci::functions[i].class_code.Match(0x0cu, 0x03u, 0x30u)) {
			xhc_funcptr = &pci::functions[i];
		
			// if this xhc controller is made by intel, select it
			if(0x8086 == pci::ReadVendorId(*xhc_funcptr)) {
				break;
			}
		}
	}

	if(xhc_funcptr) {
		Log(kInfo, "xHC has been found: %d.%d.%d\n", xhc_funcptr->bus, xhc_funcptr->device, xhc_funcptr->function);
	}

	const uint16_t cs = GetCS();
	// reinterpret_cast<uint64_t>({method}) is address of method.
	SetIDTEntry(idt[InterruptVector::kXHCI], MakeIDTAttr(DescriptorType::kInterruptGate, 0), 
				reinterpret_cast<uint64_t>(IntHandlerXHCI), cs);
	LoadIDT(sizeof(idt) - 1, reinterpret_cast<uintptr_t>(&idt[0]));

	const uint8_t bsp_local_apic_id = *reinterpret_cast<const uint32_t*>(0xfee00020) >> 24;
	pci::ConfigureMSIFixedDestination(
			*xhc_funcptr, bsp_local_apic_id, 
			pci::MSITriggerMode::kLevel, pci::MSIDeliveryMode::kFixed, InterruptVector::kXHCI, 0);

	// in bar0 there is a mmio address 
	const WithError<uint64_t> xhc_bar = pci::ReadBar(*xhc_funcptr, 0);
	Log(kDebug, "ReadBar: %s\n", xhc_bar.error.Name());
	// register that handle xHC is memory mapped I/O. ~ means reverse so this is mask of bottom 4 bit
	const uint64_t xhc_mmio_base = xhc_bar.value & ~static_cast<uint64_t>(0xf);
	Log(kDebug, "xHC mmio_base = %08lx\n", xhc_mmio_base);

	// this is declaration of Controller type variable. Controller type is in xhci namespace in usb namespace.
	usb::xhci::Controller xhc{xhc_mmio_base};

	if (0x8086 == pci::ReadVendorId(*xhc_funcptr)) {
		SwitchEhciToXhci(*xhc_funcptr);
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
	// sti is Set Interrupt Flag. it activaate interrupt.
	__asm__("sti");

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

	while (1) __asm__("hlt");
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
