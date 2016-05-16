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

    SPC700 Assembler

*/
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "expr.h"
#include "label.h"
#include "parse.h"
#include "cmd.h"
#include "codepage.h"

#include "spc700.h"


/* ---------------------------------------- TYPES AND GLOBALS
*/
enum option_t
{
    OPT_DP
};

enum dp_mode_t
{
    DP_OFF,
    DP_ON,
    DP_AUTO
};

static const ValueTable options[] =
{
    {"direct-page",       OPT_DP},
    {NULL}
};

static const ValueTable dp_table[] =
{
    YES_NO_ENTRIES(DP_ON, DP_OFF),
    {"auto", DP_AUTO},
    {NULL}
};

static struct
{
    enum dp_mode_t dp_mode;
} option;


typedef enum
{
    ACCUMULATOR,
    X_REGISTER,
    Y_REGISTER,
    YA_REGISTER,
    SP_REGISTER,
    C_FLAG,
    PSW_REGISTER,
    BIT_ADDRESS,
    NOTTED_BIT_ADDRESS,
    INDIRECT_X,
    INDIRECT_Y,
    INDIRECT_X_INC,
    IMPLIED,
    IMMEDIATE,
    ABSOLUTE,
    DIRECT_PAGE,
    ABSOLUTE_INDEX_X,
    ABSOLUTE_INDEX_Y,
    DIRECT_PAGE_INDEX_X,
    DIRECT_PAGE_INDEX_Y,
    DIRECT_PAGE_INDIRECT_X,
    DIRECT_PAGE_INDIRECT_Y,
    ADDR_MODE_ERROR,
    ADDR_MODE_UNKNOWN
} address_mode_t;

static const char *address_mode_name[] =
{
    "Accumulator",
    "X register",
    "Y register",
    "YA register",
    "Stack Pointer",
    "Carry flag",
    "PSW register",
    "Bit address",
    "Notted (/) bit address",
    "Indirect X",
    "Indirect Y",
    "Indirect X increment",
    "Implied",
    "Immediate",
    "Absolute",
    "Direct Page",
    "Absolute, index X",
    "Absolute, index Y",
    "Direct Page, index X",
    "Direct Page, index Y",
    "Direct Page, indirect X",
    "Direct Page, indirect Y",
    "Address Mode Error",
    "Address Mode Unknown"
};


typedef enum 
{
    WB_LHS      = -1,
    WW_LHS      = -2,
    WB_RHS      = -3,
    WW_RHS      = -4
} StreamCodes;


typedef struct
{
    address_mode_t      lhs;
    address_mode_t      rhs;
    int                 code[10];
} RegisterPairCodes;

#define NUM_REGISTER_CODES(a)   ((sizeof a)/(sizeof a[1]))



/* ---------------------------------------- MACROS
*/

#define ADDRESS_MODE(mode, address, idx)                                \
do                                                                      \
{                                                                       \
    CalcAddressMode(argc, argv, quoted, idx, err, errsize,              \
                    &mode, &address);                                   \
                                                                        \
    if (mode == ADDR_MODE_UNKNOWN)                                      \
    {                                                                   \
        snprintf(err, errsize, "%s: couldn't work out "                 \
                                        "addressing mode", argv[0]);    \
        return CMD_FAILED;                                              \
    }                                                                   \
                                                                        \
    if (mode == ADDR_MODE_ERROR) return CMD_FAILED;                     \
} while(0)


#define CMD_DP_MODE(DP_mode, non_DP_mode)                               \
do                                                                      \
{                                                                       \
    switch(option.dp_mode)                                              \
    {                                                                   \
        case DP_ON:                                                     \
            if (*address < 0 || *address > 255)                         \
            {                                                           \
                snprintf(err, errsize, "value %d outside of "           \
                                            "zero page", *address);     \
                *mode = ADDR_MODE_ERROR;                                \
                return;                                                 \
            }                                                           \
                                                                        \
            *mode = DP_mode;                                            \
            break;                                                      \
                                                                        \
        case DP_OFF:                                                    \
            *mode = non_DP_mode;                                        \
            break;                                                      \
                                                                        \
        case DP_AUTO:                                                   \
            if (*address >= 0 && *address <= 255)                       \
            {                                                           \
                *mode = DP_mode;                                        \
            }                                                           \
            else                                                        \
            {                                                           \
                *mode = non_DP_mode;                                    \
            }                                                           \
            break;                                                      \
    }                                                                   \
} while (0)

#define CHECK_RANGE(val, min, max)                                      \
do {                                                                    \
    if (val < min || val > max)                                         \
    {                                                                   \
        snprintf(err, errsize, "%s: value %d outside "                  \
                                    "of valid range %d - %d",           \
                                        argv[0], val, min, max);        \
        return CMD_FAILED;                                              \
    }                                                                   \
} while(0)


