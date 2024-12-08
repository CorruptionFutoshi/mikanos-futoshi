TARGET = kernel.elf
OBJS = main.o graphics.o font.o hankaku.o newlib_support.o console.o
# OBJS:.o=.d means change .o file to .d file. notdir means remove path. addprefix . means add '.' to each file. dir means directory path like './'. join means combine like ./ + main.d => ./.main.d
DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))

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
	ld.lld $(LDFLAGS) -o kernel.elf $(OBJS) -lc

# $< means first prereq
%.o: %.cpp Makefile
	clang++ $(CPPFLAGS) $(CXXFLAGS) -c $<

# -MM means dependency. > means output redirect. $@ means target 
# eval is declaration. $(< means $<. in this, OBJ = main.o
# sed 's|x|y|' z means replace x to y in z file. 
.%.d: %.cpp
	clang++ $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

%.o: %.c Makefile
	clang $(CPPFLAGS) $(CFLAGS) -c $<

.%.d: %.c
	clang $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)' $@

hankaku.bin: hankaku.txt
	../tools/makefont.py -o $@ $<

hankaku.o: hankaku.bin
	objcopy -I binary -O elf64-x86-64 -B i386:x86-64 $< $@

.%.d: %.bin
	touch $@

# $(MAKE) means make commands. make commands often have difference depends on environment.
.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

#  run when makefile is loaded
-include $(DEPENDS)

