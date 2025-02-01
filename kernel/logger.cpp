#include "logger.hpp"

#include <cstddef>
#include <cstdio>

#include "console.hpp"

// namespace {} with no name is unnamed namespace. variables or methods declared in this space is only be used in same file. 
namespace {
	LogLevel log_level = kWarn;
}

// extern means this variable is already declared in other source file. and this variable point value of variable declared in other file. in C++ we can use global variable declared in other source file, but compiler cant compile them. so we use extern declaration or include to tell compiler to use global variable. 
extern Console* console;

void SetLogLevel(LogLevel level) {
	log_level = level;
}

int Log(LogLevel level, const char* format, ...) {
	if (level > log_level) {
		return 0;
	}

	va_list ap;
	int result;
	char s[1024];

	// va_start() require last parameter before variable length parameter as second parameter
	va_start(ap, format);
	result = vsprintf(s, format, ap);
	va_end(ap);

	console->PutString(s);
	return result;
}
