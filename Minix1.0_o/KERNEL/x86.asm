; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                               KERNEL.asm
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
;                                                     Toby Du, 2013
; ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++#

%include "sconst.inc"
; import function
extern	cstart
extern	exception_handler
extern	spurious_irq
extern	main
extern	sys_call

; inport variable
extern	gdt_ptr
extern	idt_ptr
extern	disp_pos
extern	k_reenter
extern	proc_ptr
extern	tss
extern	irq_table
extern	cur_proc

; outport variable
global sizes

bits 32
[SECTION .bss]
StackSpace		resb	2 * 1024
StackTop:		; top of stack


[section .text]	; code here

global _start	; ecport _start to link

global restart
global system_call

global	divide_error
global	single_step_exception
global	nmi
global	breakpoint_exception
global	overflow
global	bounds_check
global	inval_opcode
global	copr_not_available
global	double_fault
global	copr_seg_overrun
global	inval_tss
global	segment_not_present
global	stack_exception
global	general_protection
global	page_fault
global	copr_error
global	hwint00
global	hwint01
global	hwint02
global	hwint03
global	hwint04
global	hwint05
global	hwint06
global	hwint07
global	hwint08
global	hwint09
global	hwint10
global	hwint11
global	hwint12
global	hwint13
global	hwint14
global	hwint15

_start:

	; mov esp from LOADER to KERNEL
	mov	esp, StackTop	
	mov	dword [disp_pos], 0
	sgdt	[gdt_ptr]	
	call	cstart		
	lgdt	[gdt_ptr]
	
	lidt	[idt_ptr]

	

	jmp	SELECTOR_KERNEL_CS:csinit
csinit:	
	xor	eax, eax
	mov	ax, SELECTOR_TSS
	ltr	ax
	jmp	main	


; interrupt & exception -- hardware interrupt
; -------------------------------------------
%macro	hwint_master	1
	call	save
	in	al, INT_M_CTLMASK	
	or	al, (1 << %1)		
	out	INT_M_CTLMASK, al	
	mov	al, EOI			
	out	INT_M_CTL, al		
	sti	
	push	%1			
	call	[irq_table + 4 * %1]	
	pop	ecx			
	cli
	in	al, INT_M_CTLMASK	
	and	al, ~(1 << %1)		
	out	INT_M_CTLMASK, al	
	ret
%endmacro


ALIGN	16
hwint00:		; Interrupt routine for irq 0 (the clock).
	hwint_master	0

ALIGN	16
hwint01:		; Interrupt routine for irq 1 (keyboard)
	hwint_master	1

ALIGN	16
hwint02:		; Interrupt routine for irq 2 (cascade!)
	hwint_master	2

ALIGN	16
hwint03:		; Interrupt routine for irq 3 (second serial)
	hwint_master	3

ALIGN	16
hwint04:		; Interrupt routine for irq 4 (first serial)
	hwint_master	4

ALIGN	16
hwint05:		; Interrupt routine for irq 5 (XT winchester)
	hwint_master	5

ALIGN	16
hwint06:		; Interrupt routine for irq 6 (floppy)
	hwint_master	6

ALIGN	16
hwint07:		; Interrupt routine for irq 7 (printer)
	hwint_master	7

; ---------------------------------
%macro	hwint_slave	1
	call	save
	in	al, INT_S_CTLMASK	
	or	al, (1 << (%1 - 8))	
	out	INT_S_CTLMASK, al	
	mov	al, EOI			
	out	INT_S_CTL, al		
	sti	
	push	%1			
	call	[irq_table + 4 * %1]	
	pop	ecx			
	cli
	in	al, INT_S_CTLMASK	
	and	al, ~(1 << (%1 - 8))	
	out	INT_S_CTLMASK, al	
	ret
%endmacro
; ---------------------------------

ALIGN	16
hwint08:		; Interrupt routine for irq 8 (realtime clock).
	hwint_slave	8

ALIGN	16
hwint09:		; Interrupt routine for irq 9 (irq 2 redirected)
	hwint_slave	9

