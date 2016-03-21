	; Simple example C64 code
	;

	cpu 6502

	option	codepage,cbm

	option	output-file,c64.t64
	option	output-format,t64

	org $6000

	lda	#0
	clc
loop:
	sta	53280
	adc	#1
	and	#$0f
	jmp	loop
