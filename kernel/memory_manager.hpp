#pragma once

#include <array>
#include <limits>

#include "error.hpp"
#include "memory_map.hpp"

namespace {
	// it is User-Defined Literals. usage: 8_KiB. (it is 8192)
	constexpr unsigned long long operator"" _KiB(unsigned long long kib) {
		return kib * 1024;
	}

	constexpr unsigned long long operator"" _MiB(unsigned long long mib) {
		return mib * 1024_KiB;
	}

	constexpr unsigned long long operator"" _GiB(unsigned long long gib) {
		return gib * 1024_MiB;
	}
}

static const auto kBytesPerFrame{4_KiB};

class FrameID {
	public:
		// explicit represent that it ban implicit type conversion. 
		// for example, FrameID obj = 5; we should use FrameID obj = FrameID(5) instead;
		explicit FrameID(size_t id) : id_{id} {}
		size_t ID() const { return id_; }
		void* Frame() const { return reinterpret_cast<void*>(id_ * kBytesPerFrame); }
	
	private:
		size_t id_;
};

// std::numeric_limits<size_t>::max() represent max of size_t. probably about 18 quintillion. 
static const FrameID kNullFrame{std::numeric_limits<size_t>::max()};

struct MemoryStat {
	size_t allocated_frames;
	size_t total_frames;
};

class BitmapMemoryManager {
	public:
		static const auto kMaxPhysicalMemoryBytes{128_GiB};
		static const auto kFrameCount{kMaxPhysicalMemoryBytes / kBytesPerFrame};

		using MapLineType = unsigned long;
		// sizeof(unsigned long) is 8. unit is byte.
		static const size_t kBitsPerMapLine{8 * sizeof(MapLineType)};

		BitmapMemoryManager();

		WithError<FrameID> Allocate(size_t num_frames);
		Error Free(FrameID start_frame, size_t num_frames);
		void MarkAllocated(FrameID start_frame, size_t num_frames);

		void SetMemoryRange(FrameID range_begin, FrameID range_end);

		MemoryStat Stat() const;

	private:
		// 1bit as 1 frame. so number of bit of array content affect number of contents. 
		std::array<MapLineType, kFrameCount / kBitsPerMapLine> alloc_map_;
		FrameID range_begin_;
		FrameID range_end_;

		bool GetBit(FrameID frame) const;
		void SetBit(FrameID frame, bool allocated);
};

extern BitmapMemoryManager* memory_manager;
void InitializeMemoryManager(const MemoryMap& memory_map);
