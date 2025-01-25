#pragma once

#include <stdint.h>

struct MemoryMap {
	// long long is one of the type. it's larger than long
	unsigned long long buffer_size;
	void* buffer;
	unsigned long long map_size;
	unsigned long long map_key;
	unsigned long long descriptor_size;
	uint32_t descriptor_version;
};

struct MemoryDescriptor {
	uint32_t type;
	uintptr_t physical_start;
	uintptr_t virtual_start;
	uint64_t number_of_pages;
	uint64_t attribute;
};

// enum is C++'s expression. so don't compile in C
#ifdef __cplusplus
enum class MemoryType {
	kEfiReservedMemoryType,
	kEfiLoaderCode,
	kEfiLoaderData,
	kEfiBootServicesCode,
	kEfiBootServicesData,
	kEfiRuntimeServicesCode,
	kRuntimeServicesData,
	kEfiConventionalMemory,
	kEfiACPIReclaimMemory,
	kEfiACPIMemoryNVS,
	kEfiMemoryMappedIO,
	kEfiMemoryMappedIOPortSpace,
	kEfiPalCode,
	kEfiPersistentMemory,
	kEfiMaxMemoryType
};

// first parameter is leftside of ==. second parameter is rightside
inline bool operator==(uint32_t Ihs, MemoryType rhs) {
	return Ihs == static_cast<uint32_t>(rhs);
}

inline bool operator==(MemoryType Ihs, uint32_t rhs) {
	// this call above ==. so we don't need static_cast.
	return rhs == Ihs;
}
// represent end of #ifdef
#endif
