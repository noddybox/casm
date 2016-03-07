/*

    casm - Simple, portable assembler

    Copyright (C) 2003-2015  Ian Cowburn (ianc@noddybox.demon.co.uk)

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

    Z80 Assembler

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

#include "z80.h"


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
    IXH8,
    IXL8,
    IYH8,
    IYL8,
    I8,
    R8,
    AF16,
    AF16_ALT,
    BC16,
    DE16,
    HL16,
    SP16,
    IX16,
    IY16,
    BC_ADDRESS,
    DE_ADDRESS,
    HL_ADDRESS,
    SP_ADDRESS,
    IX_ADDRESS,
    IY_ADDRESS,
    IX_OFFSET,
    IY_OFFSET,
    C_PORT,
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
    PO_FLAG,
    PE_FLAG,
    P_FLAG,
    M_FLAG
} ProcessorFlag;


typedef enum
{
    IS_NORMAL_8_BIT     =       0x001,
    IS_SPECIAL_8_BIT    =       0x002,
    IS_16_BIT           =       0x004,
    IS_MEMORY           =       0x008,
    IS_INDEX_X          =       0x010,
    IS_INDEX_Y          =       0x020,
    IS_SP               =       0x040,
    IS_VALUE            =       0x080,
    IS_SPECIAL_16_BIT   =       0x100,
    IS_ALTERNATE        =       0x200,
    IS_IO_PORT          =       0x400
} RegisterType;

#define IsNormal8Bit(t)         ((t) & IS_NORMAL_8_BIT)
#define IsSpecial8Bit(t)        ((t) & IS_SPECIAL_8_BIT)
#define Is16Bit(t)              ((t) & IS_16_BIT)
#define IsMemory(t)             ((t) & IS_MEMORY)
#define IsIndexX(t)             ((t) & IS_INDEX_X)
#define IsIndexY(t)             ((t) & IS_INDEX_Y)
#define IsIndex(t)              (IsIndexX(t) || IsIndexY(t))
#define IsSP(t)                 ((t) & IS_SP)
#define IsAddress(t)            (((t) & IS_VALUE) && ((t) & IS_MEMORY))
#define IsValue(t)              ((t) & IS_VALUE)
#define IsSimpleValue(t)        ((t) == IS_VALUE)
#define IsAlternate(t)          ((t) & IS_ALTERNATE)

#define SHIFT_IX                0xdd
#define SHIFT_IY                0xfd

#define WriteShift(r)                                   \
do                                                      \
{                                                       \
    switch(r)                                           \
    {                                                   \
        case IXH8:                                      \
        case IXL8:                                      \
        case IX16:                                      \
        case IX_OFFSET:                                 \
        case IX_ADDRESS:                                \
            PCWrite(SHIFT_IX);                          \
            break;                                      \
        case IYH8:                                      \
        case IYL8:                                      \
        case IY16:                                      \
        case IY_OFFSET:                                 \
        case IY_ADDRESS:                                \
            PCWrite(SHIFT_IY);                          \
            break;                                      \
        default:                                        \
            break;                                      \
    }                                                   \
} while(0)

#define WriteEitherShift(r1,r2)                         \
do                                                      \
{                                                       \
    int h_handled = FALSE;                              \
    switch(r1)                                          \
    {                                                   \
        case IXH8:                                      \
        case IXL8:                                      \
        case IX16:                                      \
        case IX_OFFSET:                                 \
        case IX_ADDRESS:                                \
            PCWrite(SHIFT_IX);                          \
            h_handled = TRUE;                           \
            break;                                      \
        case IYH8:                                      \
        case IYL8:                                      \
        case IY16:                                      \
        case IY_OFFSET:                                 \
        case IY_ADDRESS:                                \
            PCWrite(SHIFT_IY);                          \
            h_handled = TRUE;                           \
            break;                                      \
        default:                                        \
            break;                                      \
    }                                                   \
    if (!h_handled)                                     \
    switch(r2)                                          \
    {                                                   \
        case IXH8:                                      \
        case IXL8:                                      \
        case IX16:                                      \
        case IX_OFFSET:                                 \
        case IX_ADDRESS:                                \
            PCWrite(SHIFT_IX);                          \
            h_handled = TRUE;                           \
            break;                                      \
        case IYH8:                                      \
        case IYL8:                                      \
        case IY16:                                      \
        case IY_OFFSET:                                 \
        case IY_ADDRESS:                                \
            PCWrite(SHIFT_IY);                          \
            h_handled = TRUE;                           \
            break;                                      \
        default:                                        \
            break;                                      \
    }                                                   \
} while(0)

#define WriteOffset(r,o)                                \
do                                                      \
{                                                       \
    switch(r)                                           \
    {                                                   \
        case IX_OFFSET:                                 \
        case IY_OFFSET:                                 \
            PCWrite(o);                                 \
            break;                                      \
        default:                                        \
            break;                                      \
    }                                                   \
} while(0)

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
    0x03,               /* C_FLAG  */
    0x04,               /* PO_FLAG */
    0x05,               /* PE_FLAG */
    0x06,               /* P_FLAG  */
    0x07                /* M_FLAG  */
};


