/*

    casm - Simple, portable assembler

    Copyright (C) 2003-2015  Ian Cowburn (ianc@noddybox.co.uk)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    -------------------------------------------------------------------------

    GBCPU Assembler

*/
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "global.h"
#include "expr.h"
#include "label.h"
#include "parse.h"
#include "cmd.h"
#include "codepage.h"
#include "varchar.h"

#include "gbcpu.h"


/* ---------------------------------------- TYPES AND GLOBALS
*/
typedef enum
{
    A8,
    B8,
    C8,
    D8,
    E8,
    H8,
    L8,
    F8,
    AF16,
    BC16,
    DE16,
    HL16,
    SP16,
    BC_ADDRESS,
    DE_ADDRESS,
    HL_ADDRESS,
    SP_ADDRESS,
    HL_INCREMENT,
    HL_DECREMENT,
    FF00_C_INDEX,
    ADDRESS,
    VALUE,
    INVALID_REG
} RegisterMode;


typedef enum
{
    NZ_FLAG,
    Z_FLAG,
    NC_FLAG,
    C_FLAG,
} ProcessorFlag;


typedef enum
{
    IS_NORMAL_8_BIT     =       0x01,
    IS_SPECIAL_8_BIT    =       0x02,
    IS_16_BIT           =       0x04,
    IS_MEMORY           =       0x08,
    IS_FF00_C           =       0x10,
    IS_SP               =       0x20,
    IS_VALUE            =       0x40,
    IS_SPECIAL_16_BIT   =       0x80,
} RegisterType;

#define IsNormal8Bit(t)         ((t) & IS_NORMAL_8_BIT)
#define IsSpecial8Bit(t)        ((t) & IS_SPECIAL_8_BIT)
#define Is16Bit(t)              ((t) & IS_16_BIT)
#define IsMemory(t)             ((t) & IS_MEMORY)
#define IsSP(t)                 ((t) & IS_SP)
#define IsAddress(t)            (((t) & IS_VALUE) && ((t) & IS_MEMORY))
#define IsValue(t)              ((t) & IS_VALUE)
#define IsSimpleValue(t)        ((t) == IS_VALUE)

#define CheckRange(arg,num,min,max)                     \
do                                                      \
{                                                       \
    if (IsFinalPass() && (num < min || num > max))      \
    {                                                   \
        snprintf(err, errsize, "%s: outside valid "     \
                    "range of %d - %d", arg, min, max); \
        return CMD_FAILED;                              \
    }                                                   \
} while(0)

#define CheckOffset(a,o)        CheckRange(a,0,-128,127)


static const int flag_bitmask[] =
{
    0x00,               /* NZ_FLAG */
    0x01,               /* Z_FLAG  */
    0x02,               /* NC_FLAG */
    0x03                /* C_FLAG  */
};


static const char *flag_text[] =
{
    "NZ",               /* NZ_FLAG */
    "Z",                /* Z_FLAG  */
    "NC",               /* NC_FLAG */
    "C",                /* C_FLAG  */
    NULL
};


static const char *register_mode_name[] =
{
    "A",
    "B",
    "C",
    "D",
    "E",
    "H",
    "L",
    "F",
    "AF",
    "BC",
    "DE",
    "HL",
    "SP",
    "(BC)",
    "(DE)",
    "(HL)",
    "(SP)",
    "(HL+)",
    "(HL-)",
    "(C)",
    "(address)",
    "value",
    "INVALID"
};


static const int register_bitmask[] =
{
    0x7,        /* A8                  */
    0x0,        /* B8                  */
    0x1,        /* C8                  */
    0x2,        /* D8                  */
    0x3,        /* E8                  */
    0x4,        /* H8                  */
    0x5,        /* L8                  */
    0x6,        /* F8                  */
    0x3,        /* AF16                */
    0x0,        /* BC16                */
    0x1,        /* DE16                */
    0x2,        /* HL16                */
    0x3,        /* SP16                */
    0x0,        /* BC_ADDRESS          */
    0x0,        /* DE_ADDRESS          */
    0x0,        /* HL_ADDRESS          */
    0x0,        /* SP_ADDRESS          */
    0x0,        /* HL_INCREMENT        */
    0x0,        /* HL_DECREMENT        */
    0x0,        /* FF00_C_INDEX        */
    0x0,        /* ADDRESS             */
    0x0,        /* VALUE               */
};


typedef struct
{
    RegisterMode        mode;
    int                 quote;
    int                 starts_with;
    int                 take_value;
    const char          *ident;
    RegisterType        type;
} RegisterModeTable;


