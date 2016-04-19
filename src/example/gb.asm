	;
	; This example is poor, and leaves lots of junk lymg in memory.
	; Still, it works enough to test
	;
	
	cpu	gameboy

	option	output-file,gb.gb
	option	output-format,gameboy

	option	gameboy-irq,vbl,vbl_code

VRAM	equ	$8000
SCRN	equ	$9800
OAM	equ	$fe00
LCDC	equ	$ff40
STAT	equ	$ff41
ISWITCH	equ	$ffff
BGRDPAL	equ	$ff47
OBJ0PAL	equ	$ff48
OBJ1PAL	equ	$ff49
CURLINE	equ	$ff44

XPOS	equ	$ff82

VBLANK	macro
	push	af
.wait
	ldh	a,(CURLINE)
	cp	144
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
	ld	sp,$fffe

	; Switch of display during setup
	;
	VBLANK
	xor	a
	ld	(LCDC),a

	ld	(XPOS),a

	ld	a,$e4
	ldh	(OBJ0PAL),a
	ldh	(OBJ1PAL),a

	swap	a
	ldh	(BGRDPAL),a

	; Copy to VRAM and set up
	;
	ld	hl,VRAM
	ld	de,sprite
	ld	c,16

.copy
	ld	a,(de)
	ld	(hl+),a
	inc	de
	dec	c
	jr	nz,copy


	; Set sprite numbers
	;
	xor	a
	ld	(OAM+2),a
	ld	(OAM+6),a
	ld	(OAM+10),a

	; Set sprite flags
	;
	ld	a,$80
	ld	(OAM+3),a
	ld	(OAM+7),a
	ld	(OAM+11),a

	; Set LCD back on
	;
	ld	a,$83
	ldh	(LCDC),a

	; Activate VBL
	;
	ld	a,1
	ldh	(ISWITCH),a

	ei

.idle
	halt
	nop
	jr	idle

vbl_code:
	ldh	a,(XPOS)
	inc	a
	ldh	(XPOS),a

	ld	(OAM),a
	ld	(OAM+1),a

	add	20
	ld	(OAM+4),a
	ld	(OAM+5),a

	add	33
	ld	(OAM+8),a
	ld	(OAM+9),a

	reti

sprite:
	defb	$ff,$ff
	defb	$00,00
	defb	$ff,$ff
	defb	$00,00
	defb	$ff,$ff
	defb	$00,00
	defb	$ff,$ff
	defb	$00,00
	defb	$ff,$ff
	defb	$00,00

	;
	; Assembly checks
	;
	stop
	swap	a
	swap	b
	swap	c
	swap	d
	swap	e
	swap	h
	swap	l
	swap	(hl)
	ld	a,(hl+)
	ld	a,(hli)
	ldi	a,(hl)
	ld	a,(hl-)
	ld	a,(hld)
	ldd	a,(hl)