/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static void CalcAddressMode(int argc, char *argv[], int quoted[], int index,
                            char *err, size_t errsize,
                            address_mode_t *mode, int *address)
{
    char *arg;
    int quote;

    *mode = ADDR_MODE_UNKNOWN;
    *address = 0;

    /* Implied
    */
    if (argc == 1)
    {
        *mode = IMPLIED;
        return;
    }

    arg = argv[index];
    quote = quoted[index];

    /* Accumulator and other simple fixed string register modes
    */
    if (CompareString(arg, "A"))
    {
        *mode = ACCUMULATOR;
        return;
    }

    if (CompareString(arg, "C"))
    {
        *mode = C_FLAG;
        return;
    }

    if (CompareString(arg, "PSW"))
    {
        *mode = PSW_REGISTER;
        return;
    }

    if (CompareString(arg, "X"))
    {
        if (quote == '(')
        {
            *mode = INDIRECT_X;
        }
        else
        {
            *mode = X_REGISTER;
        }
        return;
    }

    if (CompareString(arg, "Y"))
    {
        if (quote == '(')
        {
            *mode = INDIRECT_Y;
        }
        else
        {
            *mode = Y_REGISTER;
        }
        return;
    }

    if (CompareString(arg, "YA"))
    {
        *mode = YA_REGISTER;
        return;
    }

    if (CompareString(arg, "SP"))
    {
        *mode = SP_REGISTER;
        return;
    }

    if (CompareString(arg, "(X)+"))
    {
        *mode = INDIRECT_X_INC;
        return;
    }

    /* Bit addresses
    */
    if (strchr(arg, '.'))
    {
        char *copy;
        char *end;
        int a,b;

        copy = DupStr(arg);
        end = strrchr(copy, '.');
        *end++ = 0;

        if (arg[0] == '/')
        {
            *mode = NOTTED_BIT_ADDRESS;

            if (IsFinalPass() && !ExprEval(copy + 1, &a))
            {
                snprintf(err, errsize, "%s: expression error: %s",
                                            copy + 1, ExprError());
                *mode = ADDR_MODE_ERROR;
                free(copy);
                return;
            }
        }
        else
        {
            *mode = BIT_ADDRESS;

            if (IsFinalPass() && !ExprEval(copy, &a))
            {
                snprintf(err, errsize, "%s: expression error: %s",
                                            copy, ExprError());
                *mode = ADDR_MODE_ERROR;
                free(copy);
                return;
            }
        }

        if (IsFinalPass() && !ExprEval(end, &b))
        {
            snprintf(err, errsize, "%s: expression error: %s",
                                        end, ExprError());
            *mode = ADDR_MODE_ERROR;
            free(copy);
            return;
        }

        if (IsFinalPass() && (a < 0 || a > 0x1fff))
        {
            snprintf(err, errsize, "%s: address component of bit address "
                                    "out of valid range 0x0 to 0x1fff", copy);
            *mode = ADDR_MODE_ERROR;
            free(copy);
            return;
        }

        if (IsFinalPass() && (b < 0 || b > 7))
        {
            snprintf(err, errsize, "%s: address component of bit address "
                                    "out of valid range 0 to 7", end);
            *mode = ADDR_MODE_ERROR;
            free(copy);
            return;
        }

        *address = a | b << 13;

        free(copy);
        return;
    }

    /* Immediate
    */
    if (arg[0] == '#')
    {
        *mode = IMMEDIATE;

        if (IsFinalPass() && !ExprEval(arg + 1, address))
        {
            snprintf(err, errsize, "%s: expression error: %s",
                                        arg + 1, ExprError());
            *mode = ADDR_MODE_ERROR;
        }

        return;
    }

    /* Address modes involving '+' character
    */
    if (strchr(arg, '+'))
    {
        char *copy;
        char *end;

        copy = DupStr(arg);
        end = strrchr(copy, '+');
        *end++ = 0;

        /* Direct page indirect X
        */
        if (quote == '(' && CompareString(end, "X"))
        {
            if (IsFinalPass() && !ExprEval(copy, address))
            {
                snprintf(err, errsize, "%s: expression error: %s",
                                            copy, ExprError());
                *mode = ADDR_MODE_ERROR;
            }
            else
            {
                *mode = DIRECT_PAGE_INDIRECT_X;
            }

            free(copy);
            return;
        }

        /* Direct page indirect Y
        */
        if (!quote && *copy == '(' && CompareString(end, "Y"))
        {
            /* Remember that the expression parser doesn't allow
               round braclets.
            */
            TrimChars(copy, "()");

            if (IsFinalPass() && !ExprEval(copy, address))
            {
                snprintf(err, errsize, "%s: expression error: %s",
                                            copy, ExprError());
                *mode = ADDR_MODE_ERROR;
            }
            else
            {
                *mode = DIRECT_PAGE_INDIRECT_Y;
            }

            free(copy);
            return;
        }

        /* Direct page or absolute index X
        */
        if (!quote && CompareString(end, "X"))
        {
            if (IsFinalPass() && !ExprEval(copy, address))
            {
                snprintf(err, errsize, "%s: expression error: %s",
                                            copy, ExprError());
                *mode = ADDR_MODE_ERROR;
            }
            else
            {
                CMD_DP_MODE(DIRECT_PAGE_INDEX_X, ABSOLUTE_INDEX_X);
            }

            free(copy);
            return;
        }

        /* Direct page or absolute index Y
        */
        if (!quote && CompareString(end, "Y"))
        {
            if (IsFinalPass() && !ExprEval(copy, address))
            {
                snprintf(err, errsize, "%s: expression error: %s",
                                            copy, ExprError());
                *mode = ADDR_MODE_ERROR;
            }
            else
            {
                CMD_DP_MODE(DIRECT_PAGE_INDEX_Y, ABSOLUTE_INDEX_Y);
            }

            free(copy);
            return;
        }

        free(copy);
    }

    /* If all else fails, Absolute
    */
    if (IsFinalPass() && !ExprEval(arg, address))
    {
        snprintf(err, errsize, "%s: expression error:  %s",
                                    arg, ExprError());
        *mode = ADDR_MODE_ERROR;
        return;
    }

    CMD_DP_MODE(DIRECT_PAGE, ABSOLUTE);
}