static RegisterModeTable register_mode_table[] =
{
    {
        A8,
        0,
        FALSE,
        FALSE,
        "A",
        IS_NORMAL_8_BIT
    },
    {
        B8,
        0,
        FALSE,
        FALSE,
        "B",
        IS_NORMAL_8_BIT
    },
    {
        C8,
        0,
        FALSE,
        FALSE,
        "C",
        IS_NORMAL_8_BIT
    },
    {
        D8,
        0,
        FALSE,
        FALSE,
        "D",
        IS_NORMAL_8_BIT
    },
    {
        E8,
        0,
        FALSE,
        FALSE,
        "E",
        IS_NORMAL_8_BIT
    },
    {
        H8,
        0,
        FALSE,
        FALSE,
        "H",
        IS_NORMAL_8_BIT
    },
    {
        L8,
        0,
        FALSE,
        FALSE,
        "L",
        IS_NORMAL_8_BIT
    },
    {
        F8,
        0,
        FALSE,
        FALSE,
        "F",
        IS_SPECIAL_8_BIT
    },
        
    {
        AF16,
        0,
        FALSE,
        FALSE,
        "AF",
        IS_SPECIAL_16_BIT
    },
    {
        BC16,
        0,
        FALSE,
        FALSE,
        "BC",
        IS_16_BIT
    },
    {
        DE16,
        0,
        FALSE,
        FALSE,
        "DE",
        IS_16_BIT
    },
    {
        HL16,
        0,
        FALSE,
        FALSE,
        "HL",
        IS_16_BIT
    },
    {
        SP16,
        0,
        FALSE,
        FALSE,
        "SP",
        IS_16_BIT|IS_SP
    },

    {
        BC_ADDRESS,
        '(',
        FALSE,
        FALSE,
        "BC",
        IS_16_BIT|IS_MEMORY
    },
    {
        DE_ADDRESS,
        '(',
        FALSE,
        FALSE,
        "DE",
        IS_16_BIT|IS_MEMORY
    },
    {
        HL_ADDRESS,
        '(',
        FALSE,
        FALSE,
        "HL",
        IS_16_BIT|IS_MEMORY
    },
    {
        SP_ADDRESS,
        '(',
        FALSE,
        FALSE,
        "SP",
        IS_SPECIAL_16_BIT|IS_MEMORY|IS_SP
    },
    {
        HL_INCREMENT,
        '(',
        FALSE,
        FALSE,
        "HLI",
        IS_SPECIAL_16_BIT|IS_MEMORY
    },
    {
        HL_INCREMENT,
        '(',
        FALSE,
        FALSE,
        "HL+",
        IS_SPECIAL_16_BIT|IS_MEMORY
    },
    {
        HL_DECREMENT,
        '(',
        FALSE,
        FALSE,
        "HLD",
        IS_SPECIAL_16_BIT|IS_MEMORY
    },
    {
        HL_DECREMENT,
        '(',
        FALSE,
        FALSE,
        "HL-",
        IS_SPECIAL_16_BIT|IS_MEMORY
    },
    {
        FF00_C_INDEX,
        '(',
        FALSE,
        FALSE,
        "C",
        IS_FF00_C
    },

    /* These cheat -- basically anything that doesn't match until here is either
       an address or a value.
    */
    {
        ADDRESS,
        '(',
        TRUE,
        TRUE,
        "",
        IS_VALUE|IS_MEMORY
    },
    {
        VALUE,
        0,
        TRUE,
        TRUE,
        "",
        IS_VALUE
    },

    {0}
};


typedef enum 
{
    WRITE_BYTE_LHS      = -1,
    WRITE_WORD_LHS      = -2,
    WRITE_BYTE_RHS      = -3,
    WRITE_WORD_RHS      = -4
} StreamCodes;


typedef struct
{
    RegisterMode        lhs;
    RegisterMode        rhs;
    int                 code[10];
} RegisterPairCodes;

#define NUM_REGISTER_CODES(a)   ((sizeof a)/(sizeof a[1]))

/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static int CalcRegisterMode(const char *arg, int quote,
                            RegisterMode *mode,
                            RegisterType *type,
                            long *offset,
                            char *err, size_t errsize)
{
    int f;

    if (IsNullOrEmpty(arg))
    {
        snprintf(err, errsize, "empty argument supplied");
        return FALSE;
    }

    for(f = 0; register_mode_table[f].ident; f++)
    {
        RegisterModeTable *t = register_mode_table + f;

        if (quote == t->quote)
        {
            int match;

            if (t->starts_with)
            {
                match = CompareStart(arg, t->ident);
            }
            else
            {
                match = CompareString(arg, t->ident);
            }

            if (match)
            {
                *mode = t->mode;
                *type = t->type;
                *offset = 0;

                if (t->take_value)
                {
                    size_t l = strlen(t->ident);

                    if (!ExprEval(arg + l, offset))
                    {
                        snprintf(err, errsize, "%s: expression error: %s",
                                                        arg, ExprError());
                        return FALSE;
                    }
                }

                return TRUE;
            }
        }
    }

    snprintf(err, errsize, "%s: couldn't calculate register/addressing mode",
                                        arg);

    return FALSE;
}


