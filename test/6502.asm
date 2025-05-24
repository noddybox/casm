        option  output-file, "output/6502.bin"
	cpu	6502
        ; START
	adc #$44
	adc $44
	adc $44,x
	adc $4400
	adc $4400,x
	adc $4400,y
	adc ($44,X)
	adc ($44),Y
	and #$44
	and $44
	and $44,x
	and $4400
	and $4400,x
	and $4400,y
	and ($44,x)
	and ($44),y
	asl a
	asl $44
	asl $44,x
	asl $4400
	asl $4400,x
	bit $44
	bit $4400
	bpl 1
	bmi 1
	bvc 1
	bvs 1
	bcc 1
	bcs 1
	bne 1
	beq 1
    	brk
	cmp #$44
	cmp $44
	cmp $44,x
	cmp $4400
	cmp $4400,x
	cmp $4400,y
	cmp ($44,x)
	cmp ($44),y
	cpx #$44
	cpx $44
	cpx $4400
	cpy #$44
	cpy $44
	cpy $4400
	dec $44
	dec $44,x
	dec $4400
	dec $4400,x
	eor #$44
	eor $44
	eor $44,x
	eor $4400
	eor $4400,x
	eor $4400,y
	eor ($44,x)
	eor ($44),y
	clc
	sec
	cli
	sei
	clv
	cld
	sed
	inc $44
	inc $44,x
	inc $4400
	inc $4400,x
	jmp $5597
	jmp ($5597)
	jsr $5597
	lda #$44
	lda $44
	lda $44,x
	lda $4400
	lda $4400,x
	lda $4400,y
	lda ($44,x)
	lda ($44),y
	ldx #$44
	ldx $44
	ldx $44,y
	ldx $4400
	ldx $4400,y
	ldy #$44
	ldy $44
	ldy $44,x
	ldy $4400
	ldy $4400,x
	lsr a
	lsr $44
	lsr $44,x
	lsr $4400
	lsr $4400,x
	nop
	ora #$44
	ora $44
	ora $44,x
	ora $4400
	ora $4400,x
	ora $4400,y
	ora ($44,x)
	ora ($44),y
	tax
	txa
	dex
	inx
	tay
	tya
	dey
	iny
	rol a
	rol $44
	rol $44,x
	rol $4400
	rol $4400,x
	ror a
	ror $44
	ror $44,x
	ror $4400
	ror $4400,x
	rti
	RTS
	sbc #$44
	sbc $44
	sbc $44,x
	sbc $4400
	sbc $4400,x
	sbc $4400,y
	sbc ($44,x)
	sbc ($44),y
	sta $44
	sta $44,x
	sta $4400
	sta $4400,x
	sta $4400,y
	sta ($44,x)
	sta ($44),y
	stx $44
	stx $44,y
	stx $4400
	sty $44
	sty $44,x
	sty $4400
	txs
	tsx
	pha
	pla
	php
	plp
        jam
        alr     #$44
        anc     #$44
        anc2    #$44
        ane     #$44
        arr     #$44
        dcp     $44
        dcp     $44,x
        dcp     $4400
        dcp     $4400,x
        dcp     $4400,y
        dcp     ($44,x)
        dcp     ($44),y
        isc     $44
        isc     $44,x
        isc     $4400
        isc     $4400,x
        isc     $4400,y
        isc     ($44,x)
        isc     ($44),y
        las     $4400,y
        lax     $44
        lax     $44,y
        lax     $4400
        lax     $4400,y
        lax     ($44,x)
        lax     ($44),y
        lxa     #$44
        rla     $44
        rla     $44,x
        rla     $4400
        rla     $4400,x
        rla     $4400,y
        rla     ($44,x)
        rla     ($44),y
        rra     $44
        rra     $44,x
        rra     $4400
        rra     $4400,x
        rra     $4400,y
        rra     ($44,x)
        rra     ($44),y
        sax     $44
        sax     $44,y
        sax     $4400
        sax     ($44,x)
        sbx     #$44
        sha     $4400,y
        sha     ($44),y
        shx     $4400,y
        shy     $4400,x
        slo     $44
        slo     $44,x
        slo     $4400
        slo     $4400,x
        slo     $4400,y
        slo     ($44,x)
        slo     ($44),y
        sre     $44
        sre     $44,x
        sre     $4400
        sre     $4400,x
        sre     $4400,y
        sre     ($44,x)
        sre     ($44),y
        tas     $4400,y
        usbc    #$44
