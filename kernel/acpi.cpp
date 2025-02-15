#include "acpi.hpp"

#include <cstring>
#include <cstdlib>
#include "asmfunc.h"
#include "logger.hpp"

namespace {
	template <typename T>
	uint8_t SumBytes(const T* data, size_t bytes) {
		return SumBytes(reinterpret_cast<const uint8_t*>(data), bytes);
	}

	// template <> represent explicit specialization of template.
	// data is casted to uint8_t, so we can access content per 1 byte by indexer.
	template <>
	uint8_t SumBytes<uint8_t>(const uint8_t* data, size_t bytes) {
		uint8_t sum = 0;

		for (size_t i = 0; i < bytes; ++i) {
			sum += data[i];
		}

		return sum;
	}
}

namespace acpi {
	bool RSDP::IsValid() const {
		if (strncmp(this->signature, "RSD PTR ", 8) != 0) {
			Log(kError, "invalid signature: %.8s\n", this->signature);
			return false;
		}

		if (this->revision != 2) {
			Log(kError, "ACPI revision must be 2: %d\n", this->revision);
			return false;
		}

		if (auto sum = SumBytes(this, 20); sum != 0) {
			Log(kError, "sum of 20 bytes must be 0: %d\n", sum);
			return false;
		} 
		
		if (auto sum = SumBytes(this, 36); sum != 0) {
			Log(kError, "sum of 36 bytes must be 0: %d\n", sum);
			return false;
		} 

		return true;
	}
		
	bool DescriptionHeader::IsValid(const char* expected_signature) const {
		if (strncmp(this->signature, expected_signature, 4) != 0) {
			Log(kDebug, "invalid signature: %.4s\n", this->signature);
			return false;
		}

		if (auto sum = SumBytes(this, this->length); sum != 0) {
			Log(kDebug, "sum of %u bytes must be 0: %d\n", this->length, sum);
			return false;
		}

		return true;
	}

	const DescriptionHeader& XSDT::operator[](size_t i) const {
		// There are DescriptionHeader and 64 bit addressess in XSDT struct.
		// pointer + 1 represent add 1 * type of pointer. so &this->header + 1 represent head of 64 bit addresses.
		auto entries = reinterpret_cast<const uint64_t*>(&this->header + 1);
		// various structs specified by 64 bit addressess always have DescriptionHeader. so we can cast it.
		return *reinterpret_cast<const DescriptionHeader*>(entries[i]);
	}

	size_t XSDT::Count() const {
		// header.length represent size of XSDT struct which has DescriptionHeader and 64 bit addresses.
		return (this->header.length - sizeof(DescriptionHeader)) / sizeof(uint64_t);
	}

	const FADT* fadt;

	void WaitMilliseconds(unsigned long msec) {
		const bool pm_timer_32 = (fadt->flags >> 8) & 1;
		// IoIn32 return data specified with address that is passed as parameter.
		const uint32_t start = IoIn32(fadt->pm_tmr_blk);
		uint32_t end = start + kPMTimerFreq * msec / 1000;

		if (!pm_timer_32) {
			// convert to 24 bit.
			end &= 0x00ffffffu;
		}

		if (end < start) {
			// wait until 0.
			while (IoIn32(fadt->pm_tmr_blk) >= start);
		}

		while (IoIn32(fadt->pm_tmr_blk) < end);
	}

	void Initialize(const RSDP& rsdp) {
		if (!rsdp.IsValid()) {
			Log(kError, "RSDP is not valid\n");
			exit(1);
		}

		const XSDT& xsdt = *reinterpret_cast<const XSDT*>(rsdp.xsdt_address);

		if (!xsdt.header.IsValid("XSDT")) {
			Log(kError, "XSDT is not valid\n");
			exit(1);
		}

		fadt = nullptr;

		for (int i = 0; i < xsdt.Count(); ++i) {
			const auto& entry = xsdt[i];

			if (entry.IsValid("FACP")) {
				fadt = reinterpret_cast<const FADT*>(&entry);
				break;
			}
		}

		if (fadt == nullptr) {
			Log(kError, "FADT is not fount\n");
			exit(1);
		}
	}
}
