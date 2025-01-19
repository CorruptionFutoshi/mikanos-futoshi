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
