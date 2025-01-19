#pragma once

#include <cstddef>
#include <cstdint>

template <typename T>
struct ArrayLength {};

// size_t is unsigned integer type that is basically used to represent size of object.
template <typename T, size_t N>
struct ArrayLength<T[N]> { 
	static const size_t value = N;
};

// this struct represent MemoryMappedRegister including register using MMIO.
template <typename T>
class MemMapRegister {
	public:
		T Read() const {
			T tmp;

			for (size_t i = 0; i < len_; ++i) {
				tmp.data[i] = value_.data[i];	
			}

			return tmp;
		}

		void Write(const T& value) {
			for (size_t i = 0; i < len_; ++i) {
				value_.data[i] = value.data[i];
			} 
		}

	private:
		// volatile is used to tell compiler that don't optimize this valiable.
		// i don't know when this field is assigned. always call Write() before Read()? 
		volatile T value_;
		// decltype() represent type of parameter. 
		// T should have "data" array field. when this class is create, automatically number of T::data set to len_.
		static const size_t len_ = ArrayLength<decltype(T::data)>::value;
};

template <typename T>
struct DefaultBitmap {
	T data[1];

	DefaultBitmap& operator = (const T& value) {
		data[0] = value;
	}

	// operator {type} () is type conversion operator. for example,
	// DefaultBitmap<uint32_t> bitamp = 0xFFFFFFFFU;
	// uint32_t result = bitmap. (in this, type conversion execute)
	operator T() const { return data[0]; }
};

template <typename T>
class ArrayWrapper {
	public:
		// this using is so-called type alias. it create sinonym of already declared type.
		using ValueType = T;
		using Iterator = ValueType*;
		using ConstIterator = const ValueType*;

		ArrayWrapper(uintptr_t array_base_addr, size_t size)
			: array_(reinterpret_cast<ValueType*>(array_base_addr)),
			  size_(size) {}

		size_t Size() const { return size_; }

		// begin() and end() method is necessary to use the range-based for statement.
		Iterator begin() { return array_; }
		Iterator end() { return array_ + size_; }
		ConstIterator cbegin() const { return array_; }
		ConstIterator cend() const { return array_ + size_; }

		// in c++, pointor[] is return value that pointor + index point. array_[index] equalls *(array_ + index).
		ValueType& operator [](size_t index) { return array_[index]; }

	private:
		ValueType* const array_;
		const size_t size_;
};
