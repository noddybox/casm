	
	cpu	gameboy

	option	output-file,gb.gb
	option	output-format,gameboy

	option	gameboy-irq,vbl,vbl_code

VRAM	equ	$8000
SCRN	equ	$9800
OAM	equ	$fe00
LCDC	equ	$ff40
STAT	equ	$ff41

READY	equ	$ff81
XPOS	equ	$ff82

VBLANK	macro
	push	af
.wait
	ldh	a,(LCDC)
	cp	$91
	jr	nz,wait

	pop	af

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
	ld	a,$82
	ldh	(LCDC),a
	ld	a,$10
	ldh	(STAT),a

	; Copy to VRAM
	;
	ld	hl,VRAM
	ld	de,sprite
	ld	a,16

	VBLANK

.copy
	ld	a,(hl+)
	ld	(de),a
	inc	de
	dec	a
	jr	nz,copy

	ld	a,1
	ldh	(READY),a

.idle
	halt
	nop
	jr	idle

vbl_code:
	ldh	a,(READY)
	jr	z,finish

	ldh	a,(XPOS)
	inc	a
	ldh	(XPOS),a
	ld	(OAM+1),a
	xor	a
	ld	(OAM),a
	ld	(OAM+2),a
	ld	(OAM+3),a

.finish
	reti

sprite:
	defs	16,$ff

