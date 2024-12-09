#pragma once

#include <array>

class Error {
	public:
		enum Code {
			kSuccess,
			kFull,
			kEmpty,
			kLastOfCode,
		};

		Error(Code code) : code_{code} {}

		// operator bool() represent process when this class use as bool type. const means this method doesn't change field of this class. in c, const with this meaning place after method name
		operator bool() const {
			return this->code_ != kSuccess;
		}

		// first "const" is type of return value.
		const char* Name() const {
			return code_names_[static_cast<int>(this->code_)];
		}

	private:
		static constexpr std::array<const char*, 3> code_names_ = {
			"kSuccess",
			"kFull",
			"kEmpty",
		};

		Code code_;
};
