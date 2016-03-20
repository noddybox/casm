	; Simple example spectrum code
	;
	; To use:
	;
	; LOAD "" CODE
	; RANDOMIZE USR 32768
	;

	option	output-file,spectrum.tap
	option	output-format,spectrum

	org 32768

	ld	a,0
loop:
	out	($fe), a
	inc	a
	and	7
	jp	loop
