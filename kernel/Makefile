TARGET = kernel.elf
OBJS = main.o

CXXFLAGS += -O2 -Wall -g --target=x86_64-elf -ffreestanding -mno-red-zone -fno-exceptions -fno-rtti -std=c++17
# --image-base 0x110000 means vaddress of segment Load of program header begin in 0x110000
LDFLAGS += --entry KernelMain -z norelro --image-base 0x110000 --static

# .PHONY means phony target
.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	rm -rf *.o

# write Makefile as prereq to build when Makefile update
kernel.elf: $(OBJS) Makefile
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS)

%.o: %.cpp Makefile
	# $< means first prereq
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $<
