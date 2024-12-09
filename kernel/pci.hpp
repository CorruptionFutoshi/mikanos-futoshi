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

	void WriteAddress(uint32_t address);

	void WriteData(uint32_t value);

	uint32_t ReadData();

	uint16_t ReadVendorId(uint8_t bus, uint8_t device, uint8_t function);

	uint16_t ReadDeviceId(uint8_t bus, uint8_t device, uint8_t function);

	uint8_t ReadHeaderType(uint8_t bus, uint8_t device, uint8_t function);

	uint32_t ReadClassCode(uint8_t bus, uint8_t device, uint8_t function);

	uint32_t ReadBusNumbers(uint8_t bus, uint8_t device, uint8_t function);

	bool IsSingleFunctionDevice(uint8_t header_type);

	struct Function {
		uint8_t bus, device, function, header_type;
	};

	// to declare global variable in header file, add "inline"
	// std::array is container class of fixed length array. in <a, b>, a represent type of array, b means size of array
	inline std::array<Function, 32> functions;
	// number of device
	inline int num_function;

	Error ScanAllBus();
}
