#pragma once

#include <array>

class Error {
	public:
		enum Code {
			kSuccess,
			kFull,
			kEmpty,
			kNoEnoughMemory,
			kIndexOutOfRange,
			kHostControllerNotHalted,
			kInvalidSlotID,
			kPortNotConnected,
			kInvalidEndpointNumber,
			kTransferRingNotSet,
			kAlreadyAllocated,
			kNotImplemented,
			kInvalidDescriptor,
			kBufferTooSmall,
			kUnknownDevice,
			kNoCorrespondingSetupStage,
			kTransferFailed,
			kInvalidPhase,
			kUnknownXHCISpeedID,
			kNoWaiter,
			kNoPCIMSI,
			kUnknownPixelFormat,
			kTest1,
			kTest2,
			kTest3,
			kLastOfCode,
		};


	private:
		static constexpr std::array code_names_{
			"kSuccess",
			"kFull",
			"kEmpty",
			"kNoEnoughMemory",
			"kIndexOutOfRange",
			"kHostControllerNotHalted",
			"kInvalidSlotID",
			"kPortNotConnected",
			"kInvalidEndpointNumber",
			"kTransferRingNotSet",
			"kAlreadyAllocated",
			"kNotImplemented",
			"kInvalidDescriptor",
			"kBufferTooSmall",
			"kUnknownDevice",
			"kNoCorrespondingSetupStage",
			"kTransferFailed",
			"kInvalidPhase",
			"kUnknownXHCISpeedID",
			"kNoWaiter",
			"kNoPCIMSI",
			"kUnknownPixelFormat",
			"kTest1",
			"kTest2",
			"kTest3"
		};

		// static_assert is called when it is compiled. if condition is false, it occur compile error
		static_assert(Error::Code::kLastOfCode == code_names_.size());
	
	public:
		Error(Code code, const char* file, int line) : code_{code}, line_{line}, file_{file} {}

		Code Cause() const {
			return this->code_;
		}

		// operator bool() represent process when this class use as bool type. const means this method doesn't change field of this class. in c, const with this meaning place after method name
		operator bool() const {
			return this->code_ != kSuccess;
		}

		// first "const" is type of return value.
		const char* Name() const {
			return code_names_[static_cast<int>(this->code_)];
		}

		const char* File() const {
			return this->file_;
		}

		int Line() const {
			return this->line_;
		}

	private:
		Code code_;
		int line_;
		const char* file_;
};

// #define means declaration of macro. macro can use predefined macro
// __FILE__ and __LINE__ is predefined macro. __FILE__ represent current source file. __LINE__ represent current number of row
#define MAKE_ERROR(code) Error((code), __FILE__, __LINE__)

template <class T>
struct WithError {
	T value;
	Error error;
};
