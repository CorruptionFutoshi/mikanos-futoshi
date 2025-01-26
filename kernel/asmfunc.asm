bits 64
section .text

global IoOut32
IoOut32:
	mov dx, di	; in System V AMD64 ABI first parameter store in RDI, second parameter store in RSI
	mov eax, esi
	out dx, eax	; write value of eax to place specified with IO port address of dx
	ret

global IoIn32
IoIn32:
	mov dx, di
	in eax, dx	; write value of place specified with IO port address of dx to eax
	ret		; in System V AMD64 ABI return value is value of RAX

global GetCS
GetCS:
	xor eax, eax	; clear upper 32 bits of rax
	mov ax, cs	; cs register have information about currentry using code segment
	ret

global LoadIDT
LoadIDT:
	push rbp	; this method is more complicated than others. so not omit this
	mov rbp, rsp
	sub rsp, 10	; sub means subtraction. rsp - 10(create 10 byte space)
	mov [rsp], di	; di is first parameter(16bit)
	mov [rsp + 2], rsi	; rsi is second parameter(64bit)
	lidt [rsp]	; lidt means load idt
	mov rsp, rbp
	pop rbp
	ret

global LoadGDT
LoadGDT:
	push rbp
	mov rbp, rsp
	sub rsp, 10
	mov [rsp], di	;di is first parameter(limit)
	mov [rsp + 2], rsi	; rsi is second parameter(offset)
	lgdt [rsp]	; lgdt means load gdt
	mov rsp, rbp
	pop rbp
	ret

global SetCSSS
SetCSSS:
	push rbp
	mov rbp, rsp
	mov ss, si	; si is second parameter(ss)
	mov rax, .next	; set .next label to rax
	push rdi	; rdi is first parameter(cs)
	push rax	; push rax as rip(next order)
	o64 retf	; retf set value in stack to CS and RIP registory.
.next:
	mov rsp, rbp 
	pop rbp
	ret

global SetDSAll
SetDSAll:
	mov ds, di
	mov es, di
	mov fs, di
	mov gs, di
	ret

global SetCR3
SetCR3:
	mov cr3, rdi	; cr3 register represent physical address of PML4 table
	ret

extern kernel_main_stack
extern KernelMainNewStack

global KernelMain
KernelMain:
	mov rsp, kernel_main_stack + 1024 * 1024	; kernel_main_stack is head address of 1MiB stack. and stack grow higher to lower. so add 1MiB to last address to create start address. 
	call KernelMainNewStack
.fin:
	hlt
	jmp .fin
	

