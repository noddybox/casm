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
loop:
	out	($fe), a

	ld	hl,0x5800
	ld	bc,32*24

loop1:
	ld	(hl),a
	inc	hl
	dec	c
	jr	nz,loop1
	djnz	loop1

	inc	a
	jr	loop
