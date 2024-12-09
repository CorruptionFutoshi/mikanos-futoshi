#include "pci.hpp"

#include "asmfunc.h"

// unnamed namespace means cant be called from outside and in this namespace declare method, field... without colision name
// to conceal internal processing, use unnamed namespace
namespace {
	using namespace pci;

	uint32_t MakeAddress(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_addr)	{
		// this is unnamed method. we can't declare normal method in method, but can declare unnamed method   
		auto shl = [](uint32_t x, unsigned int bits){
			return x << bits;
		};

		// 0xfcu = 0b11111100. so & 0xfcu means fix bottom 2 bit to 0
		return shl(1, 31) | shl(bus, 16) | shl(device, 11) | shl(function, 8) | (reg_addr & 0xfcu);
	}

	Error AddFunction(uint8_t bus, uint8_t device, uint8_t function, uint8_t header_type) {
		if(num_function == functions.size()) {
			return Error::kFull;
		}

		functions[num_function] = Function{bus, device, function, header_type};
		++num_function;
		return Error::kSuccess;
	}

	// prototype declaration. in c only upper method is callable.
	Error ScanBus(uint8_t bus);

	Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function) {
		auto header_type = ReadHeaderType(bus, device, function);

		// PCI device is specified with bus number and device number, so we call AddDevice() method with function0. function number is not necessary to specify device
		if (auto err = AddFunction(bus, device, function, header_type)) {
			return err;
		}

		auto class_code = ReadClassCode(bus, device, function);
		uint8_t base = (class_code >> 24) & 0xffu;
		uint8_t sub = (class_code >> 16) & 0xffu;

		// base class code is 0x06 and sub class code is 0x04 means this function is PCI-to-PCI hostbridge 
		if (base == 0x06u && sub ==0x04u) {
			auto bus_numbers = ReadBusNumbers(bus, device, function);
			// in 0x18 of PCI configuration area, thare is a bus number
			uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
			return ScanBus(secondary_bus);
		}

		return Error::kSuccess;
	}

	// scan function to discover secondary bus
	Error ScanDevice(uint8_t bus, uint8_t device) {
		auto header_type = ReadHeaderType(bus, device, 0);

		if (IsSingleFunctionDevice(header_type)) {
			return ScanFunction(bus, device, 0);
		}

		for (uint8_t function = 1; function < 8; ++function) {
			if (ReadVendorId(bus, device, function) == 0xffffu) {
				continue;
			}

			if (auto err = ScanFunction(bus, device, function)) {
				return err;
			}
		}

		return Error::kSuccess;
	}

	Error ScanBus(uint8_t bus) {
		for (uint8_t device = 0; device < 32; ++device) {
			if (ReadVendorId(bus, device, 0) == 0xffffu) {
				continue;
			}

			if (auto err = ScanDevice(bus, device)) {
				return err;
			}
		}

		return Error::kSuccess;
	}
}

// namespace to publish
namespace pci {
	void WriteAddress(uint32_t address) {
		IoOut32(kConfigAddress, address);
	}

	void WriteData(uint32_t value) {
		IoOut32(kConfigData, value);
	}

	uint32_t ReadData() {
		return IoIn32(kConfigData);
	}

	uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x00));
		return ReadData() & 0xffffu;
	}

	uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x00));
		return ReadData() >> 16;
	}

	uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x0c));
		return (ReadData() >> 16) & 0xffu;
	}

	// to use interface, declare return value type as uint32_t
	uint32_t ReadClassCode(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x08));
		return ReadData();
	}
	
	uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x18));
		return ReadData();
	}

	bool IsSingleFunctionDevice(uint8_t header_type) {
		return (header_type & 0x80u) == 0;
	}

	Error ScanAllBus() {
		num_function = 0;

		// device specified with "bus0, device0" is hostbridge. a hostbridge basically handle an independent bus. exception is PCI-to-PCI bridge. it cant connect two bus, and a hostbridge can handle them 
		auto header_type = ReadHeaderType(0, 0, 0);

		// if hostbridge is single, probably number of bus if one. probably to explain hostbridge, this logic is implemented. indeed, overhead when run below for code from 0 is bit, and dont have to this early return method. there is a humor that single function device often be filled up rest function numbers with same PCI configuration , so to avoid this, this logic is not meaningless 
		if (IsSingleFunctionDevice(header_type)) {
			return ScanBus(0);
		}

		// each functions of device specified with "bus0, device0" represent each hostbridge, and it handle each independent buses too. for example,function1 of "bus0, device0" represent hostbridge1, and it handle bus1. considering PCI-to-PCI bridge, a hostbridge can handle multi bus, but at least a hostbridge handle a bus so Scanbus(function)
		for (uint8_t function = 1; function < 8; ++function) {
			// in PCI regulation, if VenderId is filled up with 1, it represent that the device is not exist. a function have a PCI configuration space
			if (ReadVendorId(0, 0, function) == 0xffffu) {
				continue;
			}

			if (auto err = ScanBus(function)) {
				return err;
			}
		}

		return Error::kSuccess;
	}
}
