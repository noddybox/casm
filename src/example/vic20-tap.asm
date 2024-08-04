	; Simple example C64 code
	;

	cpu 6502

	option	codepage,cbm

	option	output-file,vic20.tap
	option	output-format,cbm-tap
	option	cbm-tap-start,start
        option  cbm-tap-system,vic20

	org $1100

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
