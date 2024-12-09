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
