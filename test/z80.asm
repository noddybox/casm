    option output-file,output/z80.bin
    cpu z80
    org 0
    ; START
    NOP
    LD BC,$1234
    LD (BC),A
    INC BC
    INC B
    DEC B
    LD B,$e5
    RLCA
    EX AF,AF'
    ADD HL,BC
    LD A,(BC)
    DEC BC
    INC C
    DEC C
    LD C,$e5
    RRCA
    DJNZ $0001
    LD DE,$1234
    LD (DE),A
    INC DE
    INC D
    DEC D
    LD D,$e5
    RLA
    JR $0001
    ADD HL,DE
    LD A,(DE)
    DEC DE
    INC E
    DEC E
    LD E,$e5
    RRA
    JR NZ,$0001
    LD HL,$1234
    LD ($1234),HL
    INC HL
    INC H
    DEC H
    LD H,$e5
    DAA
    JR Z,$0001
    ADD HL,HL
    LD HL,($1234)
    DEC HL
    INC L
    DEC L
    LD L,$e5
    CPL
    JR NC,$0001
    LD SP,$1234
    LD ($1234),A
    INC SP
    INC (HL)
    DEC (HL)
    LD (HL),$e5
    SCF
    JR C,$0001
    ADD HL,SP
    LD A,($1234)
    DEC SP
    INC A
    DEC A
    LD A,$e5
    CCF
    LD B,B
    LD B,C
    LD B,D
    LD B,E
    LD B,H
    LD B,L
    LD B,(HL)
    LD B,A
    LD C,B
    LD C,C
    LD C,D
    LD C,E
    LD C,H
    LD C,L
    LD C,(HL)
    LD C,A
    LD D,B
    LD D,C
    LD D,D
    LD D,E
    LD D,H
    LD D,L
    LD D,(HL)
    LD D,A
    LD E,B
    LD E,C
    LD E,D
    LD E,E
    LD E,H
    LD E,L
    LD E,(HL)
    LD E,A
    LD H,B
    LD H,C
    LD H,D
    LD H,E
    LD H,H
    LD H,L
    LD H,(HL)
    LD H,A
    LD L,B
    LD L,C
    LD L,D
    LD L,E
    LD L,H
    LD L,L
    LD L,(HL)
    LD L,A
    LD (HL),B
    LD (HL),C
    LD (HL),D
    LD (HL),E
    LD (HL),H
    LD (HL),L
    HALT
    LD (HL),A
    LD A,B
    LD A,C
    LD A,D
    LD A,E
    LD A,H
    LD A,L
    LD A,(HL)
    LD A,A
    ADD A,B
    ADD A,C
    ADD A,D
    ADD A,E
    ADD A,H
    ADD A,L
    ADD A,(HL)
    ADD A,A
    ADC A,B
    ADC A,C
    ADC A,D
    ADC A,E
    ADC A,H
    ADC A,L
    ADC A,(HL)
    ADC A,A
    SUB B
    SUB C
    SUB D
    SUB E
    SUB H
    SUB L
    SUB (HL)
    SUB A
    SBC A,B
    SBC A,C
    SBC A,D
    SBC A,E
    SBC A,H
    SBC A,L
    SBC A,(HL)
    SBC A,A
    AND B
    AND C
    AND D
    AND E
    AND H
    AND L
    AND (HL)
    AND A
    XOR B
    XOR C
    XOR D
    XOR E
    XOR H
    XOR L
    XOR (HL)
    XOR A
    OR B
    OR C
    OR D
    OR E
    OR H
    OR L
    OR (HL)
    OR A
    CP B
    CP C
    CP D
    CP E
    CP H
    CP L
    CP (HL)
    CP A
    RET NZ
    POP BC
    JP NZ,$1234
    JP $1234
    CALL NZ,$1234
    PUSH BC
    ADD A,$e5
    RST 0H
    RET Z
    RET
    JP Z,$1234
    RLC B
    RLC C
    RLC D
    RLC E
    RLC H
    RLC L
    RLC (HL)
    RLC A
    RRC B
    RRC C
    RRC D
    RRC E
    RRC H
    RRC L
    RRC (HL)
    RRC A
    RL B
    RL C
    RL D
    RL E
    RL H
    RL L
    RL (HL)
    RL A
    RR B
    RR C
    RR D
    RR E
    RR H
    RR L
    RR (HL)
    RR A
    SLA B
    SLA C
    SLA D
    SLA E
    SLA H
    SLA L
    SLA (HL)
    SLA A
    SRA B
    SRA C
    SRA D
    SRA E
    SRA H
    SRA L
    SRA (HL)
    SRA A
    SLL B
    SLL C
    SLL D
    SLL E
    SLL H
    SLL L
    SLL (HL)
    SLL A
    SRL B
    SRL C
    SRL D
    SRL E
    SRL H
    SRL L
    SRL (HL)
    SRL A
    BIT 0,B
    BIT 0,C
    BIT 0,D
    BIT 0,E
    BIT 0,H
    BIT 0,L
    BIT 0,(HL)
    BIT 0,A
    BIT 1,B
    BIT 1,C
    BIT 1,D
    BIT 1,E
    BIT 1,H
    BIT 1,L
    BIT 1,(HL)
    BIT 1,A
    BIT 2,B
    BIT 2,C
    BIT 2,D
    BIT 2,E
    BIT 2,H
    BIT 2,L
    BIT 2,(HL)
    BIT 2,A
    BIT 3,B
    BIT 3,C
    BIT 3,D
    BIT 3,E
    BIT 3,H
    BIT 3,L
    BIT 3,(HL)
    BIT 3,A
    BIT 4,B
    BIT 4,C
    BIT 4,D
    BIT 4,E
    BIT 4,H
    BIT 4,L
    BIT 4,(HL)
    BIT 4,A
    BIT 5,B
    BIT 5,C
    BIT 5,D
    BIT 5,E
    BIT 5,H
    BIT 5,L
    BIT 5,(HL)
    BIT 5,A
    BIT 6,B
    BIT 6,C
    BIT 6,D
    BIT 6,E
    BIT 6,H
    BIT 6,L
    BIT 6,(HL)
    BIT 6,A
    BIT 7,B
    BIT 7,C
    BIT 7,D
    BIT 7,E
    BIT 7,H
    BIT 7,L
    BIT 7,(HL)
    BIT 7,A
    RES 0,B
    RES 0,C
    RES 0,D
    RES 0,E
    RES 0,H
    RES 0,L
    RES 0,(HL)
    RES 0,A
    RES 1,B
    RES 1,C
    RES 1,D
    RES 1,E
    RES 1,H
    RES 1,L
    RES 1,(HL)
    RES 1,A
    RES 2,B
    RES 2,C
    RES 2,D
    RES 2,E
    RES 2,H
    RES 2,L
    RES 2,(HL)
    RES 2,A
    RES 3,B
    RES 3,C
    RES 3,D
    RES 3,E
    RES 3,H
    RES 3,L
    RES 3,(HL)
    RES 3,A
    RES 4,B
    RES 4,C
    RES 4,D
    RES 4,E
    RES 4,H
    RES 4,L
    RES 4,(HL)
    RES 4,A
    RES 5,B
    RES 5,C
    RES 5,D
    RES 5,E
    RES 5,H
    RES 5,L
    RES 5,(HL)
    RES 5,A
    RES 6,B
    RES 6,C
    RES 6,D
    RES 6,E
    RES 6,H
    RES 6,L
    RES 6,(HL)
    RES 6,A
    RES 7,B
    RES 7,C
    RES 7,D
    RES 7,E
    RES 7,H
    RES 7,L
    RES 7,(HL)
    RES 7,A
    SET 0,B
    SET 0,C
    SET 0,D
    SET 0,E
    SET 0,H
    SET 0,L
    SET 0,(HL)
    SET 0,A
    SET 1,B
    SET 1,C
    SET 1,D
    SET 1,E
    SET 1,H
    SET 1,L
    SET 1,(HL)
    SET 1,A
    SET 2,B
    SET 2,C
    SET 2,D
    SET 2,E
    SET 2,H
    SET 2,L
    SET 2,(HL)
    SET 2,A
    SET 3,B
    SET 3,C
    SET 3,D
    SET 3,E
    SET 3,H
    SET 3,L
    SET 3,(HL)
    SET 3,A
    SET 4,B
    SET 4,C
    SET 4,D
    SET 4,E
    SET 4,H
    SET 4,L
    SET 4,(HL)
    SET 4,A
    SET 5,B
    SET 5,C
    SET 5,D
    SET 5,E
    SET 5,H
    SET 5,L
    SET 5,(HL)
    SET 5,A
    SET 6,B
    SET 6,C
    SET 6,D
    SET 6,E
    SET 6,H
    SET 6,L
    SET 6,(HL)
    SET 6,A
    SET 7,B
    SET 7,C
    SET 7,D
    SET 7,E
    SET 7,H
    SET 7,L
    SET 7,(HL)
    SET 7,A
    CALL Z,$1234
    CALL $1234
    ADC A,$e5
    RST 8H
    RET NC
    POP DE
    JP NC,$1234
    OUT ($e5),A
    CALL NC,$1234
    PUSH DE
    SUB $e5
    RST $10
    RET C
    EXX
    JP C,$1234
    IN A,($e5)
    CALL C,$1234
    ADD IX,BC
    ADD IX,DE
    LD IX,$1234
    LD ($1234),IX
    INC IX
    INC IXH
    DEC IXH
    LD IXH,$e5
    ADD IX,IX
    LD IX,($1234)
    DEC IX
    INC IXL
    DEC IXL
    LD IXL,$e5
    INC (IX+1)
    DEC (IX+1)
    LD (IX+1),$e5
    ADD IX,SP
    LD B,IXH
    LD B,IXL
    LD B,(IX+1)
    LD C,IXH
    LD C,IXL
    LD C,(IX+1)
    LD D,IXH
    LD D,IXL
    LD D,(IX+1)
    LD E,IXH
    LD E,IXL
    LD E,(IX+1)
    LD IXH,B
    LD IXH,C
    LD IXH,D
    LD IXH,E
    LD IXH,IXH
    LD IXH,IXL
    LD H,(IX+1)
    LD IXH,A
    LD IXL,B
    LD IXL,C
    LD IXL,D
    LD IXL,E
    LD IXL,IXH
    LD IXL,IXL
    LD L,(IX+1)
    LD IXL,A
    LD (IX+1),B
    LD (IX+1),C
    LD (IX+1),D
    LD (IX+1),E
    LD (IX+1),H
    LD (IX+1),L
    LD (IX+1),A
    LD A,IXH
    LD A,IXL
    LD A,(IX+1)
    ADD A,IXH
    ADD A,IXL
    ADD A,(IX+1)
    ADC A,IXH
    ADC A,IXL
    ADC A,(IX+1)
    SUB IXH
    SUB IXL
    SUB (IX+1)
    SBC A,IXH
    SBC A,IXL
    SBC A,(IX+1)
    AND IXH
    AND IXL
    AND (IX+1)
    XOR IXH
    XOR IXL
    XOR (IX+1)
    OR IXH
    OR IXL
    OR (IX+1)
    CP IXH
    CP IXL
    CP (IX+1)
    RLC (IX+1),B
    RLC (IX+1),C
    RLC (IX+1),D
    RLC (IX+1),E
    RLC (IX+1),H
    RLC (IX+1),L
    RLC (IX+1)
    RLC (IX+1),A
    RRC (IX+1),B
    RRC (IX+1),C
    RRC (IX+1),D
    RRC (IX+1),E
    RRC (IX+1),H
    RRC (IX+1),L
    RRC (IX+1)
    RRC (IX+1),A
    RL (IX+1),B
    RL (IX+1),C
    RL (IX+1),D
    RL (IX+1),E
    RL (IX+1),H
    RL (IX+1),L
    RL (IX+1)
    RL (IX+1),A
    RR (IX+1),B
    RR (IX+1),C
    RR (IX+1),D
    RR (IX+1),E
    RR (IX+1),H
    RR (IX+1),L
    RR (IX+1)
    RR (IX+1),A
    SLA (IX+1),B
    SLA (IX+1),C
    SLA (IX+1),D
    SLA (IX+1),E
    SLA (IX+1),H
    SLA (IX+1),L
    SLA (IX+1)
    SLA (IX+1),A
    SRA (IX+1),B
    SRA (IX+1),C
    SRA (IX+1),D
    SRA (IX+1),E
    SRA (IX+1),H
    SRA (IX+1),L
    SRA (IX+1)
    SRA (IX+1),A
    SLL (IX+1),B
    SLL (IX+1),C
    SLL (IX+1),D
    SLL (IX+1),E
    SLL (IX+1),H
    SLL (IX+1),L
    SLL (IX+1)
    SLL (IX+1),A
    SRL (IX+1),B
    SRL (IX+1),C
    SRL (IX+1),D
    SRL (IX+1),E
    SRL (IX+1),H
    SRL (IX+1),L
    SRL (IX+1)
    SRL (IX+1),A
    BIT 0,(IX+1)
    BIT 1,(IX+1)
    BIT 2,(IX+1)
    BIT 3,(IX+1)
    BIT 4,(IX+1)
    BIT 5,(IX+1)
    BIT 6,(IX+1)
    BIT 7,(IX+1)
    RES 0,(IX+1),B
    RES 0,(IX+1),C
    RES 0,(IX+1),D
    RES 0,(IX+1),E
    RES 0,(IX+1),H
    RES 0,(IX+1),L
    RES 0,(IX+1)
    RES 0,(IX+1),A
    RES 1,(IX+1),B
    RES 1,(IX+1),C
    RES 1,(IX+1),D
    RES 1,(IX+1),E
    RES 1,(IX+1),H
    RES 1,(IX+1),L
    RES 1,(IX+1)
    RES 1,(IX+1),A
    RES 2,(IX+1),B
    RES 2,(IX+1),C
    RES 2,(IX+1),D
    RES 2,(IX+1),E
    RES 2,(IX+1),H
    RES 2,(IX+1),L
    RES 2,(IX+1)
    RES 2,(IX+1),A
    RES 3,(IX+1),B
    RES 3,(IX+1),C
    RES 3,(IX+1),D
    RES 3,(IX+1),E
    RES 3,(IX+1),H
    RES 3,(IX+1),L
    RES 3,(IX+1)
    RES 3,(IX+1),A
    RES 4,(IX+1),B
    RES 4,(IX+1),C
    RES 4,(IX+1),D
    RES 4,(IX+1),E
    RES 4,(IX+1),H
    RES 4,(IX+1),L
    RES 4,(IX+1)
    RES 4,(IX+1),A
    RES 5,(IX+1),B
    RES 5,(IX+1),C
    RES 5,(IX+1),D
    RES 5,(IX+1),E
    RES 5,(IX+1),H
    RES 5,(IX+1),L
    RES 5,(IX+1)
    RES 5,(IX+1),A
    RES 6,(IX+1),B
    RES 6,(IX+1),C
    RES 6,(IX+1),D
    RES 6,(IX+1),E
    RES 6,(IX+1),H
    RES 6,(IX+1),L
    RES 6,(IX+1)
    RES 6,(IX+1),A
    RES 7,(IX+1),B
    RES 7,(IX+1),C
    RES 7,(IX+1),D
    RES 7,(IX+1),E
    RES 7,(IX+1),H
    RES 7,(IX+1),L
    RES 7,(IX+1)
    RES 7,(IX+1),A
    SET 0,(IX+1),B
    SET 0,(IX+1),C
    SET 0,(IX+1),D
    SET 0,(IX+1),E
    SET 0,(IX+1),H
    SET 0,(IX+1),L
    SET 0,(IX+1)
    SET 0,(IX+1),A
    SET 1,(IX+1),B
    SET 1,(IX+1),C
    SET 1,(IX+1),D
    SET 1,(IX+1),E
    SET 1,(IX+1),H
    SET 1,(IX+1),L
    SET 1,(IX+1)
    SET 1,(IX+1),A
    SET 2,(IX+1),B
    SET 2,(IX+1),C
    SET 2,(IX+1),D
    SET 2,(IX+1),E
    SET 2,(IX+1),H
    SET 2,(IX+1),L
    SET 2,(IX+1)
    SET 2,(IX+1),A
    SET 3,(IX+1),B
    SET 3,(IX+1),C
    SET 3,(IX+1),D
    SET 3,(IX+1),E
    SET 3,(IX+1),H
    SET 3,(IX+1),L
    SET 3,(IX+1)
    SET 3,(IX+1),A
    SET 4,(IX+1),B
    SET 4,(IX+1),C
    SET 4,(IX+1),D
    SET 4,(IX+1),E
    SET 4,(IX+1),H
    SET 4,(IX+1),L
    SET 4,(IX+1)
    SET 4,(IX+1),A
    SET 5,(IX+1),B
    SET 5,(IX+1),C
    SET 5,(IX+1),D
    SET 5,(IX+1),E
    SET 5,(IX+1),H
    SET 5,(IX+1),L
    SET 5,(IX+1)
    SET 5,(IX+1),A
    SET 6,(IX+1),B
    SET 6,(IX+1),C
    SET 6,(IX+1),D
    SET 6,(IX+1),E
    SET 6,(IX+1),H
    SET 6,(IX+1),L
    SET 6,(IX+1)
    SET 6,(IX+1),A
    SET 7,(IX+1),B
    SET 7,(IX+1),C
    SET 7,(IX+1),D
    SET 7,(IX+1),E
    SET 7,(IX+1),H
    SET 7,(IX+1),L
    SET 7,(IX+1)
    SET 7,(IX+1),A
    POP IX
    EX (SP),IX
    PUSH IX
    JP (IX)
    LD SP,IX
    SBC A,$e5
    RST $18
    RET PO
    POP HL
    JP PO,$1234
    EX (SP),HL
    CALL PO,$1234
    PUSH HL
    AND $e5
    RST $20
    RET PE
    JP (HL)
    JP PE,$1234
    EX DE,HL
    CALL PE,$1234
    IN B,(C)
    OUT (C),B
    SBC HL,BC
    LD ($1234),BC
    NEG
    RETN
    IM 0
    LD I,A
    IN C,(C)
    OUT (C),C
    ADC HL,BC
    LD BC,($1234)
    NEG
    RETI
    IM 0/1
    LD R,A
    IN D,(C)
    OUT (C),D
    SBC HL,DE
    LD ($1234),DE
    NEG
    RETN
    IM 1
    LD A,I
    IN E,(C)
    OUT (C),E
    ADC HL,DE
    LD DE,($1234)
    NEG
    RETN
    IM 2
    LD A,R
    IN H,(C)
    OUT (C),H
    SBC HL,HL
    LD ($1234),HL
    NEG
    RETN
    IM 0
    RRD
    IN L,(C)
    OUT (C),L
    ADC HL,HL
    LD HL,($1234)
    NEG
    RETN
    IM 0/1
    RLD
    IN F,(C)
    OUT (C),0
    SBC HL,SP
    LD ($1234),SP
    NEG
    RETN
    IM 1
    IN A,(C)
    OUT (C),A
    ADC HL,SP
    LD SP,($1234)
    NEG
    RETN
    IM 2
    LDI
    CPI
    INI
    OUTI
    LDD
    CPD
    IND
    OUTD
    LDIR
    CPIR
    INIR
    OTIR
    LDDR
    CPDR
    INDR
    OTDR
    XOR $e5
    RST $28
    RET P
    POP AF
    JP P,$1234
    DI
    CALL P,$1234
    PUSH AF
    OR $e5
    RST $30
    RET M
    LD SP,HL
    JP M,$1234
    EI
    CALL M,$1234
    ADD IY,BC
    ADD IY,DE
    LD IY,$1234
    LD ($1234),IY
    INC IY
    INC IYH
    DEC IYH
    LD IYH,$e5
    ADD IY,IY
    LD IY,($1234)
    DEC IY
    INC IYL
    DEC IYL
    LD IYL,$e5
    INC (IY+1)
    DEC (IY+1)
    LD (IY+1),$e5
    ADD IY,SP
    LD B,IYH
    LD B,IYL
    LD B,(IY+1)
    LD C,IYH
    LD C,IYL
    LD C,(IY+1)
    LD D,IYH
    LD D,IYL
    LD D,(IY+1)
    LD E,IYH
    LD E,IYL
    LD E,(IY+1)
    LD IYH,B
    LD IYH,C
    LD IYH,D
    LD IYH,E
    LD IYH,IYH
    LD IYH,IYL
    LD H,(IY+1)
    LD IYH,A
    LD IYL,B
    LD IYL,C
    LD IYL,D
    LD IYL,E
    LD IYL,IYH
    LD IYL,IYL
    LD L,(IY+1)
    LD IYL,A
    LD (IY+1),B
    LD (IY+1),C
    LD (IY+1),D
    LD (IY+1),E
    LD (IY+1),H
    LD (IY+1),L
    LD (IY+1),A
    LD A,IYH
    LD A,IYL
    LD A,(IY+1)
    ADD A,IYH
    ADD A,IYL
    ADD A,(IY+1)
    ADC A,IYH
    ADC A,IYL
    ADC A,(IY+1)
    SUB IYH
    SUB IYL
    SUB (IY+1)
    SBC A,IYH
    SBC A,IYL
    SBC A,(IY+1)
    AND IYH
    AND IYL
    AND (IY+1)
    XOR IYH
    XOR IYL
    XOR (IY+1)
    OR IYH
    OR IYL
    OR (IY+1)
    CP IYH
    CP IYL
    CP (IY+1)
    RLC (IY+1),B
    RLC (IY+1),C
    RLC (IY+1),D
    RLC (IY+1),E
    RLC (IY+1),H
    RLC (IY+1),L
    RLC (IY+1)
    RLC (IY+1),A
    RRC (IY+1),B
    RRC (IY+1),C
    RRC (IY+1),D
    RRC (IY+1),E
    RRC (IY+1),H
    RRC (IY+1),L
    RRC (IY+1)
    RRC (IY+1),A
    RL (IY+1),B
    RL (IY+1),C
    RL (IY+1),D
    RL (IY+1),E
    RL (IY+1),H
    RL (IY+1),L
    RL (IY+1)
    RL (IY+1),A
    RR (IY+1),B
    RR (IY+1),C
    RR (IY+1),D
    RR (IY+1),E
    RR (IY+1),H
    RR (IY+1),L
    RR (IY+1)
    RR (IY+1),A
    SLA (IY+1),B
    SLA (IY+1),C
    SLA (IY+1),D
    SLA (IY+1),E
    SLA (IY+1),H
    SLA (IY+1),L
    SLA (IY+1)
    SLA (IY+1),A
    SRA (IY+1),B
    SRA (IY+1),C
    SRA (IY+1),D
    SRA (IY+1),E
    SRA (IY+1),H
    SRA (IY+1),L
    SRA (IY+1)
    SRA (IY+1),A
    SLL (IY+1),B
    SLL (IY+1),C
    SLL (IY+1),D
    SLL (IY+1),E
    SLL (IY+1),H
    SLL (IY+1),L
    SLL (IY+1)
    SLL (IY+1),A
    SRL (IY+1),B
    SRL (IY+1),C
    SRL (IY+1),D
    SRL (IY+1),E
    SRL (IY+1),H
    SRL (IY+1),L
    SRL (IY+1)
    SRL (IY+1),A
    BIT 0,(IY+1)
    BIT 1,(IY+1)
    BIT 2,(IY+1)
    BIT 3,(IY+1)
    BIT 4,(IY+1)
    BIT 5,(IY+1)
    BIT 6,(IY+1)
    BIT 7,(IY+1)
    LD B,RES 0,(IY+1)
    LD C,RES 0,(IY+1)
    LD D,RES 0,(IY+1)
    LD E,RES 0,(IY+1)
    LD H,RES 0,(IY+1)
    LD L,RES 0,(IY+1)
    RES 0,(IY+1)
    LD A,RES 0,(IY+1)
    LD B,RES 1,(IY+1)
    LD C,RES 1,(IY+1)
    LD D,RES 1,(IY+1)
    LD E,RES 1,(IY+1)
    LD H,RES 1,(IY+1)
    LD L,RES 1,(IY+1)
    RES 1,(IY+1)
    LD A,RES 1,(IY+1)
    LD B,RES 2,(IY+1)
    LD C,RES 2,(IY+1)
    LD D,RES 2,(IY+1)
    LD E,RES 2,(IY+1)
    LD H,RES 2,(IY+1)
    LD L,RES 2,(IY+1)
    RES 2,(IY+1)
    LD A,RES 2,(IY+1)
    LD B,RES 3,(IY+1)
    LD C,RES 3,(IY+1)
    LD D,RES 3,(IY+1)
    LD E,RES 3,(IY+1)
    LD H,RES 3,(IY+1)
    LD L,RES 3,(IY+1)
    RES 3,(IY+1)
    LD A,RES 3,(IY+1)
    LD B,RES 4,(IY+1)
    LD C,RES 4,(IY+1)
    LD D,RES 4,(IY+1)
    LD E,RES 4,(IY+1)
    LD H,RES 4,(IY+1)
    LD L,RES 4,(IY+1)
    RES 4,(IY+1)
    LD A,RES 4,(IY+1)
    LD B,RES 5,(IY+1)
    LD C,RES 5,(IY+1)
    LD D,RES 5,(IY+1)
    LD E,RES 5,(IY+1)
    LD H,RES 5,(IY+1)
    LD L,RES 5,(IY+1)
    RES 5,(IY+1)
    LD A,RES 5,(IY+1)
    LD B,RES 6,(IY+1)
    LD C,RES 6,(IY+1)
    LD D,RES 6,(IY+1)
    LD E,RES 6,(IY+1)
    LD H,RES 6,(IY+1)
    LD L,RES 6,(IY+1)
    RES 6,(IY+1)
    LD A,RES 6,(IY+1)
    LD B,RES 7,(IY+1)
    LD C,RES 7,(IY+1)
    LD D,RES 7,(IY+1)
    LD E,RES 7,(IY+1)
    LD H,RES 7,(IY+1)
    LD L,RES 7,(IY+1)
    RES 7,(IY+1)
    LD A,RES 7,(IY+1)
    LD B,SET 0,(IY+1)
    LD C,SET 0,(IY+1)
    LD D,SET 0,(IY+1)
    LD E,SET 0,(IY+1)
    LD H,SET 0,(IY+1)
    LD L,SET 0,(IY+1)
    SET 0,(IY+1)
    LD A,SET 0,(IY+1)
    LD B,SET 1,(IY+1)
    LD C,SET 1,(IY+1)
    LD D,SET 1,(IY+1)
    LD E,SET 1,(IY+1)
    LD H,SET 1,(IY+1)
    LD L,SET 1,(IY+1)
    SET 1,(IY+1)
    LD A,SET 1,(IY+1)
    LD B,SET 2,(IY+1)
    LD C,SET 2,(IY+1)
    LD D,SET 2,(IY+1)
    LD E,SET 2,(IY+1)
    LD H,SET 2,(IY+1)
    LD L,SET 2,(IY+1)
    SET 2,(IY+1)
    LD A,SET 2,(IY+1)
    LD B,SET 3,(IY+1)
    LD C,SET 3,(IY+1)
    LD D,SET 3,(IY+1)
    LD E,SET 3,(IY+1)
    LD H,SET 3,(IY+1)
    LD L,SET 3,(IY+1)
    SET 3,(IY+1)
    LD A,SET 3,(IY+1)
    LD B,SET 4,(IY+1)
    LD C,SET 4,(IY+1)
    LD D,SET 4,(IY+1)
    LD E,SET 4,(IY+1)
    LD H,SET 4,(IY+1)
    LD L,SET 4,(IY+1)
    SET 4,(IY+1)
    LD A,SET 4,(IY+1)
    LD B,SET 5,(IY+1)
    LD C,SET 5,(IY+1)
    LD D,SET 5,(IY+1)
    LD E,SET 5,(IY+1)
    LD H,SET 5,(IY+1)
    LD L,SET 5,(IY+1)
    SET 5,(IY+1)
    LD A,SET 5,(IY+1)
    LD B,SET 6,(IY+1)
    LD C,SET 6,(IY+1)
    LD D,SET 6,(IY+1)
    LD E,SET 6,(IY+1)
    LD H,SET 6,(IY+1)
    LD L,SET 6,(IY+1)
    SET 6,(IY+1)
    LD A,SET 6,(IY+1)
    LD B,SET 7,(IY+1)
    LD C,SET 7,(IY+1)
    LD D,SET 7,(IY+1)
    LD E,SET 7,(IY+1)
    LD H,SET 7,(IY+1)
    LD L,SET 7,(IY+1)
    SET 7,(IY+1)
    LD A,SET 7,(IY+1)
    POP IY
    EX (SP),IY
    PUSH IY
    JP (IY)
    LD SP,IY
    CP $e5
    RST $38