static const char *flag_text[] =
{
    "NZ",               /* NZ_FLAG */
    "Z",                /* Z_FLAG  */
    "NC",               /* NC_FLAG */
    "C",                /* C_FLAG  */
    "PO",               /* PO_FLAG */
    "PE",               /* PE_FLAG */
    "P",                /* P_FLAG  */
    "M",                /* M_FLAG  */
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
    "IXH",
    "IXL",
    "IYH",
    "IYL",
    "I",
    "R",
    "AF",
    "AF'",
    "BC",
    "DE",
    "HL",
    "SP",
    "IX",
    "IY",
    "(BC)",
    "(DE)",
    "(HL)",
    "(SP)",
    "(IX)",
    "(IY)",
    "(IX+offset)",
    "(IY+offset)",
    "(C)",
    "(address)",
    "value",
    0
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
    0x4,        /* IXH8 - In effect H  */
    0x5,        /* IXL8 - In effect L  */
    0x4,        /* IYH8 - In effect H  */
    0x5,        /* IYL8 - In effect L  */
    -1,         /* I8                  */
    -1,         /* R8                  */
    0x3,        /* AF16                */
    -1,         /* AF16_ALT            */
    0x0,        /* BC16                */
    0x1,        /* DE16                */
    0x2,        /* HL16                */
    0x3,        /* SP16                */
    0x2,        /* IX16 - In effect HL */
    0x2,        /* IY16 - In effect HL */
    0x0,        /* BC_ADDRESS          */
    0x0,        /* DE_ADDRESS          */
    0x0,        /* HL_ADDRESS          */
    0x0,        /* SP_ADDRESS          */
    0x0,        /* IX_ADDRESS          */
    0x0,        /* IY_ADDRESS          */
    0x0,        /* IX_OFFSET           */
    0x0,        /* IY_OFFSET           */
    0x0,        /* C_PORT              */
    0x0,        /* ADDRESS             */
    0x0,        /* VALUE               */
};


