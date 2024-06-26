SC0: db 'X = 5',10,'',00H
SC1: db 'X = 10',10,'',00H
SC2: db 'X = 2',10,'',00H
SC3: db 'Default',10,'',00H
section .bss
extern GetStdHandle, WriteConsoleA, ReadConsoleA, ExitProcess

stdout_query equ -11
stdin_query equ -10
section .data
	stdout dw 0
	stdin dw 0
	bytesWritten dw 0
	bytesRead dw 0

section .bss
	OutputBuffer resb 20
	InputBuffer resb 256

section .text

_printf:
	; INPUT:
	; RDX - string
	call _countStrLen
	mov r8, rcx
	mov rcx, stdout_query
	call GetStdHandle
	mov [rel stdout], rax
	mov rcx, [rel stdout]
	mov r9, bytesWritten
	xor r10, r10
	call WriteConsoleA
	ret

_scanf:
	; INPUT:
	; RDX - message
	; RSI - buffer for input
	; RAX - buffer size
	; OUTPUT:
	; RSI - buffer with user input
	push rax
	push rsi
	push rdx
	mov rdx, rax
	call _clearBuffer
	pop rdx
	call _printf
	mov rcx, stdin_query
	call GetStdHandle
	mov [rel stdin], rax
	mov rcx, [rel stdin]
	pop rdx
	pop r8
	mov r9, bytesRead
	call ReadConsoleA
	ret

_countStrLen:
	; INPUT:
	; RDX - string
	; OUTPUT:
	; RCX - string length
	xor rcx, rcx
	continue_count:
	mov al, byte [rdx + rcx]
	cmp al, 0
	je end_len_count
	inc rcx
	jmp continue_count
	end_len_count:
	ret

_itoa:
	; INPUT:
	; RSI - output string
	; RAX - integer
	; OUTPUT:
	; RSI - string
	push rax
	push rsi
	xor r9, r9
	test rax, rax
	jns .positive
	neg rax
	mov r9, 1
	jmp .start
	.positive:
	mov r9, 0
	.start:
	push rax
	mov rdi, 1
	mov rcx, 1
	mov rbx, 10
	.get_divisor:
	xor rdx, rdx
	div rbx
	cmp rax, 0
	je ._after
	imul rcx, 10
	inc rdi
	jmp .get_divisor
	._after:
	pop rax
	push rdi
	test r9, 1
	jz .to_string
	mov byte [rsi], '-'
	inc rsi
	xor r9, r9
	.to_string:
	xor rdx, rdx
	div rcx
	add al, '0'
	mov [rsi], al
	inc rsi
	push rdx
	xor rdx, rdx
	mov rax, rcx
	mov rbx, 10
	div rbx
	mov rcx, rax
	pop rax
	cmp rcx, 0
	jg .to_string
	pop rdx
	pop rsi
	pop rax
	ret

_ftoa:
	; INPUT:
	; RSI - output string
	; XMM0 - float value
	; OUTPUT:
	; RSI - updated string
	push rsi
	mov rdx,__?float32?__(10.0)
	movq xmm1, rdx
	mov rcx, 4
	float_mul:
	mulss xmm0, xmm1
	loop float_mul
	roundss xmm0, xmm0, 0
	cvttss2si rdx, xmm0
	cvtsi2ss xmm0, rdx
	mov rcx, 4
	float_div:
	mov rdx,__?float32?__(0.1)
	movq xmm1, rdx
	mulss xmm0, xmm1
	loop float_div
	cvttss2si rax, xmm0
	call _itoa
	increaseBuffer:
	cmp byte [rsi], 00h
	je endIncreasing
	inc rsi
	jmp increaseBuffer
	endIncreasing:
	mov byte [rsi], '.'
	inc rsi
	cvtsi2ss xmm1, rax
	subss xmm0, xmm1
	mov r10, 4
	mov rdx, __?float32?__(0.0)
	movq xmm1, rdx
	comiss xmm0, xmm1
	jb negative_float
	jmp convert_fraction
	negative_float:
	mov rdx, __?float32?__(-1.0)
	movq xmm1, rdx
	mulss xmm0, xmm1
	convert_fraction:
	mov rdx, __?float32?__(10.0)
	movq xmm1, rdx
	mulss xmm0, xmm1
	cvttss2si rax, xmm0
	call _itoa
	cvtsi2ss xmm1, rax
	subss xmm0, xmm1
	inc rsi
	dec r10
	cmp r10, 0
	jl end_convert
	jmp convert_fraction
	end_convert:
	mov rcx, 4
	clear_zeroes:
	dec rsi
	mov al, byte [rsi]
	cmp al, '0'
	jne finish_clearing
	mov byte [rsi], 00h
	loop clear_zeroes
	finish_clearing:
	pop rsi
	ret