static int CalcFlagMode(const char *arg, ProcessorFlag *flag, int *mask,
                        char *err, size_t errsize)
{
    int f;

    for(f = 0; flag_text[f]; f++)
    {
        if (CompareString(arg, flag_text[f]))
        {
            *flag = f;
            *mask = flag_bitmask[f];
            return TRUE;
        }
    }

    snprintf(err, errsize, "%s: unknown flag", arg);

    return FALSE;
}


static int WriteRegisterPairModes(const RegisterPairCodes *codes, size_t count,
                                  RegisterMode lhs, RegisterMode rhs,
                                  int val_lhs, int val_rhs,
                                  char *err, size_t errsize)
{
    size_t f;

    for(f = 0; f < count; f++)
    {
        if (codes[f].lhs == lhs && codes[f].rhs == rhs)
        {
            int r;

            for(r = 0; codes[f].code[r]; r++)
            {
                switch(codes[f].code[r])
                {
                    case WRITE_BYTE_LHS:
                        PCWrite(val_lhs);
                        break;

                    case WRITE_WORD_LHS:
                        PCWriteWord(val_lhs);
                        break;

                    case WRITE_BYTE_RHS:
                        PCWrite(val_rhs);
                        break;

                    case WRITE_WORD_RHS:
                        PCWriteWord(val_rhs);
                        break;

                    default:
                        PCWrite(codes[f].code[r]);
                        break;
                }
            }
            return TRUE;
        }
    }

    snprintf(err, errsize, "LD: no code generation for register pair %s,%s",
                             register_mode_name[lhs], register_mode_name[rhs]);

    return FALSE;
}


/* Assume accumulator if only one argument
*/
static CommandStatus AssumeAccumulator(int argc, char *argv[], int quoted[],
                                       char *err, size_t errsize,
                                       RegisterMode *r1, RegisterMode *r2,
                                       RegisterType *t1, RegisterType *t2,
                                       long *off1, long *off2)
{
    CMD_ARGC_CHECK(2);

    if (argc == 2)
    {
        CalcRegisterMode("A", 0, r1, t1, off1, err, errsize);

        if (!CalcRegisterMode(argv[1], quoted[1], r2, t2, off2,
                              err, errsize))
        {
            return CMD_FAILED;
        }
    }
    else
    {
        if (!CalcRegisterMode(argv[1], quoted[1], r1, t1, off1, err, errsize))
        {
            return CMD_FAILED;
        }

        if (!CalcRegisterMode(argv[2], quoted[2], r2, t2, off2, err, errsize))
        {
            return CMD_FAILED;
        }
    }

    return CMD_OK;
}


/* Returns true if the passed register is any of the passed ones.  The list of
   registers is terminated with INVALID_REG
*/
static int IsAnyOf(RegisterMode reg, ...)
{
    va_list ap;
    RegisterMode m;

    va_start(ap, reg);

    m = va_arg(ap, RegisterMode);

    while(m != INVALID_REG)
    {
        if (reg == m)
        {
            return TRUE;
        }

        m = va_arg(ap, RegisterMode);
    }

    return FALSE;
}


static CommandStatus IllegalArgs(int argc, char *argv[], int quoted[],
                                 char *err, size_t errsize)
{
    Varchar *str;
    int f;

    str = VarcharCreate(NULL);

    switch(argc)
    {
        case 0:
            VarcharPrintf(str, "no command/arguments");
            break;

        case 1:
            VarcharPrintf(str, "%s: no arguments", argv[0]);
            break;

        case 2:
            VarcharPrintf(str, "%s: illegal argument", argv[0]);
            break;

        default:
            VarcharPrintf(str, "%s: illegal arguments", argv[0]);
            break;
    }

    for(f = 1; f < argc; f++)
    {
        if (f == 1)
        {
            VarcharAdd(str, " ");
        }
        else
        {
            VarcharAdd(str, ", ");
        }

        if (quoted[f] && quoted[f] == '(')
        {
            VarcharPrintf(str, "(%s)", argv[f]);
        }
        else if (quoted[f])
        {
            VarcharPrintf(str, "%c%s%c", quoted[f], argv[f], quoted[f]);
        }
        else
        {
            VarcharAdd(str, argv[f]);
        }
    }

    snprintf(err, errsize, "%s", VarcharContents(str));
    VarcharFree(str);

    return CMD_FAILED;
}


