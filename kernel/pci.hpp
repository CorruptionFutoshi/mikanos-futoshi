#pragma once

#include <cstdint>
#include <array>

#include "error.hpp"

// namespace is used to avoid collision of name. from outside you want to call method or field in namespace, put {namespace}::{method or field} 
namespace pci {
	// IO port address of CONFIG_ADDRESS register
	const uint16_t kConfigAddress = 0x0cf8;

	// IO port address of CONFIG_DATA register
	const uint16_t kConfigData = 0x0cfc;

	struct ClassCode {
		uint8_t base, sub, interface;

		bool Match(uint8_t b) {return b == base;}

		bool Match(uint8_t b, uint8_t s) {return Match(b) && s == sub;}
		
		bool Match(uint8_t b, uint8_t s, uint8_t i) {return Match(b, s) && i == interface;}
	};

	struct Function {
		uint8_t bus, device, function, header_type;
		ClassCode class_code;
	};
	
	void WriteAddress(uint32_t address);

	void WriteData(uint32_t value);

	uint32_t ReadData();

	uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);

	uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);

	uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);

	ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);

	// inline means inline expansion this method when compile. if not declared with "inline", probably error will not occur
	inline uint16_t ReadVendorId(const Function& func) {
		return ReadVendorId(func.bus, func.device, func.function);
	}

	uint32_t ReadConfReg(const Function& func, uint8_t reg_addr);

	void WriteConfReg(const Function& func, uint8_t reg_addr, uint32_t value);

	uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);

	bool IsSingleFunctionDevice(uint8_t header_type);

	// to declare global variable in header file, add "inline"
	// std::array is container class of fixed length array. in <a, b>, a represent type of array, b means size of array
	inline std::array<Function, 32> functions;
	
	inline int num_function;

	Error ScanAllBus();

	// constexpr represent constant when compile.
	constexpr uint8_t CalcBarAddress(unsigned int bar_index) {
		//0x10 means address that point BAR0(Base Address Register 0). and a BAR is 32 bit(4 byte), so times 4. use uint8_t because PCI Configuration area is 256byte(can represent 8bit)
		return 0x10 + 4 * bar_index;
	}

	WithError<uint64_t> ReadBar(Function& func, unsigned int bar_index);
}