ALIGN	16
hwint10:		; Interrupt routine for irq 10
	hwint_slave	10

ALIGN	16
hwint11:		; Interrupt routine for irq 11
	hwint_slave	11

ALIGN	16
hwint12:		; Interrupt routine for irq 12
	hwint_slave	12

ALIGN	16
hwint13:		; Interrupt routine for irq 13 (FPU exception)
	hwint_slave	13

ALIGN	16
hwint14:		; Interrupt routine for irq 14 (AT winchester)
	hwint_slave	14

ALIGN	16
hwint15:		; Interrupt routine for irq 15
	hwint_slave	15



; interrupt & exception -- exceptions
;-----------------------------------------
divide_error:
	push	0xFFFFFFFF	; no err code
	push	0		; vector_no	= 0
	jmp	exception
single_step_exception:
	push	0xFFFFFFFF	; no err code
	push	1		; vector_no	= 1
	jmp	exception
nmi:
	push	0xFFFFFFFF	; no err code
	push	2		; vector_no	= 2
	jmp	exception
breakpoint_exception:
	push	0xFFFFFFFF	; no err code
	push	3		; vector_no	= 3
	jmp	exception
overflow:
	push	0xFFFFFFFF	; no err code
	push	4		; vector_no	= 4
	jmp	exception
bounds_check:
	push	0xFFFFFFFF	; no err code
	push	5		; vector_no	= 5
	jmp	exception
inval_opcode:
	push	0xFFFFFFFF	; no err code
	push	6		; vector_no	= 6
	jmp	exception
copr_not_available:
	push	0xFFFFFFFF	; no err code
	push	7		; vector_no	= 7
	jmp	exception
double_fault:
	push	8		; vector_no	= 8
	jmp	exception
copr_seg_overrun:
	push	0xFFFFFFFF	; no err code
	push	9		; vector_no	= 9
	jmp	exception
inval_tss:
	push	10		; vector_no	= A
	jmp	exception
segment_not_present:
	push	11		; vector_no	= B
	jmp	exception
stack_exception:
	push	12		; vector_no	= C
	jmp	exception
general_protection:
	push	13		; vector_no	= D
	jmp	exception
page_fault:
	push	14		; vector_no	= E
	jmp	exception
copr_error:
	push	0xFFFFFFFF	; no err code
	push	16		; vector_no	= 10h
	jmp	exception

exception:
	call	exception_handler
	add	esp, 4*2	;
	hlt


; ====================================================================================
;                                   save
; ====================================================================================
save:
	pushad		
	push	 ds	
	push	 es	
	push	 fs	
	push	 gs	
	mov	dx, ss
	mov	ds, dx
	mov	es, dx
	mov	fs, dx

	mov	esi, esp			

	inc	dword [k_reenter]		
	cmp	dword [k_reenter], 0		
	jne	.1				
	mov	esp, StackTop			
	push	restart				
	
	jmp	[esi + RETADR - P_STACKBASE]	
.1:						
	
	push	restart_reenter			
	
	jmp	[esi + RETADR - P_STACKBASE]	
						

; ====================================================================================
;                                 sys_call
; ====================================================================================
system_call:
	call	save
	;sti
	;inc	dword [k_reenter]
	push ecx	 ;m_ptr
	push ebx	 ;dest
	push dword [cur_proc] ;caller
	push eax        ;function
	call	sys_call
	add esp ,16
	;cli
	;ret
	
	


; ====================================================================================
;                                   restart
; ====================================================================================
restart:
	mov	esp, [proc_ptr]
	lldt	[esp + P_LDT_SEL] 
	lea	eax, [esp + P_STACKTOP]
	mov	dword [tss + TSS3_S_SP0], eax
restart_reenter:
	dec	dword [k_reenter]
	pop	 gs
	pop	 fs
	pop	 es
	pop	 ds
	popad
	add	esp, 4
	iretd




[section .data]	; data here
sizes	 DD	526Fh		; this must be the first data entry (magic nr)
 times 7 DD 0	; space	for build table	- total	16b
memorysize DD 0 ; memory size.