/* ---------------------------------------- COMMAND HANDLERS
*/
static CommandStatus LD(const char *label, int argc, char *argv[],       
                        int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             BC_ADDRESS,     {0x0a},
        A8,             DE_ADDRESS,     {0x1a},
        A8,             ADDRESS,        {0xfa, WRITE_WORD_RHS},
        BC_ADDRESS,     A8,             {0x02},
        DE_ADDRESS,     A8,             {0x12},
        ADDRESS,        A8,             {0xea, WRITE_WORD_LHS},
        HL_DECREMENT,   A8,             {0x32},
        A8,             HL_DECREMENT,   {0x3a},
        HL_INCREMENT,   A8,             {0x22},
        A8,             HL_INCREMENT,   {0x2a},
        FF00_C_INDEX,   A8,             {0xe2}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;

    CMD_ARGC_CHECK(3);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    if (!CalcRegisterMode(argv[2], quoted[2], &r2, &t2, &off2, err, errsize))
    {
        return CMD_FAILED;
    }

    /* LD r,r'
    */
    if (IsNormal8Bit(t1) && IsNormal8Bit(t2))
    {
        CommandStatus s = CMD_OK;

        PCWrite(0x40 | register_bitmask[r1] << 3 | register_bitmask[r2]);

        return s;
    }


    /* LD r,n
    */
    if (IsNormal8Bit(t1) && IsSimpleValue(t2))
    {
        PCWrite(register_bitmask[r1] << 3 | 0x6);
        PCWrite(off2);
        return CMD_OK;
    }


    /* LD r,(HL)
    */
    if (IsNormal8Bit(t1) && r2 == HL_ADDRESS)
    {
        PCWrite(0x46 | register_bitmask[r1] << 3);
        return CMD_OK;
    }


    /* LD (HL),r
    */
    if (IsNormal8Bit(t2) && r1 == HL_ADDRESS)
    {
        PCWrite(0x70 | register_bitmask[r2]);
        return CMD_OK;
    }


    /* LD (HL),n
    */
    if (r1 == HL_ADDRESS && r2 == VALUE)
    {
        PCWrite(0x36);
        PCWrite(off2);
        return CMD_OK;
    }


    /* LD rr,nn
    */
    if (Is16Bit(t1) && !IsMemory(t1) && r2 == VALUE)
    {
        PCWrite(register_bitmask[r1] << 4 | 0x01);
        PCWriteWord(off2);
        return CMD_OK;
    }


    /* LD HL,(nn)
    */
    if (r1 == HL16 && r2 == ADDRESS)
    {
        PCWrite(0x2a);
        PCWriteWord(off2);
        return CMD_OK;
    }


    /* LD rr,(nn)
    */
    if (Is16Bit(t1) && !IsMemory(t1) && r2 == ADDRESS)
    {
        PCWrite(0xed);
        PCWrite(0x4b | register_bitmask[r1] << 4);
        PCWriteWord(off2);
        return CMD_OK;
    }


    /* LD (nn),HL
    */
    if (r2 == HL16 && r1 == ADDRESS)
    {
        PCWrite(0x22);
        PCWriteWord(off1);
        return CMD_OK;
    }


    /* LD (nn),rr
    */
    if (Is16Bit(t2) && !IsMemory(t2) && r1 == ADDRESS)
    {
        PCWrite(0xed);
        PCWrite(0x43 | register_bitmask[r2] << 4);
        PCWriteWord(off1);
        return CMD_OK;
    }


    /* LD SP,HL
    */
    if (r2 == HL16 && r1 == SP16)
    {
        PCWrite(0xf9);
        return CMD_OK;
    }

    /* LD ($ff00 + n), A
    */
    if (r1 == A8 && r2 == ADDRESS && off2 >= 0xff00)
    {
        PCWrite(0xf0);
        PCWrite(off2 - 0xff00);
        return CMD_OK;
    }

    /* LD ($ff00 + n), A
    */
    if (r1 == ADDRESS && r2 == A8 && off1 >= 0xff00)
    {
        PCWrite(0xe0);
        PCWrite(off1 - 0xff00);
        return CMD_OK;
    }

    /* Custom opcode generation using the codes table
    */
    if (!WriteRegisterPairModes(codes, NUM_REGISTER_CODES(codes),
                                r1, r2, off1, off2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus LDH(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             ADDRESS,        {0xf0, WRITE_BYTE_RHS},
        ADDRESS,        A8,             {0xe0, WRITE_BYTE_LHS}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;

    CMD_ARGC_CHECK(3);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    if (!CalcRegisterMode(argv[2], quoted[2], &r2, &t2, &off2, err, errsize))
    {
        return CMD_FAILED;
    }

    if (r1 == ADDRESS)
    {
        if (off1 >= 0xff00 && off1 <= 0xffff)
        {
            off1 -= 0xff00;
        }

        CheckRange(argv[1], off1, 0, 255); 
    }

    if (r2 == ADDRESS)
    {
        if (off2 >= 0xff00 && off2 <= 0xffff)
        {
            off2 -= 0xff00;
        }

        CheckRange(argv[2], off2, 0, 255); 
    }

    if (!WriteRegisterPairModes(codes, NUM_REGISTER_CODES(codes),
                                r1, r2, off1, off2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus LDD(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        HL_ADDRESS,     A8,             {0x32},
        A8,             HL_ADDRESS,     {0x3a}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;

    CMD_ARGC_CHECK(3);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    if (!CalcRegisterMode(argv[2], quoted[2], &r2, &t2, &off2, err, errsize))
    {
        return CMD_FAILED;
    }

    if (!WriteRegisterPairModes(codes, NUM_REGISTER_CODES(codes),
                                r1, r2, off1, off2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus LDI(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        HL_ADDRESS,     A8,             {0x22},
        A8,             HL_ADDRESS,     {0x2a}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;

    CMD_ARGC_CHECK(3);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    if (!CalcRegisterMode(argv[2], quoted[2], &r2, &t2, &off2, err, errsize))
    {
        return CMD_FAILED;
    }

    if (!WriteRegisterPairModes(codes, NUM_REGISTER_CODES(codes),
                                r1, r2, off1, off2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus PUSH(const char *label, int argc, char *argv[],       
                          int quoted[], char *err, size_t errsize)
{
    RegisterMode r1;
    RegisterType t1;
    long off1;

    CMD_ARGC_CHECK(2);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    /* PUSH rr
    */
    if (r1 == AF16 || r1 == BC16 || r1 == DE16 || r1 == HL16)
    {
        PCWrite(0xc5 | register_bitmask[r1] << 4);
        return CMD_OK;
    }

    snprintf(err, errsize, "%s: invalid argument %s", argv[0], argv[1]);

    return CMD_OK;
}


static CommandStatus POP(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    RegisterMode r1;
    RegisterType t1;
    long off1;

    CMD_ARGC_CHECK(2);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    /* POP rr
    */
    if (r1 == AF16 || r1 == BC16 || r1 == DE16 || r1 == HL16)
    {
        PCWrite(0xc1 | register_bitmask[r1] << 4);
        return CMD_OK;
    }

    snprintf(err, errsize, "%s: invalid argument %s", argv[0], argv[1]);

    return CMD_OK;
}


static CommandStatus ADD(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xc6, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0x86},
        SP16,           VALUE,          {0xe8, WRITE_WORD_RHS}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* ADD A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        PCWrite(0x80 | register_bitmask[r2]);
        return CMD_OK;
    }

    /* ADD HL,rr
    */
    if (r1 == HL16 && IsAnyOf(r2, BC16, DE16, HL16, SP16, INVALID_REG))
    {
        PCWrite(0x09 | register_bitmask[r2] << 4);
        return CMD_OK;
    }

    if (!WriteRegisterPairModes(codes, NUM_REGISTER_CODES(codes),
                                r1, r2, off1, off2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus ADC(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xce, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0x8e}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* ADC A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        PCWrite(0x88 | register_bitmask[r2]);
        return CMD_OK;
    }

    /* ADC HL,rr
    */
    if (r1 == HL16 && IsAnyOf(r2, BC16, DE16, HL16, SP16, INVALID_REG))
    {
        PCWrite(0xed);
        PCWrite(0x4a | register_bitmask[r2] << 4);
        return CMD_OK;
    }

    if (!WriteRegisterPairModes(codes, NUM_REGISTER_CODES(codes),
                                r1, r2, off1, off2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus SUB(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xd6, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0x96}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* SUB A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        PCWrite(0x90 | register_bitmask[r2]);
        return CMD_OK;
    }

    if (!WriteRegisterPairModes(codes, NUM_REGISTER_CODES(codes),
                                r1, r2, off1, off2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus SBC(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xde, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0x9e}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* SBC A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        PCWrite(0x98 | register_bitmask[r2]);
        return CMD_OK;
    }

    /* SBC HL,rr
    */
    if (r1 == HL16 && IsAnyOf(r2, BC16, DE16, HL16, SP16, INVALID_REG))
    {
        PCWrite(0xed);
        PCWrite(0x42 | register_bitmask[r2] << 4);
        return CMD_OK;
    }

    if (!WriteRegisterPairModes(codes, NUM_REGISTER_CODES(codes),
                                r1, r2, off1, off2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus AND(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xe6, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0xa6}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* AND A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        PCWrite(0xa0 | register_bitmask[r2]);
        return CMD_OK;
    }

    if (!WriteRegisterPairModes(codes, NUM_REGISTER_CODES(codes),
                                r1, r2, off1, off2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus OR(const char *label, int argc, char *argv[],       
                        int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xf6, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0xb6}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* OR A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        PCWrite(0xb0 | register_bitmask[r2]);
        return CMD_OK;
    }

    if (!WriteRegisterPairModes(codes, NUM_REGISTER_CODES(codes),
                                r1, r2, off1, off2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus XOR(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xee, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0xae}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* XOR A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        PCWrite(0xa8 | register_bitmask[r2]);
        return CMD_OK;
    }

    if (!WriteRegisterPairModes(codes, NUM_REGISTER_CODES(codes),
                                r1, r2, off1, off2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus CP(const char *label, int argc, char *argv[],       
                        int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xfe, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0xbe}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* CP A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        PCWrite(0xb8 | register_bitmask[r2]);
        return CMD_OK;
    }

    if (!WriteRegisterPairModes(codes, NUM_REGISTER_CODES(codes),
                                r1, r2, off1, off2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus INC(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    RegisterMode r1;
    RegisterType t1;
    long off1;

    CMD_ARGC_CHECK(2);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    /* INC r
    */
    if (IsNormal8Bit(t1))
    {
        PCWrite(0x04 | register_bitmask[r1] << 3);
        return CMD_OK;
    }

    /* INC (HL)
    */
    if (r1 == HL_ADDRESS)
    {
        PCWrite(0x34);
        return CMD_OK;
    }

    /* INC rr
    */
    if (Is16Bit(t1) && !IsMemory(t1))
    {
        PCWrite(0x03 | register_bitmask[r1] << 4);
        return CMD_OK;
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


static CommandStatus DEC(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    RegisterMode r1;
    RegisterType t1;
    long off1;

    CMD_ARGC_CHECK(2);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    /* DEC r
    */
    if (IsNormal8Bit(t1))
    {
        PCWrite(0x05 | register_bitmask[r1] << 3);
        return CMD_OK;
    }

    /* DEC (HL)
    */
    if (r1 == HL_ADDRESS)
    {
        PCWrite(0x35);
        return CMD_OK;
    }

    /* DEC rr
    */
    if (Is16Bit(t1) && !IsMemory(t1))
    {
        PCWrite(0x0b | register_bitmask[r1] << 4);
        return CMD_OK;
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


static CommandStatus RLC_RL_SLA_SWAP(const char *label, int argc, char *argv[],
                                     int quoted[], char *err, size_t errsize)
{
    RegisterMode r1;
    RegisterType t1;
    long off1;
    int opcode_mask;

    CMD_ARGC_CHECK(2);

    if (CompareString(argv[0], "RLC"))
    {
        opcode_mask = 0x00;
    }
    else if (CompareString(argv[0], "RL"))
    {
        opcode_mask = 0x10;
    }
    else if (CompareString(argv[0], "SLA"))
    {
        opcode_mask = 0x20;
    }
    else if (CompareString(argv[0], "SWAP"))
    {
        opcode_mask = 0x30;
    }

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    /* Normal opcodes
    */
    if (argc == 2)
    {
        /* OP r
        */
        if (IsNormal8Bit(t1))
        {
            PCWrite(0xcb);
            PCWrite(opcode_mask | register_bitmask[r1]);
            return CMD_OK;
        }

        /* OP (HL)
        */
        if (r1 == HL_ADDRESS)
        {
            PCWrite(0xcb);
            PCWrite(opcode_mask | 0x06);
            return CMD_OK;
        }
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


static CommandStatus BIT_SET_RES(const char *label, int argc, char *argv[],
                                 int quoted[], char *err, size_t errsize)
{
    RegisterMode r1, r2;
    RegisterType t1, t2;
    long off1, off2;
    int opcode_mask;

    CMD_ARGC_CHECK(3);

    if (CompareString(argv[0], "BIT"))
    {
        opcode_mask = 0x40;
    }
    else if (CompareString(argv[0], "SET"))
    {
        opcode_mask = 0xc0;
    }
    else if (CompareString(argv[0], "RES"))
    {
        opcode_mask = 0x80;
    }

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    if (!CalcRegisterMode(argv[2], quoted[2], &r2, &t2, &off2, err, errsize))
    {
        return CMD_FAILED;
    }

    if (r1 != VALUE || (off1 < 0 || off1 > 7))
    {
        snprintf(err, errsize, "%s: illegal value %s for bit number",
                                        argv[0], argv[1]);
        return CMD_FAILED;
    }

    /* Normal opcodes
    */
    if (argc == 3)
    {
        /* OP b,r
        */
        if (IsNormal8Bit(t2))
        {
            PCWrite(0xcb);
            PCWrite(opcode_mask | off1 << 3 | register_bitmask[r2]);
            return CMD_OK;
        }

        /* OP b,(HL)
        */
        if (r2 == HL_ADDRESS)
        {
            PCWrite(0xcb);
            PCWrite(opcode_mask | off1 << 3 | 0x06);
            return CMD_OK;
        }
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


static CommandStatus JP(const char *label, int argc, char *argv[],
                        int quoted[], char *err, size_t errsize)
{
    if (argc == 2)
    {
        RegisterMode mode;
        RegisterType type;
        long val;

        if (!CalcRegisterMode(argv[1], quoted[1], &mode, &type, &val,
                              err, errsize))
        {
            return CMD_FAILED;
        }

        if (mode == VALUE)
        {
            PCWrite(0xc3);
            PCWriteWord(val);
            return CMD_OK;
        }

        if (mode == HL_ADDRESS)
        {
            PCWrite(0xe9);
            return CMD_OK;
        }
    }
    else if (argc == 3)
    {
        RegisterMode mode;
        RegisterType type;
        long val;
        ProcessorFlag flag;
        int mask;

        if (!CalcFlagMode(argv[1], &flag, &mask, err, errsize) ||
            !CalcRegisterMode(argv[2], quoted[2], &mode, &type, &val,
                              err, errsize))
        {
            return CMD_FAILED;
        }

        if (mode == VALUE)
        {
            PCWrite(0xc2 | mask << 3);
            PCWriteWord(val);
            return CMD_OK;
        }
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


static CommandStatus JR(const char *label, int argc, char *argv[],
                        int quoted[], char *err, size_t errsize)
{
    if (argc == 2)
    {
        RegisterMode mode;
        RegisterType type;
        long val;

        if (!CalcRegisterMode(argv[1], quoted[1], &mode, &type, &val,
                              err, errsize))
        {
            return CMD_FAILED;
        }

        if (mode == VALUE)
        {
            int rel;

            rel = val - ((PC() + 2) % 0x10000);

            CheckOffset(argv[1], rel);

            PCWrite(0x18);
            PCWrite(rel);
            return CMD_OK;
        }
    }
    else if (argc == 3)
    {
        RegisterMode mode;
        RegisterType type;
        long val;
        ProcessorFlag flag;
        int mask;

        if (!CalcFlagMode(argv[1], &flag, &mask, err, errsize) ||
            !CalcRegisterMode(argv[2], quoted[2], &mode, &type, &val,
                              err, errsize))
        {
            return CMD_FAILED;
        }

        if (mode == VALUE && (flag >= NZ_FLAG && flag <= C_FLAG))
        {
            int rel;

            rel = val - ((PC() + 2) % 0x10000);

            CheckOffset(argv[2], rel);

            PCWrite(0x20 | mask << 3);
            PCWrite(rel);
            return CMD_OK;
        }
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


static CommandStatus CALL(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    if (argc == 2)
    {
        RegisterMode mode;
        RegisterType type;
        long val;

        if (!CalcRegisterMode(argv[1], quoted[1], &mode, &type, &val,
                              err, errsize))
        {
            return CMD_FAILED;
        }

        if (mode == VALUE)
        {
            PCWrite(0xcd);
            PCWriteWord(val);
            return CMD_OK;
        }
    }
    else if (argc == 3)
    {
        RegisterMode mode;
        RegisterType type;
        long val;
        ProcessorFlag flag;
        int mask;

        if (!CalcFlagMode(argv[1], &flag, &mask, err, errsize) ||
            !CalcRegisterMode(argv[2], quoted[2], &mode, &type, &val,
                              err, errsize))
        {
            return CMD_FAILED;
        }

        if (mode == VALUE)
        {
            PCWrite(0xc4 | mask << 3);
            PCWriteWord(val);
            return CMD_OK;
        }
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


static CommandStatus RET(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    if (argc == 1)
    {
        PCWrite(0xc9);
        return CMD_OK;
    }
    else if (argc == 2)
    {
        ProcessorFlag flag;
        int mask;

        if (!CalcFlagMode(argv[1], &flag, &mask, err, errsize))
        {
            return CMD_FAILED;
        }

        PCWrite(0xc0 | mask << 3);
        return CMD_OK;
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


static CommandStatus RST(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    static struct
    {
        int     dec;
        int     hex;
    } rst_arg[] =
    {
        {0,     0x00},
        {8,     0x08},
        {10,    0x10},
        {18,    0x18},
        {20,    0x20},
        {28,    0x28},
        {30,    0x30},
        {38,    0x38},
        {-1}
    };

    RegisterMode mode;
    RegisterType type;
    long val;

    CMD_ARGC_CHECK(2);

    if (!CalcRegisterMode(argv[1], quoted[1], &mode, &type, &val,
                          err, errsize))
    {
        return CMD_FAILED;
    }

    if (mode == VALUE)
    {
        int f;
        int mask = -1;

        for(f = 0; rst_arg[f].dec != -1; f++)
        {
            if (rst_arg[f].dec == val || rst_arg[f].hex == val)
            {
                mask = f << 3;
            }
        }

        if (mask != -1)
        {
            PCWrite(0xc7 | mask);
            return CMD_OK;
        }
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


static CommandStatus STOP(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    if (argc == 1)
    {
        PCWrite(0x10);
        PCWrite(0x00);
        return CMD_OK;
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}



/* ---------------------------------------- OPCODE TABLES
*/
typedef struct
{
    const char  *op;
    int         code[3];        /* Zero ends, but code[0] always used. */
} OpcodeTable;

typedef struct
{
    const char  *op;
    Command     cmd;
} HandlerTable;


static const HandlerTable handler_table[] =
{
    {"LD",      LD},
    {"LDH",     LDH},
    {"LDD",     LDD},
    {"LDI",     LDI},
    {"PUSH",    PUSH},
    {"POP",     POP},
    {"ADD",     ADD},
    {"ADC",     ADC},
    {"SUB",     SUB},
    {"SBC",     SBC},
    {"AND",     AND},
    {"OR",      OR},
    {"XOR",     XOR},
    {"EOR",     XOR},
    {"CP",      CP},
    {"INC",     INC},
    {"DEC",     DEC},
    {"RLC",     RLC_RL_SLA_SWAP},
    {"RL",      RLC_RL_SLA_SWAP},
    {"SLA",     RLC_RL_SLA_SWAP},
    {"SWAP",    RLC_RL_SLA_SWAP},
    {"BIT",     BIT_SET_RES},
    {"RES",     BIT_SET_RES},
    {"SET",     BIT_SET_RES},
    {"JP",      JP},
    {"JR",      JR},
    {"CALL",    CALL},
    {"RET",     RET},
    {"RST",     RST},
    {"STOP",    STOP},
    {NULL}
};


static const OpcodeTable implied_opcodes[] =
{
    {"NOP",     {0x00}},
    {"DI",      {0xf3}},
    {"EI",      {0xfb}},
    {"HALT",    {0x76}},
    {"HLT",     {0x76}},
    {"DAA",     {0x27}},
    {"CPL",     {0x2f}},
    {"SCF",     {0x37}},
    {"CCF",     {0x3f}},
    {"RLCA",    {0x07}},
    {"RRCA",    {0x0f}},
    {"RLA",     {0x17}},
    {"RRA",     {0x1f}},
    {"RETI",    {0xd9}},
    {NULL}
};


/* ---------------------------------------- PUBLIC INTERFACES
*/

void Init_GBCPU(void)
{
}


const ValueTable *Options_GBCPU(void)
{
    return NULL;
}
 

CommandStatus SetOption_GBCPU(int opt, int argc, char *argv[], int quoted[],
                            char *err, size_t errsize)
{
    return CMD_OK;
}


CommandStatus Handler_GBCPU(const char *label, int argc, char *argv[],       
                           int quoted[], char *err, size_t errsize)
{
    int f;

    /* Check for simple (implied addressing) opcodes
    */
    for(f = 0; implied_opcodes[f].op; f++)
    {
        if (CompareString(argv[0], implied_opcodes[f].op))
        {
            int r;

            PCWrite(implied_opcodes[f].code[0]);

            for(r = 1; implied_opcodes[f].code[r]; r++)
            {
                PCWrite(implied_opcodes[f].code[r]);
            }

            return CMD_OK;
        }
    }

    /* Check for other opcodes
    */
    for(f = 0; handler_table[f].op; f++)
    {
        if (CompareString(argv[0], handler_table[f].op))
        {
            return handler_table[f].cmd(label, argc, argv,
                                        quoted, err, errsize);;
        }
    }

    return CMD_NOT_KNOWN;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
