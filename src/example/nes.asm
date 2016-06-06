	processor 6502

	option  output-file,"nes.nes"
	option  output-format,nes

	option  nes-vector,reset,start
	option  nes-vector,nmi,nmi
	option  nes-vector,brk,nmi

vsync:	macro
.wait
	lda	$2002
	bpl	wait
	endm

start:	org	$c000

	sei
	cld
	ldx	#$ff
	txs

	; this sets up the PPU.  Apparently.
	lda	#%00001000     
	sta	$2000          
	lda	#%00011110 
	sta	$2001

	; Set PPU to palette
	;
loadpalette:
	lda	#$3f
	sta	$2006
	lda	#$00
	sta	$2006

	; Load the palette
	;
	ldx	#0
.loop	lda	palette,x
	sta	$2007
	inx
	cpx	#$20
	bne	loop

	; Wait for a vysnc before loading the name table
	;
	vsync

	; Load the name map
	;
load_namemap:
	lda	#$20
	sta	$2006
	lda	#$20
	sta	$2006

	ldx	#0
.loop	lda	map,x
	cmp	#$ff
	beq	done
	sta	$2007
	inx
	jmp	loop
.done


forever:
	jmp	forever

nmi:
	rti

palette:
	incbin	"nes.pal"

map:
	db	'H'-'@'
	db	'E'-'@'
	db	'L'-'@'
	db	'L'-'@'
	db	'O'-'@'
	db	0
	db	'W'-'@'
	db	'O'-'@'
	db	'R'-'@'
	db	'L'-'@'
	db	'D'-'@'
	db	255


;
; VROM
;
	org 0
	bank 1

	incbin	"tiles.chr"
