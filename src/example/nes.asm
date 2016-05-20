	processor 6502

	option  output-file,"nes.nes"
	option  output-format,nes

	option  nes-vector,reset,start
	option  nes-vector,nmi,nmi
	option  nes-vector,brk,nmi

start:	org	$c000

	;
	; Code taken from NES example
	;

	sei			; disable IRQs
	cld			; disable decimal mode
	ldx	#$40
	stx	$4017		; dsiable APU frame IRQ
	ldx	#$ff		; Set up stack
	txs			;  .
	inx			; now X = 0
	stx	$2000		; disable NMI
	stx	$2001		; disable rendering
	stx	$4010		; disable DMC IRQs

	;; first wait for vblank to make sure PPU is ready
vblankwait1:
	bit	$2002
	bpl	vblankwait1

clear_memory:
	lda	#$00
	sta	$0000, x
	sta	$0100, x
	sta	$0200, x
	sta	$0300, x
	sta	$0400, x
	sta	$0500, x
	sta	$0600, x
	sta	$0700, x
	inx
	bne	clear_memory

	;; second wait for vblank, PPU is ready after this
vblankwait2:
	bit	$2002
	bpl	vblankwait2

clear_palette:	
	;; Need clear both palettes to $00. Needed for Nestopia. Not
	;; needed for FCEU* as they're already $00 on powerup.
	lda	$2002		; Read PPU status to reset PPU address
	lda	#$3f		; Set PPU address to BG palette RAM ($3F00)
	sta	$2006
	lda	#$00
	sta 	$2006

	ldx	#$20		; Loop $20 times (up to $3F20)
	lda	#$00		; Set each entry to $00
.loop
	sta	$2007
	dex
	bne	loop

	lda	#%10000000	; intensify blues
	sta	$2001

forever:
	jmp	forever

nmi:
	rti


;
; Dummy VROM
;
	org 0
	bank 1

	db	0
