#include "pci.hpp"

#include "asmfunc.h"

// unnamed namespace means cant be called from outside and in this namespace declare method, field... without colision name
// to conceal internal processing, use unnamed namespace
namespace {
	// if declare using namespace, in this namespace we can use pci namespace without "pci::". 
	using namespace pci;

	uint32_t MakeAddress(uint8_t bus, uint8_t device, uint8_t function, uint8_t reg_addr)	{
		// this is unnamed method. we can't declare normal method in method, but can declare unnamed method   
		auto shl = [](uint32_t x, unsigned int bits){
			return x << bits;
		};

		// 0xfcu = 0b11111100. so & 0xfcu means fix bottom 2 bit to 0
		return shl(1, 31) | shl(bus, 16) | shl(device, 11) | shl(function, 8) | (reg_addr & 0xfcu);
	}

	Error AddFunction(const Function& function) {
		if(num_function == functions.size()) {
			return MAKE_ERROR(Error::kFull);
		}

		functions[num_function] = function;
		++num_function;
		return MAKE_ERROR(Error::kSuccess);
	}

	// prototype declaration. in c only upper method is callable.
	Error ScanBus(uint8_t bus);

	Error ScanFunction(uint8_t bus, uint8_t device, uint8_t function) {
		auto class_code = ReadClassCode(bus, device, function);
		auto header_type = ReadHeaderType(bus, device, function);

		Function func{bus, device, function, header_type, class_code};

		// PCI device is specified with bus number and device number, so we call AddDevice() method with function0. function number is not necessary to specify device. i don't know this meaning of this comment
		if (auto err = AddFunction(func)) {
			return err;
		}

		// base class code is 0x06 and sub class code is 0x04 means this function is PCI-to-PCI hostbridge 
		if (class_code.Match(0x06u, 0x04u)) {
			auto bus_numbers = ReadBusNumbers(bus, device, function);
			// in 0x18 of PCI configuration area, thare is a bus numbers. and bus number is 8 bit, so to get number of
			// secondary bus, 8bit right shift.
			uint8_t secondary_bus = (bus_numbers >> 8) & 0xffu;
			return ScanBus(secondary_bus);
		}

		return MAKE_ERROR(Error::kSuccess);
	}

