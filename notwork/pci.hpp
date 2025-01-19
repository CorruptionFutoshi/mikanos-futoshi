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

	// this is union. so if data field is align, value of bits field is overwritten. data and bits represent same data.
	union CapabilityHeader {
		uint32_t data;
		struct {
			uint32_t cap_id : 8;
			uint32_t next_ptr : 8;
			uint32_t cap: 16;
		} __attribute__((packed)) bits;
	} __attribute__((packed));

	const uint8_t kCapabilityMSI = 0x05;
	const uint8_t kCapabilityMSIX = 0x11;

	CapabilityHeader ReadCapabilityHeader(const Function& func, uint8_t addr);

	struct MSICapability {
		union {
			uint32_t data;
			struct {
				uint32_t cap_id : 8;
				uint32_t next_ptr : 8;
				uint32_t msi_enable : 1;
				uint32_t multi_msg_capable : 3;
				uint32_t multi_msg_enable : 3;
				uint32_t addr_64_capable : 1;
				uint32_t per_vector_mask_capable : 1;
				uint32_t : 7;
			} __attribute__((packed)) bits;
		} __attribute__((packed)) header;

		uint32_t msg_addr;
		uint32_t msg_upper_addr;
		uint32_t msg_data;			
		uint32_t mask_bits;
		uint32_t pending_bits;
	} __attribute__((packed)); 	

	Error ConfigureMSI(const Function& func, uint32_t msg_addr, uint32_t msg_data, unsigned int num_vector_exponent);

	enum class MSITriggerMode {
		kEdge = 0,
		kLevel = 1,
	};

	enum class MSIDeliveryMode {
		kFixed		= 0b000,
		kLowestPriority	= 0b001,
		kSMI		= 0b010,
		kNMI		= 0b100,
		kINIT		= 0b101,
		kExtINT		= 0b111,
	};

	Error ConfigureMSIFixedDestination(const Function& func, uint8_t apic_id, MSITriggerMode trigger_mode, 
			MSIDeliveryMode delivery_mode, uint8_t vector, unsigned int num_vector_exponent);
