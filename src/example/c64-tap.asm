	; Simple example C64 code
	;

	cpu 6502

	option	codepage,cbm

	option	output-file,c64.tap
	option	output-format,cbm-tap
	option	cbm-tap-start,start

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