	Error ScanDevice(uint8_t bus, uint8_t device) {
		// there is a rumor that single function device often be filled up rest function numbers with same PCI configuration , so to avoid this, this logic is not meaningless 
		if (IsSingleFunctionDevice(ReadHeaderType(bus, device, 0))) {
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

		return MAKE_ERROR(Error::kSuccess);
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

		return MAKE_ERROR(Error::kSuccess);
	}

	MSICapability ReadMSICapability(const Function& func, uint8_t cap_addr) {
		MSICapability msi_cap{};

		// MSICapability is in PCI configuration space, so we can Read data with ReadConfReg(). PCI configuration space, specifically offset 40~FF, because space of before offset 40 is determind. 
		msi_cap.header.data = ReadConfReg(func, cap_addr);
		// i don't know why we can get messageaddress by Read 0x38. solved. cap_addr is not 0x34, this is value in offset 0x34 of PCI configuration space.
		msi_cap.msg_addr = ReadConfReg(func, cap_addr + 4);
		
		uint8_t msg_data_addr = cap_addr + 8;
		if (msg_cap.header.bits.addr_64_capable) {
			msi_cap.msg_upper_addr = ReadConfReg(func, cap_addr + 8);
			msg_data_addr = cap_addr + 12;
		}

		msi_cap.msg_data = ReadConfReg(func, msg_data_addr);

		if (msi_cap.header.bits.per_vector_mask_capable) {
			msi_cap.mask_bits = ReadConfReg(func, msg_data_addr + 4);
			msi_cap.pending_bits = ReadConfReg(func, msg_data_addr + 8);
		}

		return msi_cap;
	}

	void WriteMSICapability(const Function& func, uint8_t cap_addr, const MSICapability& msi_cap) {
		WriteConfReg(func, cap_addr, msi_cap.header.data);
		WriteConfReg(func, cap_addr + 4, msi_cap.msg_addr);

		uint8_t msg_data_addr = cap_addr + 8;
		if (msi_cap.header.bits.addr_64_capable) {
			WriteConfReg(func, cap_addr + 8, msi_cap.msg_upper_addr);
			msg_data_addr = cap_addr + 12;
		}

		WriteConfReg(func, msg_data_addr, msi_cap.msg_data);

		if (msi_cap.header.bits.per_vector_mask_capable) {
			WriteConfReg(func, msg_data_addr + 4, msi_cap.mask_bits);
			WriteConfReg(func, msg_data_addr + 8, msi_cap.pending_bits);
		}
	}

	Error ConfigureMSIRegister(const Function& func, uint8_t cap_addr, uint32_t msg_addr, uint32_t msg_data,
		       	unsigned int num_vector_exponent) {
		auto msi_cap = ReadMSICapability(func, cap_addr);

		// num_vector_expont maybe represent number of interrupt vector -1. 
		// multi_msg_capable represent number of message(vector). so in this point, compare require number with indeed possible number.
		if (msi_cap.header.bits.multi_msg_capable <= num_vector_exponent) {
			msi_cap.header.bits.multi_msg_enable =msi_cap.header.bits.multi_msg_capable;
		} else {
			msi_cap.header.bits.multi_msg_enable = num_vector_exponent;
		}

		msi_cap.header.bits.msi_enable = 1;
		msi_cap.msg_addr = msg_addr;
		msi_cap.msg_data = msg_data;

		WriteMSICapability(func, cap_addr, msi_cap);
		return MAKE_ERROR(Error::kSuccess);
	}

	Error ConfigureMSIXRegister(const Function& func, uint8_t cap_addr, uint32_t msg_addr, uint32_t msg_data,
			unsigned int num_vector_exponent) {
		return MAKE_ERROR(Error::kNotImplemented);
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

	// to use interface, declare return value type as uint32_t. i don't know meaning of this comment
	ClassCode ReadClassCode(uint8_t bus, uint8_t device, uint8_t function) {
		WriteAddress(MakeAddress(bus, device, function, 0x08));
		auto reg = ReadData();
		ClassCode cc;
		cc.base		= (reg >> 24) & 0xffu;
		cc.sub		= (reg >> 16) & 0xffu;
		cc.interface	= (reg >> 8)  & 0xffu;
		return cc;
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

		// if hostbridge is single, probably number of bus is also one. probably to explain hostbridge, this logic is implemented. indeed, overhead when run below for code from 0 is bit, and dont have to this early return method. there is a rumor that single function device often be filled up rest function numbers with same PCI configuration , so to avoid this, this logic is not meaningless 
		if (IsSingleFunctionDevice(header_type)) {
			return ScanBus(0);
		}

		// each functions of device specified with "bus0, device0" represent each hostbridge, and it handle each independent buses too. for example, function1 of "bus0, device0" represent hostbridge1, and it handle bus1. considering PCI-to-PCI bridge, a hostbridge can handle multi bus, but at least a hostbridge handle a bus so Scanbus(function)
		for (uint8_t function = 0; function < 8; ++function) {
			// in PCI regulation, if VenderId is filled up with 1, it represent that the device is not exist. a function have a PCI configuration space
			if (ReadVendorId(0, 0, function) == 0xffffu) {
				continue;
			}

			if (auto err = ScanBus(function)) {
				return err;
			}
		}

		return MAKE_ERROR(Error::kSuccess);
	}

	uint32_t ReadConfReg(const Function& func, uint8_t reg_addr) {
		WriteAddress(MakeAddress(func.bus, func.device, func.function, reg_addr));
		return ReadData();
	}

	void WriteConfReg(const Function& func, uint8_t reg_addr, uint32_t value) {
		WriteAddress(MakeAddress(func.bus, func.device, func.function, reg_addr));
		WriteData(value);
	}

	WithError<uint64_t> ReadBar(Function& func, unsigned int bar_index) {
		if (bar_index >= 6) {
			return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
		}

		const auto addr = CalcBarAddress(bar_index);
		const auto bar = ReadConfReg(func, addr);

		// lowest 3 bit of base address register(bar) is flag. and if these 3 bit equall 4u, it represent that this bar is 64bit bar. so this condition judge this bar is 32bit bar or not. 
		if ((bar & 4u) == 0 ) {
			return {bar, MAKE_ERROR(Error::kSuccess)};
		}

		// above condition suggest that this bar is 64bit bar. so at least bar_index must be smaller than 5. (there are 6 bar. and to represent 64bit address, use 2 bar.) 
		if (bar_index >= 5) {
			return {0, MAKE_ERROR(Error::kIndexOutOfRange)};
		}

		// this bar is 64bit bar. and "ReadConfReg()" only get 1 bar. so we should get next bar and add up them.
		const auto bar_upper = ReadConfReg(func, addr + 4);
		return {bar | (static_cast<uint64_t>(bar_upper) << 32), MAKE_ERROR(Error::kSuccess)};
	}

	CapabilityHeader ReadCapabilityHeader(const Function& func, uint8_t addr) {
		CapabilityHeader header;
		// i don't know whe "pci::" is written
		header.data = pci::ReadConfReg(func, addr);
		return header;
	}

	Error ConfigureMSI(const Function& func, uint32_t msg_addr, uint32_t msg_data, 
			unsigned int num_vector_exponent) {
		uint8_t cap_addr = ReadConfReg(func, 0x34) & 0xffu;
		uint8_t msi_cap_addr = 0, msix_cap_addr = 0;

		// use while to check all capability register to find MSIcapability register
		while (cap_addr !=0) {
			auto header = ReadCapabilityHeader(func, cap_addr);
			if (header.bits.cap_id == kCapabilityMSI) {
				msi_cap_addr = cap_addr;
			} else if (header.bits.cap_id == kCapabilityMSIX) {
				msix_cap_addr = cap_addr;
			}
			cap_addr = header.bits.next_ptr;
		}

		if (msi_cap_addr) {
			return ConfigureMSIRegister(func, msi_cap_addr, msg_addr, msg_data, num_vector_exponent);
		} else if (msix_cap_addr) {
			return ConfigureMSIXRegister(func, msix_cap_addr, msg_addr, msg_data, num_vector_exponent);
		}

		return MAKE_ERROR(Error::kNoPCIMSI);
	}

	Error ConfigureMSIFixedDestination(const Function& func, uint8_t apic_id, MSITriggerMode trigger_mode,
		       	MSIDeliveryMode delivery_mode, uint8_t vector, unsigned int num_vector_exponent) {
		uint32_t msg_addr = 0xfee00000u | (apic_id << 12);
		uint32_t msg_data = (static_cast<uint32_t>(delivery_mode) << 8) | vector;
		if (trigger_mode == MSITriggerMode::kLevel) {
			msg_data |= 0xc000;
		}

		return ConfigureMSI(func, msg_addr, msg_data, num_vector_exponent);
	}
}
