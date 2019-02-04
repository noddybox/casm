	; Simple example CPC code
	;
	; To use:
	;
	; RUN ""

	option	output-file,cpc.cdt
	option	output-format,cpc
	option	cpc-start,start

start:	org	$8000

	ld	hl,msg
loop:
	ld	a,(hl)
	ret	z
	call	$bb5a
	inc	hl
	jr	loop

	org	$8800
msg:	defb	"Hello World",0
