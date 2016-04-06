	; Simple example ZX81 code
	;

	option	output-file,zx81.p
	option	output-format,zx81

	option	zx81-margin,pal
	option	zx81-autorun,on
	option	zx81-collapse-dfile,off

	option	codepage,zx81

DFILE:	equ	16396

	org 16514

	ld	hl,(DFILE)
	inc	hl
	ld	de,hello
loop:
	ld	a,(de)
	cp	255
	ret	z
	ld	(hl),a
	inc	hl
	inc	de

	ld	a,(hl)
	cp	$76
	jr	nz,loop
	inc	hl
	jr	loop

hello:	db	"HELLO|WORLD hello world (#69.99 well spent)",255
