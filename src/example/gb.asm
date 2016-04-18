	
	cpu	gameboy

	option	output-file,gb.gb
	option	output-format,gameboy

	option	gameboy-irq,vbl,vbl_code

VRAM	equ	$8000
SCRN	equ	$9800
OAM	equ	$fe00
LCDC	equ	$ff40
STAT	equ	$ff41
BGRDPAL	equ	$ff47
OBJ0PAL	equ	$ff48
OBJ1PAL	equ	$ff49
CURLINE	equ	$ff44

READY	equ	$ff81
XPOS	equ	$ff82

VBLANK	macro
;	push	af
;.wait
;	ldh	a,(CURLINE)
;	cp	144
;	jr	nz,wait
;
;	pop	af

	endm


	;
	; **********
	; CODE START
	; **********
	;
	org	$150

	di
	xor	a
	ldh	(READY),a
	ld	sp,$fffe

	; Set LCD so only sprites show
	;
	VBLANK

	ld	a,$81
	ldh	(LCDC),a
	ld	a,$10
	ldh	(STAT),a
	ld	a,$e4
	ldh	(BGRDPAL),a
	ldh	(OBJ0PAL),a
	ldh	(OBJ1PAL),a

	; Copy to VRAM
	;
	ld	hl,VRAM
	ld	de,sprite
	ld	a,16

	VBLANK

.copy
	ld	a,(de)
	ld	(hl+),a
	inc	de
	dec	a
	jr	nz,copy

	ld	a,1
	ldh	(READY),a

.idle
	ei
	halt
	nop
	jr	idle

vbl_code:
	ldh	a,(READY)
	or	a
	jr	z,finish

	ldh	a,(XPOS)
	inc	a
	ldh	(XPOS),a
	ld	(OAM),a
	ld	(OAM+1),a
	xor	a
	ld	(OAM+2),a
	ld	(OAM+3),a

.finish
	reti

sprite:
	defb	$ff,00
	defb	$ff,00
	defb	$ff,00
	defb	$ff,00
	defb	$ff,00
	defb	$ff,00
	defb	$ff,00
	defb	$ff,00

