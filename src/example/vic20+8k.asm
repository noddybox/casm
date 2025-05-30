	; Simple example C64 code
	;

	cpu 6502

	option	codepage,cbm

	option	output-file,vic20+8k.prg
	option	output-format,prg
	option	prg-start,start
        option  prg-system,vic20+8k

	org $1300

main:
	lda	#0
	clc
loop:
	sta	36879
	adc	#1
        pha
        jsr     $ffe4
        beq     nokey
        rts
nokey:
        pla
	jmp	loop

start:
	jmp	main
