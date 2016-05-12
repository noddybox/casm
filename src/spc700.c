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
    OPT_ZP
};

enum zp_mode_t
{
    ZP_OFF,
    ZP_ON,
    ZP_AUTO
};

static const ValueTable options[] =
{
    {"zero-page",       OPT_ZP},
    {NULL}
};

static const ValueTable zp_table[] =
{
    YES_NO_ENTRIES(ZP_ON, ZP_OFF),
    {"auto", ZP_AUTO},
    {NULL}
};

static struct
{
    enum zp_mode_t zp_mode;
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
    NOTTED_BIT,
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
    "Notted (/) bit",
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


#define CMD_ZP_MODE(ZP_mode, non_ZP_mode)                               \
do                                                                      \
{                                                                       \
    switch(option.zp_mode)                                              \
    {                                                                   \
        case ZP_ON:                                                     \
            if (*address < 0 || *address > 255)                         \
            {                                                           \
                snprintf(err, errsize, "value %d outside of "           \
                                            "zero page", *address);     \
                *mode = ADDR_MODE_ERROR;                                \
                return;                                                 \
            }                                                           \
                                                                        \
            *mode = ZP_mode;                                            \
            break;                                                      \
                                                                        \
        case ZP_OFF:                                                    \
            *mode = non_ZP_mode;                                        \
            break;                                                      \
                                                                        \
        case ZP_AUTO:                                                   \
            if (*address >= 0 && *address <= 255)                       \
            {                                                           \
                *mode = ZP_mode;                                        \
            }                                                           \
            else                                                        \
            {                                                           \
                *mode = non_ZP_mode;                                    \
            }                                                           \
            break;                                                      \
    }                                                                   \
} while (0)


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

    /* 'Notted' bit
    */
    if (arg[0] == '/')
    {
        *mode = NOTTED_BIT;

        if (!ExprEval(arg + 1, address))
        {
            snprintf(err, errsize, "%s: expression error: %s",
                                        arg + 1, ExprError());
            *mode = ADDR_MODE_ERROR;
        }

        return;
    }

    /* Immediate
    */
    if (arg[0] == '#')
    {
        *mode = IMMEDIATE;

        if (!ExprEval(arg + 1, address))
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
            if (!ExprEval(copy, address))
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
            if (!ExprEval(copy + 1, address))
            {
                snprintf(err, errsize, "%s: expression error: %s",
                                            copy + 1, ExprError());
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
            if (!ExprEval(copy, address))
            {
                snprintf(err, errsize, "%s: expression error: %s",
                                            copy, ExprError());
                *mode = ADDR_MODE_ERROR;
            }
            else
            {
                CMD_ZP_MODE(DIRECT_PAGE_INDEX_X, ABSOLUTE_INDEX_X);
            }

            free(copy);
            return;
        }

        /* Direct page or absolute index Y
        */
        if (!quote && CompareString(end, "Y"))
        {
            if (!ExprEval(copy, address))
            {
                snprintf(err, errsize, "%s: expression error: %s",
                                            copy, ExprError());
                *mode = ADDR_MODE_ERROR;
            }
            else
            {
                CMD_ZP_MODE(DIRECT_PAGE_INDEX_Y, ABSOLUTE_INDEX_Y);
            }

            free(copy);
            return;
        }

        free(copy);
    }

    /* If all else fails, Absolute
    */
    if (!ExprEval(arg, address))
    {
        snprintf(err, errsize, "%s: expression error:  %s",
                                    arg, ExprError());
        *mode = ADDR_MODE_ERROR;
        return;
    }

    CMD_ZP_MODE(DIRECT_PAGE, ABSOLUTE);
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
        {X_REGISTER,            SP_REGISTER,            {0xbd}},
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
    {"RET1",    0x7f},
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
    {"ADC",	ADC},
    {"AND",	AND},
    {"ASL",	ASL},
    {"CMP",	CMP},
    {"DEC",	DEC},
    {"EOR",	EOR},
    {"INC",	INC},
    {"JMP",	JMP},
    {"CALL",	CALL},
    {"PCALL",	PCALL},
    {"LSR",	LSR},
    {"OR",	OR},
    {"ROL",	ROL},
    {"ROR",	ROR},
    {"SBC",	SBC},
    {"MOV",	MOV},
    {"MOVW",	MOVW},
    {NULL}
};




/* ---------------------------------------- PUBLIC FUNCTIONS
*/

void Init_SPC700(void)
{
    option.zp_mode = ZP_AUTO;
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
        case OPT_ZP:
            CMD_ARGC_CHECK(1);
            CMD_TABLE(argv[0], zp_table, val);

            option.zp_mode = val->value;
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

            offset = offset - (PC() + 2);

            if (IsFinalPass() && (offset < -128 || offset > 127))
            {
                snprintf(err, errsize, "%s: Branch offset (%d) too big",
                                            argv[1], offset);
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
