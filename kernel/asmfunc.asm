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

global GetCR0
GetCR0:
	mov rax, cr0
	ret

global SetCR0
SetCR0:
	mov cr0, rdi
	ret

global GetCR2
GetCR2:
	mov rax, cr2
	ret

global SetCR3
SetCR3:
	mov cr3, rdi	; cr3 register represent physical address of PML4 table
	ret

global GetCR3
GetCR3:
	mov rax, cr3
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
	
global SwitchContext
SwitchContext:
	mov [rsi + 0x40], rax
	mov [rsi + 0x48], rbx
	mov [rsi + 0x50], rcx
	mov [rsi + 0x58], rdx
	mov [rsi + 0x60], rdi
	mov [rsi + 0x68], rsi

	lea rax, [rsp + 8]
	mov [rsi + 0x70], rax
	mov [rsi + 0x78], rbp

	mov [rsi + 0x80], r8
	mov [rsi + 0x88], r9
	mov [rsi + 0x90], r10
	mov [rsi + 0x98], r11
	mov [rsi + 0xa0], r12
	mov [rsi + 0xa8], r13
	mov [rsi + 0xb0], r14
	mov [rsi + 0xb8], r15

	mov rax, cr3
	mov [rsi + 0x00], rax
	mov rax, [rsp]
	mov [rsi + 0x08], rax
	pushfq
	pop qword [rsi + 0x10]

	mov ax, cs
	mov [rsi + 0x20], rax
	mov bx, ss
	mov [rsi + 0x28], rbx
	mov cx, fs
	mov [rsi + 0x30], rcx
	mov dx, gs
	mov [rsi + 0x38], rdx

	fxsave [rsi + 0xc0]
	
global RestoreContext
RestoreContext:
	push qword [rdi + 0x28]
	push qword [rdi + 0x70]
	push qword [rdi + 0x10]
	push qword [rdi + 0x20]
	push qword [rdi + 0x08]
	
	fxrstor [rdi + 0xc0]	

	mov rax, [rdi + 0x00]
	mov cr3, rax
	mov rax, [rdi + 0x30]
	mov fs, ax
	mov rax, [rdi + 0x38]
	mov gs, ax

	mov rax, [rdi + 0x40]
	mov rbx, [rdi + 0x48]
	mov rcx, [rdi + 0x50]
	mov rdx, [rdi + 0x58]
	mov rsi, [rdi + 0x68]
	mov rbp, [rdi + 0x78]
	mov r8,  [rdi + 0x80]
	mov r9,  [rdi + 0x88]
	mov r10, [rdi + 0x90]
	mov r11, [rdi + 0x98]
	mov r12, [rdi + 0xa0]
	mov r13, [rdi + 0xa8]
	mov r14, [rdi + 0xb0]
	mov r15, [rdi + 0xb8]
	
	mov rdi, [rdi + 0x60]

	o64 iret

global CallApp
CallApp:
	push rbx
	push rbp
	push r12
	push r13
	push r14
	push r15
	mov [r9], rsp

	push rdx
	push r8
	add rdx, 8
	push rdx
	push rcx
	o64 retf

extern LAPICTimerOnInterrupt

global IntHandlerLAPICTimer
IntHandlerLAPICTimer:
	push rbp
	mov rbp, rsp

	sub rsp, 512
	fxsave [rsp]
	push r15
	push r14
	push r13
	push r12
	push r11
	push r10
	push r9
	push r8
	push qword [rbp]
	push qword [rbp + 0x20]
	push rsi
	push rdi
	push rdx
	push rcx
	push rbx
	push rax

	mov ax, fs
	mov bx, gs
	mov rcx, cr3

	push rbx
	push rax
	push qword [rbp + 0x28]
	push qword [rbp + 0x10]
	push rbp
	push qword [rbp + 0x18]
	push qword [rbp + 0x08]
	push rcx

	mov rdi, rsp
	call LAPICTimerOnInterrupt

	add rsp, 8*8
	pop rax
	pop rbx
	pop rcx
	pop rdx
	pop rdi
	pop rsi
	add rsp, 16
	pop r8
	pop r9
	pop r10
	pop r11
	pop r12
	pop r13
	pop r14
	pop r15
	fxrstor [rsp]

	mov rsp, rbp
	pop rbp
	iretq

global LoadTR
LoadTR:
	ltr di
	ret

global WriteMSR
WriteMSR:
	mov rdx, rsi
	shr rdx, 32
	mov eax, esi
	mov ecx, edi
	wrmsr
	ret

extern GetCurrentTaskOSStackPointer
extern syscall_table
global SyscallEntry
SyscallEntry:
	push rbp
	push rcx
	push r11

	push rax

	mov rcx, r10
	and eax, 0x7fffffff
	mov rbp, rsp

	and rsp, 0xfffffffffffffff0
	push rax
	push rdx
	cli
	call GetCurrentTaskOSStackPointer
	sti
	mov rdx, [rsp + 0]
	mov [rax - 16], rdx
	mov rdx, [rsp + 8]
	mov [rax - 8], rdx

	lea rsp, [rax - 16]
	pop rdx
	pop rax
	and rsp, 0xfffffffffffffff0	

	call [syscall_table + 8 * eax]

	mov rsp, rbp

	pop rsi
	cmp esi, 0x80000002
	je .exit

	pop r11
	pop rcx
	pop rbp
	o64 sysret

.exit:
	mov rdi, rax
	mov esi, edx
	jmp ExitApp
	
global ExitApp
ExitApp:
	mov rsp, rdi
	mov eax, esi

	pop r15
	pop r14
	pop r13
	pop r12
	pop rbp
	pop rbx
	
	ret

global InvalidateTLB
InvalidateTLB:
	invlpg [rdi]
	ret