static int WriteRegisterPairModes(const char *caller,
                                  const RegisterPairCodes *codes, size_t count,
                                  address_mode_t lhs, address_mode_t rhs,
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
                    case WB_LHS:
                        PCWrite(val_lhs);
                        break;

                    case WW_LHS:
                        PCWriteWord(val_lhs);
                        break;

                    case WB_RHS:
                        PCWrite(val_rhs);
                        break;

                    case WW_RHS:
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

    snprintf(err, errsize, "%s: no code generation for register pair %s/%s",
                     caller, address_mode_name[lhs], address_mode_name[rhs]);

    return FALSE;
}


static int MakeRelative(int *addr, char *cmd, char *err, size_t errsize)
{
    *addr = *addr - (PC() + 2);

    if (IsFinalPass() && (*addr < -128 || *addr > 127))
    {
        snprintf(err, errsize, "%s: Branch offset (%d) too big",
                                    cmd, *addr);
        return FALSE;
    }

    return TRUE;
}


/* ---------------------------------------- COMMAND HANDLERS
*/

#define COMMON_ALU(base)                                                       \
do {                                                                           \
static RegisterPairCodes codes[] =                                             \
{                                                                              \
    {ACCUMULATOR,           IMMEDIATE,              {base + 0x08, WB_RHS}},    \
    {ACCUMULATOR,           INDIRECT_X,             {base + 0x06}},            \
    {ACCUMULATOR,           DIRECT_PAGE,            {base + 0x04, WB_RHS}},    \
    {ACCUMULATOR,           DIRECT_PAGE_INDEX_X,    {base + 0x14, WB_RHS}},    \
    {ACCUMULATOR,           ABSOLUTE,               {base + 0x05, WW_RHS}},    \
    {ACCUMULATOR,           ABSOLUTE_INDEX_X,       {base + 0x15, WW_RHS}},    \
    {ACCUMULATOR,           ABSOLUTE_INDEX_Y,       {base + 0x16, WW_RHS}},    \
    {ACCUMULATOR,           DIRECT_PAGE_INDEX_Y,    {base + 0x16, WW_RHS}},    \
    {ACCUMULATOR,           DIRECT_PAGE_INDIRECT_X, {base + 0x07, WB_RHS}},    \
    {ACCUMULATOR,           DIRECT_PAGE_INDIRECT_Y, {base + 0x17, WB_RHS}},    \
    {INDIRECT_X,            INDIRECT_Y,             {base + 0x19}},            \
    {DIRECT_PAGE,           DIRECT_PAGE,            {base + 0x09,              \
                                                     WB_LHS, WB_RHS}},         \
    {DIRECT_PAGE,           IMMEDIATE,              {base + 0x18,              \
                                                     WB_LHS, WB_RHS}}          \
};                                                                             \
                                                                               \
address_mode_t mode1, mode2;                                                   \
int addr1, addr2;                                                              \
                                                                               \
CMD_ARGC_CHECK(3);                                                             \
                                                                               \
ADDRESS_MODE(mode1, addr1, 1);                                                 \
ADDRESS_MODE(mode2, addr2, 2);                                                 \
                                                                               \
if (!WriteRegisterPairModes(argv[0], codes, NUM_REGISTER_CODES(codes),         \
                            mode1, mode2, addr1, addr2, err, errsize))         \
{                                                                              \
    return CMD_FAILED;                                                         \
}                                                                              \
                                                                               \
return CMD_OK;                                                                 \
} while(0)


