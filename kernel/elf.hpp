#pragma once

#include <stdint.h>

// declare type that use in field of 64bit Elf
typedef uintptr_t	Elf64_Addr;
typedef uint64_t	Elf64_Off;
typedef uint16_t	Elf64_Half;
typedef uint16_t	Elf64_Word;
typedef int32_t		Elf64_Sword;
typedef uint64_t	Elf64_Xword;
typedef int64_t		Elf64_Sxword;

// #define means set of variable and value. e_ident field is always 16 byte so declare as 16
#define EI_NIDENT	16

// this struct Elf64_Ehdr represent file header of 64bit Elf. file header of 64bit Elf is singular in Elf
typedef struct {
	unsigned char e_ident[EI_NIDENT];
	Elf64_Half	e_type;
	Elf64_Half	e_machine;
	Elf64_Word	e_version;
	Elf64_Addr	e_entry;
	Elf64_Off	e_phoff;
	Elf64_Off	e_shoff;
	Elf64_Word	e_flags;
	Elf64_Half	e_ehsize;
	Elf64_Half	e_phentsize;
	Elf64_Half	e_phnum;
	Elf64_Half	e_shentsize;
	Elf64_Half	e_shnum;
	Elf64_Half	e_shstrndx;
} Elf64_Ehdr;

// this struct Elf64_Phdr represent program header of 64bit Elf. program header of 64bit Elf is array in Elf
// one program header represent one segment
typedef struct {
Elf64_Word	p_type;
Elf64_Word	p_flags;
Elf64_Off	p_offset;
Elf64_Addr	p_vaddr;
Elf64_Addr	p_paddr;
// filesz is size of segment that already exist in Elf file
Elf64_Xword	p_filesz;
// memsz is size of segment that will be used in runtime 
Elf64_Xword	p_memsz;
Elf64_Xword	p_align;
} Elf64_Phdr;

// declare program header types. these are used as a value of p_type
#define	PT_NULL		0
#define	PT_LOAD		1
#define	PT_DYNAMIC	2
#define	PT_INTERP	3
#define	PT_NOTE		4
#define	PT_SHLIB	5
#define	PT_PHDR		6
#define	PT_TLS		7

// this struct Elf64_Dyn represent dynamic section of 64bit elf. by the way including this, below code is not be used in this program. 
typedef struct {
	Elf64_Sxword	d_tag;
	union {
		Elf64_Xword d_val;
		Elf64_Addr d_ptr;
	} d_un;
} Elf64_Dyn;

#define	DT_NULL		0
#define	DT_RELA		7
#define	DT_RELASZ	8
#define	DT_RELAENT	9

typedef struct {
	Elf64_Addr r_offset;
	Elf64_Xword r_info;
	Elf64_Sxword r_addend;
} Elf64_Rela;

#define ELF64_R_SYM(i)		((i)>>32)
#define ELF64_R_TYPE(i)		((i)&0xffffffffL)
#define	ELF64_R_INFO(s,t)	(((S)<<32)+((t)&0xffffffffL))

#define R_X86_64_RELATIVE 8