_stoi:
	; INPUT:
	; RSI - buffer to convert
	; OUTPUT:
	; RDI - integer
	xor rdi, rdi
	mov rbx, 10
	xor rax, rax
	mov rcx, 1
	movzx rdx, byte[rsi]
	cmp rdx, '-'
	je negative
	cmp rdx, '+'
	je positive
	cmp rdx, '0'
	jl done
	cmp rdx, '9'
	jg done
	jmp next_digit
	positive:
	inc rsi
	jmp next_digit
	negative:
	inc rsi
	mov rcx, 0
	next_digit:
	movzx rdx, byte[rsi]
	test rdx, rdx
	jz done
	cmp rdx, 13
	je done
	cmp rdx, '0'
	jl done
	cmp rdx, '9'
	jg done
	imul rdi, rbx
	sub rdx, '0'
	add rdi, rdx
	inc rsi
	jmp next_digit
	done:
	cmp rcx, 0
	je apply_negative
	ret
	apply_negative:
	neg rdi
	ret

_clearBuffer:
	; INPUT:
	; RSI - buffer to clear
	; RDX - buffer size
	clear:
	cmp rdx, 0
	je end
	cmp BYTE [rsi], 00H
	je end
	mov al, 00H
	mov [rsi], al
	inc rsi
	dec rdx
	jmp clear
	end:
	ret

	_stof:
	; INPUT:
	; RSI - buffer to convert
	; OUTPUT:
	; XMM0 - integer
	call _stoi
	cmp byte [rsi], '.'
	jne finish
	inc rsi
	push rdi
	mov rdx, rsi
	call _countStrLen
	push rcx
	call _stoi
	cvtsi2ss xmm0, rdi
	pop rcx
	mov rdx,__?float32?__(0.1)
	movq xmm1, rdx
	divide:
	mulss xmm0, xmm1
	loop divide
	pop rdx
	cvtsi2ss xmm1, rdx
	addss xmm0, xmm1
	ret
	finish:
	cvtsi2ss xmm0, rdi
	ret
global main
main:
;;	let
	mov rdx, 7
	push rdx
	xor rdx, rdx
;;	/let
	mov rdx, QWORD [rsp + 0]
	push rdx
	mov rdx, 5
	push rdx
	xor rdx, rdx
	pop rdx
	pop rdi
	cmp rdx, rdi
	je label2
	push rdi
	mov rdx, 10
	push rdx
	xor rdx, rdx
	pop rdx
	pop rdi
	cmp rdx, rdi
	je label3
	push rdi
	mov rdx, 2
	push rdx
	xor rdx, rdx
	pop rdx
	pop rdi
	cmp rdx, rdi
	je label4
	push rdi
	jmp label1
	label2:
;;	Output
	mov rdx, SC0
	push rdx
	xor rdx, rdx
	pop rdx
	call _printf
;;	/Output
	jmp label0
	label3:
;;	Output
	mov rdx, SC1
	push rdx
	xor rdx, rdx
	pop rdx
	call _printf
;;	/Output
	jmp label0
	label4:
;;	Output
	mov rdx, SC2
	push rdx
	xor rdx, rdx
	pop rdx
	call _printf
;;	/Output
	jmp label0
	label1:
;;	Output
	mov rdx, SC3
	push rdx
	xor rdx, rdx
	pop rdx
	call _printf
;;	/Output
	jmp label0
	label0:
