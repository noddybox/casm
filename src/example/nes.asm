	processor 6502

	option  output-file,"nes.nes"
	option  output-format,nes

	option  nes-vector,reset,start
	option  nes-vector,nmi,nmi
	option  nes-vector,brk,nmi
	option	nes-mirror,horizontal

vsync:	macro
.wait
	lda	$2002
	bpl	wait
	endm

start:	org	$c000

	; Clear decimal and setup stack
	;
	cld
	ldx	#$ff
	txs

	; Wait for the PPU.  Recommended practice is clear the flag, and
	; wait for 2 VBL signals
	;
	bit	$2002
	vsync
	vsync

	; Setup PPU
	;
	lda	#%00001000     
	sta	$2000          
	lda	#%00011110 
	sta	$2001

	; Setup the palette
	;
loadpalette:
	lda	#$3f
	sta	$2006
	lda	#$00
	sta	$2006

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
	lda	#$40
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
