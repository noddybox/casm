	; Simple example spectrum code
	;
	; To use:
	;
	; LOAD "" CODE
	; RANDOMIZE USR 32768
	;

	option	output-file,spectrum.tap
	option	output-format,spectrum
	option	+spectrum-loader
	option	spectrum-start,start

start:	org 32768

	xor	a
	ld	bc,$fe00
loop:
	out	(c), a
	inc	a
	jr	loop
