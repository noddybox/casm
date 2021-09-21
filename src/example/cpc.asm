	; Simple example CPC code
	;
	; To use:
	;
	; RUN ""

	option	output-file,cpc.cdt
	option	output-format,cpc
	option	cpc-start,start
	option	cpc-loader,true

start:	org	$8000

	ld	hl,msg
	call	print
	ret

	org	$8400
print:
	ld	a,(hl)
	cp	0
	ret	z
	push	hl
	call	$bb5a
	pop	hl
	inc	hl
	jr	print

	org	$8800
msg:	defb	"Hello World",0
