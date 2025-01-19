#include <new>
#include <cerrno>

// in C++ std:: means standard library of C++. already there is a std::get_new_handler() method that returns std::new_handler. and below overwrite former method. in C++ we cant override method of standard library, but we can redeclare with same name and same return value and overwrite former method
std::new_handler std::get_new_handler() noexcept {
	return nullptr;
}

// this method also overwrite former method. Posix is C library, so declare extern "C" and not need to write prefix like "std::" 
extern "C" int posix_memalign(void**, size_t, size_t) {
	return ENOMEM;
}
