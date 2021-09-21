	; Simple example C64 code
	;

	cpu 6502

	option	codepage,cbm

	option	output-file,t64.t64
	option	output-format,t64
	option	t64-start,start

	org $820

main:
	lda	#0
	clc
loop:
	sta	53280
	adc	#1
	and	#$0f
	jmp	loop

start:
	jmp	main