static CommandStatus MOV(const char *label, int argc, char *argv[],       
                         int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        {ACCUMULATOR,           IMMEDIATE,              {0xe8, WB_RHS}},
        {ACCUMULATOR,           INDIRECT_X,             {0xe6}},
        {ACCUMULATOR,           INDIRECT_X_INC,         {0xbf}},
        {ACCUMULATOR,           DIRECT_PAGE,            {0xe4, WB_RHS}},
        {ACCUMULATOR,           DIRECT_PAGE_INDEX_X,    {0xf4, WB_RHS}},
        {ACCUMULATOR,           ABSOLUTE,               {0xe5, WW_RHS}},
        {ACCUMULATOR,           ABSOLUTE_INDEX_X,       {0xf5, WW_RHS}},
        {ACCUMULATOR,           DIRECT_PAGE_INDEX_Y,    {0xf6, WW_RHS}},
        {ACCUMULATOR,           ABSOLUTE_INDEX_Y,       {0xf6, WW_RHS}},
        {ACCUMULATOR,           DIRECT_PAGE_INDIRECT_X, {0xe7, WB_RHS}},
        {ACCUMULATOR,           DIRECT_PAGE_INDIRECT_Y, {0xf7, WB_RHS}},

        {X_REGISTER,            IMMEDIATE,              {0xcd, WB_RHS}},
        {X_REGISTER,            DIRECT_PAGE,            {0xf8, WB_RHS}},
        {X_REGISTER,            DIRECT_PAGE_INDEX_Y,    {0xf9, WB_RHS}},
        {X_REGISTER,            ABSOLUTE,               {0xe9, WW_RHS}},

        {Y_REGISTER,            IMMEDIATE,              {0x8d, WB_RHS}},
        {Y_REGISTER,            DIRECT_PAGE,            {0xeb, WB_RHS}},
        {Y_REGISTER,            DIRECT_PAGE_INDEX_X,    {0xfb, WB_RHS}},
        {Y_REGISTER,            ABSOLUTE,               {0xec, WW_RHS}},

        {INDIRECT_X,            ACCUMULATOR,            {0xc6}},
        {INDIRECT_X_INC,        ACCUMULATOR,            {0xaf}},
        {DIRECT_PAGE,           ACCUMULATOR,            {0xc4, WB_LHS}},
        {DIRECT_PAGE_INDEX_X,   ACCUMULATOR,            {0xd4, WB_LHS}},
        {ABSOLUTE,              ACCUMULATOR,            {0xc5, WW_LHS}},
        {ABSOLUTE_INDEX_X,      ACCUMULATOR,            {0xd5, WW_LHS}},
        {ABSOLUTE_INDEX_Y,      ACCUMULATOR,            {0xd6, WW_LHS}},
        {DIRECT_PAGE_INDEX_Y,   ACCUMULATOR,            {0xd6, WW_LHS}},
        {DIRECT_PAGE_INDIRECT_X,ACCUMULATOR,            {0xc7, WB_LHS}},
        {DIRECT_PAGE_INDIRECT_Y,ACCUMULATOR,            {0xd7, WB_LHS}},

        {DIRECT_PAGE,           X_REGISTER,             {0xd8, WB_LHS}},
        {DIRECT_PAGE_INDEX_Y,   X_REGISTER,             {0xd9, WB_LHS}},
        {ABSOLUTE,              X_REGISTER,             {0xc9, WW_LHS}},

        {DIRECT_PAGE,           Y_REGISTER,             {0xcb, WB_LHS}},
        {DIRECT_PAGE_INDEX_X,   Y_REGISTER,             {0xdb, WB_LHS}},
        {ABSOLUTE,              Y_REGISTER,             {0x0c, WW_LHS}},

        {ACCUMULATOR,           X_REGISTER,             {0x7d}},
        {ACCUMULATOR,           Y_REGISTER,             {0xdd}},
        {X_REGISTER,            ACCUMULATOR,            {0x5d}},
        {Y_REGISTER,            ACCUMULATOR,            {0xfd}},
        {X_REGISTER,            SP_REGISTER,            {0x9d}},
        {SP_REGISTER,           X_REGISTER,             {0xbd}},
        {DIRECT_PAGE,           DIRECT_PAGE,            {0xfa, WB_LHS, WB_RHS}},
        {DIRECT_PAGE,           IMMEDIATE,              {0x8f, WB_LHS, WB_RHS}}
    };

    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (!WriteRegisterPairModes(argv[0], codes, NUM_REGISTER_CODES(codes),
                                mode1, mode2, addr1, addr2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}



static CommandStatus ADC(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    COMMON_ALU(0x80);
}

static CommandStatus ADDW(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        {YA_REGISTER,           DIRECT_PAGE,            {0x7a, WB_RHS}},
    };

    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (!WriteRegisterPairModes(argv[0], codes, NUM_REGISTER_CODES(codes),
                                mode1, mode2, addr1, addr2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}


static CommandStatus AND(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    COMMON_ALU(0x20);
}

static CommandStatus ASL(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case ACCUMULATOR:
            PCWrite(0x1c);
            return CMD_OK;

        case DIRECT_PAGE:
            PCWrite(0x0b);
            PCWrite(addr);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0x1b);
            PCWrite(addr);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xcc);
            PCWriteWord(addr);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus CMP(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        {ACCUMULATOR,           IMMEDIATE,              {0x68, WB_RHS}},
        {ACCUMULATOR,           INDIRECT_X,             {0x66}},
        {ACCUMULATOR,           DIRECT_PAGE,            {0x64, WB_RHS}},
        {ACCUMULATOR,           DIRECT_PAGE_INDEX_X,    {0x74, WB_RHS}},
        {ACCUMULATOR,           ABSOLUTE,               {0x65, WW_RHS}},
        {ACCUMULATOR,           ABSOLUTE_INDEX_X,       {0x75, WW_RHS}},
        {ACCUMULATOR,           ABSOLUTE_INDEX_Y,       {0x76, WW_RHS}},
        {ACCUMULATOR,           DIRECT_PAGE_INDEX_Y,    {0x76, WW_RHS}},
        {ACCUMULATOR,           DIRECT_PAGE_INDIRECT_X, {0x67, WB_RHS}},
        {ACCUMULATOR,           DIRECT_PAGE_INDIRECT_Y, {0x77, WB_RHS}},
        {INDIRECT_X,            INDIRECT_Y,             {0x79}},
        {DIRECT_PAGE,           DIRECT_PAGE,            {0x69, WB_LHS, WB_RHS}},
        {DIRECT_PAGE,           IMMEDIATE,              {0x78, WB_LHS, WB_RHS}},

        {X_REGISTER,            IMMEDIATE,              {0xc8, WB_RHS}},
        {X_REGISTER,            DIRECT_PAGE,            {0x3e, WB_RHS}},
        {X_REGISTER,            ABSOLUTE,               {0x1e, WW_RHS}},

        {Y_REGISTER,            IMMEDIATE,              {0xad, WB_RHS}},
        {Y_REGISTER,            DIRECT_PAGE,            {0x7e, WB_RHS}},
        {Y_REGISTER,            ABSOLUTE,               {0x5e, WW_RHS}},
    };

    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (!WriteRegisterPairModes(argv[0], codes, NUM_REGISTER_CODES(codes),
                                mode1, mode2, addr1, addr2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}

static CommandStatus CMPW(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        {YA_REGISTER,           DIRECT_PAGE,            {0x5a, WB_RHS}},
    };

    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (!WriteRegisterPairModes(argv[0], codes, NUM_REGISTER_CODES(codes),
                                mode1, mode2, addr1, addr2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}

static CommandStatus DEC(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case ACCUMULATOR:
            PCWrite(0x9c);
            return CMD_OK;

        case DIRECT_PAGE:
            PCWrite(0x8b);
            PCWrite(addr);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0x9b);
            PCWrite(addr);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x8c);
            PCWriteWord(addr);
            return CMD_OK;

        case X_REGISTER:
            PCWrite(0x1d);
            return CMD_OK;

        case Y_REGISTER:
            PCWrite(0xdc);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus DECW(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case DIRECT_PAGE:
            PCWrite(0x1a);
            PCWrite(addr);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus EOR(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    COMMON_ALU(0x40);
}

static CommandStatus INC(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case ACCUMULATOR:
            PCWrite(0xbc);
            return CMD_OK;

        case DIRECT_PAGE:
            PCWrite(0xab);
            PCWrite(addr);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0xbb);
            PCWrite(addr);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xac);
            PCWriteWord(addr);
            return CMD_OK;

        case X_REGISTER:
            PCWrite(0x3d);
            return CMD_OK;

        case Y_REGISTER:
            PCWrite(0xfc);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus INCW(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case DIRECT_PAGE:
            PCWrite(0x3a);
            PCWrite(addr);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus JMP(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int address;

    ADDRESS_MODE(mode, address, 1);

    switch(mode)
    {
        case ABSOLUTE:
        case DIRECT_PAGE:
            PCWrite(0x5f);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
        case ABSOLUTE_INDEX_X:
        case DIRECT_PAGE_INDIRECT_X:
            PCWrite(0x1f);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus CALL(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int address;

    ADDRESS_MODE(mode, address, 1);

    switch(mode)
    {
        case ABSOLUTE:
        case DIRECT_PAGE:
            PCWrite(0x3f);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus PCALL(const char *label, int argc, char *argv[],
                           int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int address;

    ADDRESS_MODE(mode, address, 1);

    switch(mode)
    {
        case ABSOLUTE:
        case DIRECT_PAGE:
            PCWrite(0x4f);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus TCALL(const char *label, int argc, char *argv[],
                           int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int address;

    ADDRESS_MODE(mode, address, 1);

    switch(mode)
    {
        case ABSOLUTE:
        case DIRECT_PAGE:
            CHECK_RANGE(address, 0, 15);
            PCWrite(0x01 + 0x10 * address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus LSR(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case ACCUMULATOR:
            PCWrite(0x5c);
            return CMD_OK;

        case DIRECT_PAGE:
            PCWrite(0x4b);
            PCWrite(addr);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0x5b);
            PCWrite(addr);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x4c);
            PCWriteWord(addr);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus OR(const char *label, int argc, char *argv[],
                        int quoted[], char *err, size_t errsize)
{
    COMMON_ALU(0x00);
}

static CommandStatus ROL(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case ACCUMULATOR:
            PCWrite(0x3c);
            return CMD_OK;

        case DIRECT_PAGE:
            PCWrite(0x2b);
            PCWrite(addr);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0x3b);
            PCWrite(addr);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x2c);
            PCWriteWord(addr);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus ROR(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case ACCUMULATOR:
            PCWrite(0x7c);
            return CMD_OK;

        case DIRECT_PAGE:
            PCWrite(0x6b);
            PCWrite(addr);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0x7b);
            PCWrite(addr);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x6c);
            PCWriteWord(addr);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus SBC(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    COMMON_ALU(0xa0);
}

static CommandStatus SUBW(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        {YA_REGISTER,           DIRECT_PAGE,            {0x9a, WB_RHS}},
    };

    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (!WriteRegisterPairModes(argv[0], codes, NUM_REGISTER_CODES(codes),
                                mode1, mode2, addr1, addr2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}

static CommandStatus XCN(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case ACCUMULATOR:
            PCWrite(0x9f);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus MOVW(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        {YA_REGISTER,           DIRECT_PAGE,            {0xba}},
        {DIRECT_PAGE,           YA_REGISTER,            {0xda}}
    };

    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (!WriteRegisterPairModes(argv[0], codes, NUM_REGISTER_CODES(codes),
                                mode1, mode2, addr1, addr2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}

static CommandStatus MUL(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case YA_REGISTER:
            PCWrite(0xcf);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus DIV(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    static RegisterPairCodes codes[] =
    {
        {YA_REGISTER,           X_REGISTER,             {0x9e}}
    };

    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (!WriteRegisterPairModes(argv[0], codes, NUM_REGISTER_CODES(codes),
                                mode1, mode2, addr1, addr2, err, errsize))
    {
        return CMD_FAILED;
    }

    return CMD_OK;
}

static CommandStatus DAA(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case ACCUMULATOR:
            PCWrite(0xdf);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus DAS(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case ACCUMULATOR:
            PCWrite(0xbe);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus PUSH(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case ACCUMULATOR:
            PCWrite(0x2d);
            return CMD_OK;

        case X_REGISTER:
            PCWrite(0x4d);
            return CMD_OK;

        case Y_REGISTER:
            PCWrite(0x6d);
            return CMD_OK;

        case PSW_REGISTER:
            PCWrite(0x0d);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus POP(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case ACCUMULATOR:
            PCWrite(0xae);
            return CMD_OK;

        case X_REGISTER:
            PCWrite(0xce);
            return CMD_OK;

        case Y_REGISTER:
            PCWrite(0xee);
            return CMD_OK;

        case PSW_REGISTER:
            PCWrite(0x8e);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus BBCx(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode1, mode2;
    int addr1, addr2;
    int bit;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    bit = argv[0][strlen(argv[0]) - 1] - '0';

    if (!MakeRelative(&addr2, argv[0], err, errsize))
    {
        return CMD_FAILED;
    }

    switch(mode1)
    {
        case ABSOLUTE:
        case DIRECT_PAGE:
            PCWrite(0x13 + bit * 0x20);
            PCWrite(addr1);
            PCWrite(addr2);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode1]);
            return CMD_FAILED;
    }
}

static CommandStatus BBSx(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode1, mode2;
    int addr1, addr2;
    int bit;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    bit = argv[0][strlen(argv[0]) - 1] - '0';

    if (!MakeRelative(&addr2, argv[0], err, errsize))
    {
        return CMD_FAILED;
    }

    switch(mode1)
    {
        case ABSOLUTE:
        case DIRECT_PAGE:
            PCWrite(0x03 + bit * 0x20);
            PCWrite(addr1);
            PCWrite(addr2);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode1]);
            return CMD_FAILED;
    }
}

static CommandStatus CBNE(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (!MakeRelative(&addr2, argv[0], err, errsize))
    {
        return CMD_FAILED;
    }

    switch(mode1)
    {
        case ABSOLUTE:
        case DIRECT_PAGE:
            PCWrite(0x2e);
            PCWrite(addr1);
            PCWrite(addr2);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0xde);
            PCWrite(addr1);
            PCWrite(addr2);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode1]);
            return CMD_FAILED;
    }
}

static CommandStatus DBNZ(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (!MakeRelative(&addr2, argv[0], err, errsize))
    {
        return CMD_FAILED;
    }

    switch(mode1)
    {
        case ABSOLUTE:
        case DIRECT_PAGE:
            PCWrite(0x6e);
            PCWrite(addr1);
            PCWrite(addr2);
            return CMD_OK;

        case Y_REGISTER:
            PCWrite(0xfe);
            PCWrite(addr2);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode1]);
            return CMD_FAILED;
    }
}

static CommandStatus SETx(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;
    int bit;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    bit = argv[0][strlen(argv[0]) - 1] - '0';

    switch(mode)
    {
        case ABSOLUTE:
        case DIRECT_PAGE:
            PCWrite(0x12 + bit * 0x20);
            PCWrite(addr);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus CLRx(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;
    int bit;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    bit = argv[0][strlen(argv[0]) - 1] - '0';

    switch(mode)
    {
        case ABSOLUTE:
        case DIRECT_PAGE:
            PCWrite(0x02 + bit * 0x20);
            PCWrite(addr);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus TSET1(const char *label, int argc, char *argv[],
                           int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case DIRECT_PAGE:
        case ABSOLUTE:
            PCWrite(0x0e);
            PCWriteWord(addr);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus TCLR1(const char *label, int argc, char *argv[],
                           int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    switch(mode)
    {
        case ABSOLUTE:
        case DIRECT_PAGE:
            PCWrite(0xe4);
            PCWriteWord(addr);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus AND1(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (mode1 == C_FLAG)
    {
        switch(mode2)
        {
            case BIT_ADDRESS:
                PCWrite(0x4a);
                PCWriteWord(addr2);
                return CMD_OK;

            case NOTTED_BIT_ADDRESS:
                PCWrite(0x6a);
                PCWriteWord(addr2);
                return CMD_OK;

            default:
                break;
        }
    }

    snprintf(err, errsize, "%s: unsupported addressing mode %s, %s",
                                    argv[0], address_mode_name[mode1],
                                             address_mode_name[mode2]);
    return CMD_FAILED;
}

static CommandStatus OR1(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (mode1 == C_FLAG)
    {
        switch(mode2)
        {
            case BIT_ADDRESS:
                PCWrite(0x0a);
                PCWriteWord(addr2);
                return CMD_OK;

            case NOTTED_BIT_ADDRESS:
                PCWrite(0x2a);
                PCWriteWord(addr2);
                return CMD_OK;

            default:
                break;
        }
    }

    snprintf(err, errsize, "%s: unsupported addressing mode %s, %s",
                                    argv[0], address_mode_name[mode1],
                                             address_mode_name[mode2]);
    return CMD_FAILED;
}

static CommandStatus EOR1(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (mode1 == C_FLAG)
    {
        switch(mode2)
        {
            case BIT_ADDRESS:
                PCWrite(0x8a);
                PCWriteWord(addr2);
                return CMD_OK;

            default:
                break;
        }
    }

    snprintf(err, errsize, "%s: unsupported addressing mode %s, %s",
                                    argv[0], address_mode_name[mode1],
                                             address_mode_name[mode2]);
    return CMD_FAILED;
}

static CommandStatus NOT1(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    int addr;

    CMD_ARGC_CHECK(2);

    ADDRESS_MODE(mode, addr, 1);

    if (mode == BIT_ADDRESS)
    {
        PCWrite(0xea);
        PCWriteWord(addr);
        return CMD_OK;
    }

    snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                    argv[0], address_mode_name[mode]);
    return CMD_FAILED;
}

static CommandStatus MOV1(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode1, mode2;
    int addr1, addr2;

    CMD_ARGC_CHECK(3);

    ADDRESS_MODE(mode1, addr1, 1);
    ADDRESS_MODE(mode2, addr2, 2);

    if (mode1 == C_FLAG && mode2 == BIT_ADDRESS)
    {
        PCWrite(0xaa);
        PCWriteWord(addr2);
        return CMD_OK;
    }

    if (mode2 == C_FLAG && mode1 == BIT_ADDRESS)
    {
        PCWrite(0xca);
        PCWriteWord(addr2);
        return CMD_OK;
    }

    snprintf(err, errsize, "%s: unsupported addressing mode %s, %s",
                                    argv[0], address_mode_name[mode1],
                                             address_mode_name[mode2]);
    return CMD_FAILED;
}

/* ---------------------------------------- OPCODE TABLES
*/
typedef struct
{
    const char  *op;
    int         code;
} OpcodeTable;

typedef struct
{
    const char  *op;
    Command     cmd;
} HandlerTable;

static const OpcodeTable implied_opcodes[] =
{
    {"NOP",     0x00},
    {"SLEEP",   0xef},
    {"STOP",    0xff},
    {"CLRC",    0x60},
    {"SETC",    0x80},
    {"NOTC",    0xed},
    {"CLRV",    0xe0},
    {"CLRP",    0x20},
    {"SETP",    0x40},
    {"EI",      0xa0},
    {"DI",      0xc0},
    {"BRK",     0x0f},
    {"RET",     0x6f},
    {"RETI",    0x7f},
    {NULL}
};


static const OpcodeTable branch_opcodes[] =
{
    {"BRA",	0x2f},
    {"BEQ",	0xF0},
    {"BNE",	0xD0},
    {"BCS",	0xB0},
    {"BVC",	0x50},
    {"BCC",	0x90},
    {"BVS",	0x70},
    {"BVC",	0x50},
    {"BMI",	0x30},
    {"BPL",	0x10},
    {NULL}
};


static const HandlerTable handler_table[] =
{
    {"ADC",     ADC},
    {"ADDW",    ADDW},
    {"AND",     AND},
    {"AND1",    AND1},
    {"ASL",     ASL},
    {"BBC0",    BBCx},
    {"BBC1",    BBCx},
    {"BBC2",    BBCx},
    {"BBC3",    BBCx},
    {"BBC4",    BBCx},
    {"BBC5",    BBCx},
    {"BBC6",    BBCx},
    {"BBC7",    BBCx},
    {"BBS0",    BBSx},
    {"BBS1",    BBSx},
    {"BBS2",    BBSx},
    {"BBS3",    BBSx},
    {"BBS4",    BBSx},
    {"BBS5",    BBSx},
    {"BBS6",    BBSx},
    {"BBS7",    BBSx},
    {"CALL",    CALL},
    {"CBNE",    CBNE},
    {"CLR0",    CLRx},
    {"CLR1",    CLRx},
    {"CLR2",    CLRx},
    {"CLR3",    CLRx},
    {"CLR4",    CLRx},
    {"CLR5",    CLRx},
    {"CLR6",    CLRx},
    {"CLR7",    CLRx},
    {"CMP",     CMP},
    {"CMPW",    CMPW},
    {"DAA",     DAA},
    {"DAS",     DAS},
    {"DBNZ",    DBNZ},
    {"DEC",     DEC},
    {"DECW",    DECW},
    {"DIV",     DIV},
    {"EOR",     EOR},
    {"EOR1",    EOR1},
    {"INC",     INC},
    {"INCW",    INCW},
    {"JMP",     JMP},
    {"LSR",     LSR},
    {"MOV",     MOV},
    {"MOV1",    MOV1},
    {"MOVW",    MOVW},
    {"MUL",     MUL},
    {"NOT1",    NOT1},
    {"OR",      OR},
    {"OR1",     OR1},
    {"PCALL",   PCALL},
    {"POP",     POP},
    {"PUSH",    PUSH},
    {"ROL",     ROL},
    {"ROR",     ROR},
    {"SBC",     SBC},
    {"SET0",    SETx},
    {"SET1",    SETx},
    {"SET2",    SETx},
    {"SET3",    SETx},
    {"SET4",    SETx},
    {"SET5",    SETx},
    {"SET6",    SETx},
    {"SET7",    SETx},
    {"SUBW",    SUBW},
    {"TCALL",   TCALL},
    {"TCLR1",   TCLR1},
    {"TSET1",   TSET1},
    {"XCN",     XCN},
    {NULL}
};




/* ---------------------------------------- PUBLIC FUNCTIONS
*/

void Init_SPC700(void)
{
    option.dp_mode = DP_AUTO;
    SetNeededPasses(3);
}


const ValueTable *Options_SPC700(void)
{
    return options;
}

CommandStatus SetOption_SPC700(int opt, int argc, char *argv[],
                               int quoted[], char *err, size_t errsize)
{
    const ValueTable *val;

    switch(opt)
    {
        case OPT_DP:
            CMD_ARGC_CHECK(1);
            CMD_TABLE(argv[0], dp_table, val);

            option.dp_mode = val->value;
            break;

        default:
            break;
    }

    return CMD_OK;
}

CommandStatus Handler_SPC700(const char *label, int argc, char *argv[],       
                             int quoted[], char *err, size_t errsize)
{
    int f;

    /* Check for simple (implied addressing) opcodes
    */
    for(f = 0; implied_opcodes[f].op; f++)
    {
        if (CompareString(argv[0], implied_opcodes[f].op))
        {
            PCWrite(implied_opcodes[f].code);
            return CMD_OK;
        }
    }

    /* Check for branch opcodes
    */
    for(f = 0; branch_opcodes[f].op; f++)
    {
        if (CompareString(argv[0], branch_opcodes[f].op))
        {
            int offset;

            CMD_ARGC_CHECK(2);

            CMD_EXPR(argv[1], offset);

            if (!MakeRelative(&offset, argv[0], err, errsize))
            {
                return CMD_FAILED;
            }

            PCWrite(branch_opcodes[f].code);
            PCWrite(offset);

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