typedef struct
{
    RegisterMode        mode;
    int                 quote;
    int                 starts_with;
    int                 take_offset;
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
        FALSE,
        "A",
        IS_NORMAL_8_BIT
    },
    {
        B8,
        0,
        FALSE,
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
        FALSE,
        "C",
        IS_NORMAL_8_BIT
    },
    {
        D8,
        0,
        FALSE,
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
        FALSE,
        "E",
        IS_NORMAL_8_BIT
    },
    {
        H8,
        0,
        FALSE,
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
        FALSE,
        "L",
        IS_NORMAL_8_BIT
    },
    {
        F8,
        0,
        FALSE,
        FALSE,
        FALSE,
        "F",
        IS_SPECIAL_8_BIT
    },
    {
        IXL8,
        0,
        FALSE,
        FALSE,
        FALSE,
        "IXL",
        IS_NORMAL_8_BIT|IS_INDEX_X
    },
    {
        IXH8,
        0,
        FALSE,
        FALSE,
        FALSE,
        "IXH",
        IS_NORMAL_8_BIT|IS_INDEX_X
    },
    {
        IYL8,
        0,
        FALSE,
        FALSE,
        FALSE,
        "IYL",
        IS_NORMAL_8_BIT|IS_INDEX_Y
    },
    {
        IYH8,
        0,
        FALSE,
        FALSE,
        FALSE,
        "IYH",
        IS_NORMAL_8_BIT|IS_INDEX_Y
    },
    {
        I8,
        0,
        FALSE,
        FALSE,
        FALSE,
        "I",
        IS_SPECIAL_8_BIT
    },
    {
        R8,
        0,
        FALSE,
        FALSE,
        FALSE,
        "R",
        IS_SPECIAL_8_BIT
    },
        
    {
        AF16,
        0,
        FALSE,
        FALSE,
        FALSE,
        "AF",
        IS_SPECIAL_16_BIT
    },
    {
        AF16_ALT,
        0,
        FALSE,
        FALSE,
        FALSE,
        "AF'",
        IS_SPECIAL_16_BIT|IS_ALTERNATE
    },
    {
        BC16,
        0,
        FALSE,
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
        FALSE,
        "DE",
        IS_16_BIT
    },
    {
        HL16,
        0,
        FALSE,
        FALSE,
        FALSE,
        "HL",
        IS_16_BIT
    },
    {
        IX16,
        0,
        FALSE,
        FALSE,
        FALSE,
        "IX",
        IS_16_BIT|IS_INDEX_X
    },
    {
        IY16,
        0,
        FALSE,
        FALSE,
        FALSE,
        "IY",
        IS_16_BIT|IS_INDEX_Y
    },
    {
        SP16,
        0,
        FALSE,
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
        FALSE,
        "BC",
        IS_16_BIT|IS_MEMORY
    },
    {
        DE_ADDRESS,
        '(',
        FALSE,
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
        FALSE,
        "HL",
        IS_16_BIT|IS_MEMORY
    },
    {
        IX_ADDRESS,
        '(',
        FALSE,
        FALSE,
        FALSE,
        "IX",
        IS_16_BIT|IS_MEMORY|IS_INDEX_X
    },
    {
        IY_ADDRESS,
        '(',
        FALSE,
        FALSE,
        FALSE,
        "IY",
        IS_16_BIT|IS_MEMORY|IS_INDEX_Y
    },
    {
        IX_OFFSET,
        '(',
        TRUE,
        TRUE,
        FALSE,
        "IX",
        IS_16_BIT|IS_MEMORY|IS_INDEX_X
    },
    {
        IY_OFFSET,
        '(',
        TRUE,
        TRUE,
        FALSE,
        "IY",
        IS_16_BIT|IS_MEMORY|IS_INDEX_Y
    },
    {
        SP_ADDRESS,
        '(',
        FALSE,
        FALSE,
        FALSE,
        "SP",
        IS_SPECIAL_16_BIT|IS_MEMORY|IS_SP
    },
    {
        C_PORT,
        '(',
        FALSE,
        FALSE,
        FALSE,
        "C",
        IS_IO_PORT
    },

    /* These cheat -- basically anything that doesn't match until here is either
       an address or a value.  No distinction is made between 8/16 bit values.
    */
    {
        ADDRESS,
        '(',
        TRUE,
        FALSE,
        TRUE,
        "",
        IS_VALUE|IS_MEMORY
    },
    {
        VALUE,
        0,
        TRUE,
        FALSE,
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
                            int *offset,
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

                if (t->take_offset || t->take_value)
                {
                    size_t l = strlen(t->ident);

                    if (!ExprEval(arg + l, offset))
                    {
                        snprintf(err, errsize, "%s: expression error: %s",
                                                        arg, ExprError());
                        return FALSE;
                    }
                }

                if (t->take_offset)
                {
                    if (IsFinalPass() && (*offset < -128 || *offset > 127))
                    {
                        snprintf(err, errsize, "%s: outside valid range "
                                                "for offset", arg);
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
CommandStatus AssumeAccumulator(int argc, char *argv[], int quoted[],
                                char *err, size_t errsize,
                                RegisterMode *r1, RegisterMode *r2,
                                RegisterType *t1, RegisterType *t2,
                                int *off1, int *off2)
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
CommandStatus LD(const char *label, int argc, char *argv[],       
                 int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             BC_ADDRESS,     {0x0a},
        A8,             DE_ADDRESS,     {0x1a},
        A8,             ADDRESS,        {0x3a, WRITE_WORD_RHS},
        BC_ADDRESS,     A8,             {0x02},
        DE_ADDRESS,     A8,             {0x12},
        ADDRESS,        A8,             {0x32, WRITE_WORD_RHS},
        A8,             I8,             {0xed, 0x57},
        A8,             R8,             {0xed, 0x5f},
        I8,             A8,             {0xed, 0x47},
        R8,             A8,             {0xed, 0x4f}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;

    CMD_ARGC_CHECK(3);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    if (!CalcRegisterMode(argv[2], quoted[2], &r2, &t2, &off2, err, errsize))
    {
        return CMD_FAILED;
    }

    if ((IsIndexX(t1) && IsIndexY(t2)) ||
        (IsIndexY(t1) && IsIndexX(t2)))
    {
        snprintf(err, errsize,
                 "%s: can't have mixed IX/IY registers", argv[0]);

        return CMD_FAILED;
    }

    /* LD r,r'
    */
    if (IsNormal8Bit(t1) && IsNormal8Bit(t2))
    {
        CommandStatus s = CMD_OK;

        if ((IsIndex(t1) || IsIndex(t2)) &&
                (IsAnyOf(r1, H8, L8, INVALID_REG) ||
                 IsAnyOf(r2, H8, L8, INVALID_REG)))
        {
            snprintf(err, errsize, "%s: H/L will actually be the index "
                                        "register low/high register", argv[0]);
            s = CMD_OK_WARNING;
        }

        WriteEitherShift(r1,r2);
        PCWrite(0x40 | register_bitmask[r1] << 3 | register_bitmask[r2]);

        return s;
    }


    /* LD r,n
    */
    if (IsNormal8Bit(t1) && IsSimpleValue(t2))
    {
        WriteShift(r1);
        PCWrite(register_bitmask[r1] << 3 | 0x6);
        PCWrite(off2);

        return CMD_OK;
    }


    /* LD r,(HL)/(IX+d)/(IY+d)
    */
    if ((IsNormal8Bit(t1) && !IsIndex(t1))  &&
                (r2 == HL_ADDRESS || IsIndex(t2)))
    {
        WriteShift(r2);
        PCWrite(0x46 | register_bitmask[r1] << 3);
        WriteOffset(r2, off2);

        return CMD_OK;
    }


    /* LD (HL)/(IX+d)/(IY+d),r
    */
    if ((IsNormal8Bit(t2) && !IsIndex(t2))  &&
                (r1 == HL_ADDRESS || IsIndex(t1)))
    {
        WriteShift(r1);
        PCWrite(0x70 | register_bitmask[r2]);
        WriteOffset(r1, off1);

        return CMD_OK;
    }


    /* LD (HL)/(IX+d)/(IY+d),n
    */
    if ((r1 == HL_ADDRESS || r1 == IX_OFFSET || r1 == IY_OFFSET) && r2 == VALUE)
    {
        WriteShift(r1);
        PCWrite(0x36);
        WriteOffset(r1, off1);
        PCWrite(off2);
        return CMD_OK;
    }


    /* LD rr,nn
    */
    if (Is16Bit(t1) && !IsMemory(t1) && r2 == VALUE)
    {
        WriteShift(r1);
        PCWrite(register_bitmask[r1] << 4 | 0x01);
        PCWriteWord(off2);
        return CMD_OK;
    }


    /* LD HL/IX/IY,(nn)
    */
    if ((r1 == HL16 || r1 == IX16 || r1 == IY16) && r2 == ADDRESS)
    {
        WriteShift(r1);
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


    /* LD (nn),HL/IX/IY
    */
    if ((r2 == HL16 || r2 == IX16 || r2 == IY16) && r1 == ADDRESS)
    {
        WriteShift(r2);
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


    /* LD SP,HL/IX/IY
    */
    if ((r2 == HL16 || r2 == IX16 || r2 == IY16) && r1 == SP16)
    {
        WriteShift(r2);
        PCWrite(0xf9);
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


CommandStatus PUSH(const char *label, int argc, char *argv[],       
                   int quoted[], char *err, size_t errsize)
{
    RegisterMode r1;
    RegisterType t1;
    int off1;

    CMD_ARGC_CHECK(2);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    /* PUSH HL/IX/IY
    */
    if (r1 == HL16 || r1 == IX16 || r1 == IY16)
    {
        WriteShift(r1);
        PCWrite(0xe5);
        return CMD_OK;
    }


    /* PUSH rr
    */
    if (r1 == AF16 || r1 == BC16 || r1 == DE16)
    {
        PCWrite(0xc5 | register_bitmask[r1] << 4);
        return CMD_OK;
    }

    snprintf(err, errsize, "%s: invalid argument %s", argv[0], argv[1]);

    return CMD_OK;
}


CommandStatus POP(const char *label, int argc, char *argv[],       
                  int quoted[], char *err, size_t errsize)
{
    RegisterMode r1;
    RegisterType t1;
    int off1;

    CMD_ARGC_CHECK(2);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    /* POP HL/IX/IY
    */
    if (r1 == HL16 || r1 == IX16 || r1 == IY16)
    {
        WriteShift(r1);
        PCWrite(0xe1);
        return CMD_OK;
    }


    /* POP rr
    */
    if (r1 == AF16 || r1 == BC16 || r1 == DE16)
    {
        PCWrite(0xc1 | register_bitmask[r1] << 4);
        return CMD_OK;
    }

    snprintf(err, errsize, "%s: invalid argument %s", argv[0], argv[1]);

    return CMD_OK;
}


CommandStatus EX(const char *label, int argc, char *argv[],       
                 int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        DE16,           HL16,           {0xeb},
        AF16,           AF16_ALT,       {0x08},
        SP_ADDRESS,     HL16,           {0xe3},
        SP_ADDRESS,     IX16,           {SHIFT_IX, 0xe3},
        SP_ADDRESS,     IY16,           {SHIFT_IY, 0xe3}
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;

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


CommandStatus ADD(const char *label, int argc, char *argv[],       
                  int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xc6, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0x86},
        A8,             IX_OFFSET,      {SHIFT_IX, 0x86, WRITE_BYTE_RHS},
        A8,             IY_OFFSET,      {SHIFT_IY, 0x86, WRITE_BYTE_RHS},
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* ADD A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        WriteShift(r2);
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

    /* ADD IX,rr
    */
    if (r1 == IX16 && IsAnyOf(r2, BC16, DE16, IX16, SP16, INVALID_REG))
    {
        PCWrite(SHIFT_IX);
        PCWrite(0x09 | register_bitmask[r2] << 4);
        return CMD_OK;
    }

    /* ADD IY,rr
    */
    if (r1 == IY16 && IsAnyOf(r2, BC16, DE16, IY16, SP16, INVALID_REG))
    {
        PCWrite(SHIFT_IY);
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


CommandStatus ADC(const char *label, int argc, char *argv[],       
                  int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xce, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0x8e},
        A8,             IX_OFFSET,      {SHIFT_IX, 0x8e, WRITE_BYTE_RHS},
        A8,             IY_OFFSET,      {SHIFT_IY, 0x8e, WRITE_BYTE_RHS},
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* ADC A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        WriteShift(r2);
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


CommandStatus SUB(const char *label, int argc, char *argv[],       
                  int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xd6, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0x96},
        A8,             IX_OFFSET,      {SHIFT_IX, 0x96, WRITE_BYTE_RHS},
        A8,             IY_OFFSET,      {SHIFT_IY, 0x96, WRITE_BYTE_RHS},
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* SUB A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        WriteShift(r2);
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


CommandStatus SBC(const char *label, int argc, char *argv[],       
                  int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xde, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0x9e},
        A8,             IX_OFFSET,      {SHIFT_IX, 0x9e, WRITE_BYTE_RHS},
        A8,             IY_OFFSET,      {SHIFT_IY, 0x9e, WRITE_BYTE_RHS},
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* SBC A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        WriteShift(r2);
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


CommandStatus AND(const char *label, int argc, char *argv[],       
                  int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xe6, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0xa6},
        A8,             IX_OFFSET,      {SHIFT_IX, 0xa6, WRITE_BYTE_RHS},
        A8,             IY_OFFSET,      {SHIFT_IY, 0xa6, WRITE_BYTE_RHS},
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* AND A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        WriteShift(r2);
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


CommandStatus OR(const char *label, int argc, char *argv[],       
                 int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xf6, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0xb6},
        A8,             IX_OFFSET,      {SHIFT_IX, 0xb6, WRITE_BYTE_RHS},
        A8,             IY_OFFSET,      {SHIFT_IY, 0xb6, WRITE_BYTE_RHS},
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* OR A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        WriteShift(r2);
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


CommandStatus XOR(const char *label, int argc, char *argv[],       
                  int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xee, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0xae},
        A8,             IX_OFFSET,      {SHIFT_IX, 0xae, WRITE_BYTE_RHS},
        A8,             IY_OFFSET,      {SHIFT_IY, 0xae, WRITE_BYTE_RHS},
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* XOR A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        WriteShift(r2);
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


CommandStatus CP(const char *label, int argc, char *argv[],       
                 int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        A8,             VALUE,          {0xfe, WRITE_BYTE_RHS},
        A8,             HL_ADDRESS,     {0xbe},
        A8,             IX_OFFSET,      {SHIFT_IX, 0xbe, WRITE_BYTE_RHS},
        A8,             IY_OFFSET,      {SHIFT_IY, 0xbe, WRITE_BYTE_RHS},
    };

    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;

    if (AssumeAccumulator(argc, argv, quoted, err, errsize,
                          &r1, &r2, &t1, &t2, &off1, &off2) != CMD_OK)
    {
        return CMD_FAILED;
    }

    /* CP A,r
    */
    if (r1 == A8 && IsNormal8Bit(t2))
    {
        WriteShift(r2);
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


CommandStatus IM(const char *label, int argc, char *argv[],       
                 int quoted[], char *err, size_t errsize)
{
    static int im[3] = {0x46, 0x56, 0x5e};
    RegisterMode r1;
    RegisterType t1;
    int off1;

    CMD_ARGC_CHECK(2);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    if (r1 != VALUE || (off1 < 0 || off1 > 2))
    {
        snprintf(err, errsize, "%s: invalid argument %s", argv[0], argv[1]);
        return CMD_FAILED;
    }

    PCWrite(0xed);
    PCWrite(im[off1]);

    return CMD_OK;
}


CommandStatus INC(const char *label, int argc, char *argv[],       
                  int quoted[], char *err, size_t errsize)
{
    RegisterMode r1;
    RegisterType t1;
    int off1;

    CMD_ARGC_CHECK(2);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    /* INC r
    */
    if (IsNormal8Bit(t1))
    {
        WriteShift(r1);
        PCWrite(0x04 | register_bitmask[r1] << 3);
        return CMD_OK;
    }

    /* INC (HL)/(IX+d)/(IY+d)
    */
    if (r1 == HL_ADDRESS || r1 == IX_OFFSET || r1 == IY_OFFSET)
    {
        WriteShift(r1);
        PCWrite(0x34);
        WriteOffset(r1, off1);
        return CMD_OK;
    }

    /* INC rr
    */
    if (Is16Bit(t1) && !IsMemory(t1))
    {
        WriteShift(r1);
        PCWrite(0x03 | register_bitmask[r1] << 4);
        return CMD_OK;
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


CommandStatus DEC(const char *label, int argc, char *argv[],       
                  int quoted[], char *err, size_t errsize)
{
    RegisterMode r1;
    RegisterType t1;
    int off1;

    CMD_ARGC_CHECK(2);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    /* DEC r
    */
    if (IsNormal8Bit(t1))
    {
        WriteShift(r1);
        PCWrite(0x05 | register_bitmask[r1] << 3);
        return CMD_OK;
    }

    /* DEC (HL)/(IX+d)/(IY+d)
    */
    if (r1 == HL_ADDRESS || r1 == IX_OFFSET || r1 == IY_OFFSET)
    {
        WriteShift(r1);
        PCWrite(0x35);
        WriteOffset(r1, off1);
        return CMD_OK;
    }

    /* DEC rr
    */
    if (Is16Bit(t1) && !IsMemory(t1))
    {
        WriteShift(r1);
        PCWrite(0x0b | register_bitmask[r1] << 4);
        return CMD_OK;
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


CommandStatus RLC_RL_RRC_RR_ETC(const char *label, int argc, char *argv[],
                                int quoted[], char *err, size_t errsize)
{
    RegisterMode r1;
    RegisterType t1;
    int off1;
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
    else if (CompareString(argv[0], "RRC"))
    {
        opcode_mask = 0x08;
    }
    else if (CompareString(argv[0], "RR"))
    {
        opcode_mask = 0x18;
    }
    else if (CompareString(argv[0], "SLA"))
    {
        opcode_mask = 0x20;
    }
    else if (CompareString(argv[0], "SRA"))
    {
        opcode_mask = 0x28;
    }
    else if (CompareString(argv[0], "SRL"))
    {
        opcode_mask = 0x38;
    }
    else if (CompareString(argv[0], "SLL"))
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
        if (IsNormal8Bit(t1) && !IsIndex(t1))
        {
            PCWrite(0xcb);
            PCWrite(opcode_mask | register_bitmask[r1]);
            return CMD_OK;
        }

        /* OP (HL)/(IX+d)/(IY+d)
        */
        if (r1 == HL_ADDRESS || r1 == IX_OFFSET || r1 == IY_OFFSET)
        {
            WriteShift(r1);
            PCWrite(0xcb);
            WriteOffset(r1, off1);
            PCWrite(opcode_mask | 0x06);
            return CMD_OK;
        }
    }

    /* Undocumented opcodes
    */
    if (argc == 3)
    {
        RegisterMode r2;
        RegisterType t2;
        int off2;

        if (!CalcRegisterMode(argv[2], quoted[2], &r2, &t2,
                              &off2, err, errsize))
        {
            return CMD_FAILED;
        }

        /* OP (IX+d)/(IY+d),r
        */
        if ((r1 == IX_OFFSET || r1 == IY_OFFSET) && (r2 >= A8 && r2 <= L8))
        {
            WriteShift(r1);
            PCWrite(0xcb);
            WriteOffset(r1, off1);
            PCWrite(opcode_mask | register_bitmask[r2]);
            return CMD_OK;
        }
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


CommandStatus BIT_SET_RES(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;
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
        if (IsNormal8Bit(t2) && !IsIndex(t2))
        {
            PCWrite(0xcb);
            PCWrite(opcode_mask | off1 << 3 | register_bitmask[r2]);
            return CMD_OK;
        }

        /* OP b,(HL)/(IX+d)/(IY+d)
        */
        if (r2 == HL_ADDRESS || r2 == IX_OFFSET || r2 == IY_OFFSET)
        {
            WriteShift(r2);
            PCWrite(0xcb);
            WriteOffset(r2, off2);
            PCWrite(opcode_mask | off1 << 3 | 0x06);
            return CMD_OK;
        }
    }

    /* Undocumented opcodes
    */
    if (argc > 3 && (CompareString(argv[0], "SET") ||
                     CompareString(argv[0], "RES")))
    {
        RegisterMode r3;
        RegisterType t3;
        int off3;

        if (!CalcRegisterMode(argv[3], quoted[3], &r3, &t3,
                              &off3, err, errsize))
        {
            return CMD_FAILED;
        }

        /* OP b,(IX+d)/(IY+d),r
        */
        if ((r2 == IX_OFFSET || r2 == IY_OFFSET) && (r3 >= A8 && r3 <= L8))
        {
            WriteShift(r2);
            PCWrite(0xcb);
            WriteOffset(r2, off2);
            PCWrite(opcode_mask | off1 << 3 | register_bitmask[r3]);
            return CMD_OK;
        }
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


CommandStatus JP(const char *label, int argc, char *argv[],
                 int quoted[], char *err, size_t errsize)
{
    if (argc == 2)
    {
        RegisterMode mode;
        RegisterType type;
        int val;

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

        if (mode == HL_ADDRESS || mode == IX_ADDRESS || mode == IY_ADDRESS)
        {
            WriteShift(mode);
            PCWrite(0xe9);
            return CMD_OK;
        }
    }
    else if (argc == 3)
    {
        RegisterMode mode;
        RegisterType type;
        int val;
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


CommandStatus JR(const char *label, int argc, char *argv[],
                 int quoted[], char *err, size_t errsize)
{
    if (argc == 2)
    {
        RegisterMode mode;
        RegisterType type;
        int val;

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
        int val;
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


CommandStatus DJNZ(const char *label, int argc, char *argv[],
                   int quoted[], char *err, size_t errsize)
{
    RegisterMode mode;
    RegisterType type;
    int val;

    CMD_ARGC_CHECK(2);

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

        PCWrite(0x10);
        PCWrite(rel);

        return CMD_OK;
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


CommandStatus CALL(const char *label, int argc, char *argv[],
                   int quoted[], char *err, size_t errsize)
{
    if (argc == 2)
    {
        RegisterMode mode;
        RegisterType type;
        int val;

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
        int val;
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


CommandStatus RET(const char *label, int argc, char *argv[],
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


CommandStatus RST(const char *label, int argc, char *argv[],
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
    int val;

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


CommandStatus IN(const char *label, int argc, char *argv[],
                 int quoted[], char *err, size_t errsize)
{
    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;
    int opcode_mask;

    CMD_ARGC_CHECK(2);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    if (r1 == C_PORT && argc == 2)
    {
        PCWrite(0xed);
        PCWrite(0x70);
        return CMD_OK;
    }

    CMD_ARGC_CHECK(3);

    if (!CalcRegisterMode(argv[2], quoted[2], &r2, &t2, &off2, err, errsize))
    {
        return CMD_FAILED;
    }

    if (r1 == A8 && r2 == ADDRESS)
    {
        CheckRange(argv[2], off2, 0, 255);
        PCWrite(0xdb);
        PCWrite(off2);
        return CMD_OK;
    }

    if (!IsIndex(t1) && (IsNormal8Bit(t1) || r1 == F8) && r2 == C_PORT)
    {
        PCWrite(0xed);
        PCWrite(0x40 | register_bitmask[r1] << 3);
        return CMD_OK;
    }

    return IllegalArgs(argc, argv, quoted, err, errsize);
}


CommandStatus OUT(const char *label, int argc, char *argv[],
                  int quoted[], char *err, size_t errsize)
{
    RegisterMode r1, r2;
    RegisterType t1, t2;
    int off1, off2;
    int opcode_mask;

    CMD_ARGC_CHECK(3);

    if (!CalcRegisterMode(argv[1], quoted[1], &r1, &t1, &off1, err, errsize))
    {
        return CMD_FAILED;
    }

    if (!CalcRegisterMode(argv[2], quoted[2], &r2, &t2, &off2, err, errsize))
    {
        return CMD_FAILED;
    }

    if (r1 == ADDRESS && r2 == A8)
    {
        CheckRange(argv[1], off1, 0, 255);
        PCWrite(0xd3);
        PCWrite(off1);
        return CMD_OK;
    }

    if (r1 == C_PORT && !IsIndex(t2) && (IsNormal8Bit(t2) || r2 == F8))
    {
        PCWrite(0xed);
        PCWrite(0x41 | register_bitmask[r2] << 3);
        return CMD_OK;
    }

    if (r1 == C_PORT && r2 == VALUE)
    {
        PCWrite(0xed);
        PCWrite(0x71);
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
    {"PUSH",    PUSH},
    {"POP",     POP},
    {"EX",      EX},
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
    {"IM",      IM},
    {"RLC",     RLC_RL_RRC_RR_ETC},
    {"RL",      RLC_RL_RRC_RR_ETC},
    {"RRC",     RLC_RL_RRC_RR_ETC},
    {"RR",      RLC_RL_RRC_RR_ETC},
    {"SLA",     RLC_RL_RRC_RR_ETC},
    {"SRA",     RLC_RL_RRC_RR_ETC},
    {"SRL",     RLC_RL_RRC_RR_ETC},
    {"SLL",     RLC_RL_RRC_RR_ETC},
    {"BIT",     BIT_SET_RES},
    {"RES",     BIT_SET_RES},
    {"SET",     BIT_SET_RES},
    {"JP",      JP},
    {"JR",      JR},
    {"DJNZ",    DJNZ},
    {"CALL",    CALL},
    {"RET",     RET},
    {"RST",     RST},
    {"IN",      IN},
    {"OUT",     OUT},
    {NULL}
};


static const OpcodeTable implied_opcodes[] =
{
    {"NOP",     {0x00}},
    {"DI",      {0xf3}},
    {"EI",      {0xfb}},
    {"HALT",    {0x76}},
    {"HLT",     {0x76}},
    {"EXX",     {0xd9}},
    {"DAA",     {0x27}},
    {"CPL",     {0x2f}},
    {"SCF",     {0x37}},
    {"CCF",     {0x3f}},
    {"NEG",     {0xed, 0x44}},
    {"RLCA",    {0x07}},
    {"RRCA",    {0x0f}},
    {"RLA",     {0x17}},
    {"RRA",     {0x1f}},
    {"CPI",     {0xed, 0xa1}},
    {"CPIR",    {0xed, 0xb1}},
    {"CPD",     {0xed, 0xa9}},
    {"CPDR",    {0xed, 0xb9}},
    {"INI",     {0xed, 0xa2}},
    {"INIR",    {0xed, 0xb2}},
    {"IND",     {0xed, 0xaa}},
    {"INDR",    {0xed, 0xba}},
    {"OUTI",    {0xed, 0xa3}},
    {"OTIR",    {0xed, 0xb3}},
    {"OUTD",    {0xed, 0xab}},
    {"OTDR",    {0xed, 0xbb}},
    {"LDI",     {0xed, 0xa0}},
    {"LDIR",    {0xed, 0xb0}},
    {"LDD",     {0xed, 0xa8}},
    {"LDDR",    {0xed, 0xb8}},
    {"RRD",     {0xed, 0x67}},
    {"RLD",     {0xed, 0x6f}},
    {"RETI",    {0xed, 0x4d}},
    {"RETN",    {0xed, 0x45}},
    {NULL}
};


/* ---------------------------------------- PUBLIC INTERFACES
*/

void Init_Z80(void)
{
}


const ValueTable *Options_Z80(void)
{
    return NULL;
}
 

CommandStatus SetOption_Z80(int opt, int argc, char *argv[], int quoted[],
                            char *err, size_t errsize)
{
    return CMD_OK;
}


CommandStatus Handler_Z80(const char *label, int argc, char *argv[],       
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
