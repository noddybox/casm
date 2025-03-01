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

    6502 Assembler

*/
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "expr.h"
#include "label.h"
#include "parse.h"
#include "cmd.h"
#include "codepage.h"

#include "6502.h"


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
    IMPLIED,
    IMMEDIATE,
    ABSOLUTE,
    ZERO_PAGE,
    ABSOLUTE_INDEX_X,
    ABSOLUTE_INDEX_Y,
    ZERO_PAGE_INDEX_X,
    ZERO_PAGE_INDEX_Y,
    ZERO_PAGE_INDIRECT_X,
    ZERO_PAGE_INDIRECT_Y,
    INDIRECT,
    ADDR_MODE_ERROR,
    ADDR_MODE_UNKNOWN
} address_mode_t;

static const char *address_mode_name[] =
{
    "Accumulator",
    "Implied",
    "Immediate",
    "Absolute",
    "Zero Page",
    "Absolute, index X",
    "Absolute, index Y",
    "Zero Page, index X",
    "Zero Page, index Y",
    "Zero Page, indirect X",
    "Zero Page, indirect Y",
    "Indirect",
    "Address Mode Error",
    "Address Mode Unknown"
};


/* ---------------------------------------- MACROS
*/
#define CMD_ADDRESS_MODE(mode, address)                                 \
do                                                                      \
{                                                                       \
    CalcAddressMode(argc, argv, quoted, err, errsize, &mode, &address); \
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
static void CalcAddressMode(int argc, char *argv[], int quoted[],
                            char *err, size_t errsize,
                            address_mode_t *mode, long *address)
{
    *mode = ADDR_MODE_UNKNOWN;
    *address = 0;

    /* Implied
    */
    if (argc == 1)
    {
        *mode = IMPLIED;
        return;
    }

    /* Accumulator
    */
    if (argc == 2 && CompareString(argv[1], "A"))
    {
        *mode = ACCUMULATOR;
        return;
    }

    /* Immediate
    */
    if (argc == 2 && argv[1][0] == '#')
    {
        *mode = IMMEDIATE;

        if (!ExprEval(argv[1] + 1, address))
        {
            snprintf(err, errsize, "%s: expression error: %s",
                                        argv[1], ExprError());
            *mode = ADDR_MODE_ERROR;
        }

        return;
    }


    /* Absolute
    */
    if (argc == 2 && !quoted[1])
    {
        if (!ExprEval(argv[1], address))
        {
            snprintf(err, errsize, "%s: expression error:  %s",
                                        argv[1], ExprError());
            *mode = ADDR_MODE_ERROR;
            return;
        }

        CMD_ZP_MODE(ZERO_PAGE, ABSOLUTE);

        return;
    }


    /* Absolute,[XY]
    */
    if (argc == 3 && !quoted[1])
    {
        if (!ExprEval(argv[1], address))
        {
            snprintf(err, errsize, "%s: expression error:  %s",
                                        argv[1], ExprError());
            *mode = ADDR_MODE_ERROR;
            return;
        }

        if (CompareString(argv[2], "X"))
        {
            CMD_ZP_MODE(ZERO_PAGE_INDEX_X, ABSOLUTE_INDEX_X);
        }
        else if (CompareString(argv[2], "Y"))
        {
            CMD_ZP_MODE(ZERO_PAGE_INDEX_Y, ABSOLUTE_INDEX_Y);
        }
        else
        {
            snprintf(err, errsize, "unknown index register '%s'", argv[2]);
            *mode = ADDR_MODE_ERROR;
            return;
        }

        return;
    }


    /* (zp,x) or (ind)
    */
    if (argc == 2 && quoted[1] == '(')
    {
        char *addr;

        if (!CompareEnd(argv[1], ",x"))
        {
            if (!ExprEval(argv[1], address))
            {
                snprintf(err, errsize, "%s: expression error:  %s",
                                            argv[1], ExprError());
                *mode = ADDR_MODE_ERROR;
                return;
            }

            *mode = INDIRECT;
            return;
        }

        *mode = ZERO_PAGE_INDIRECT_X;

        addr = DupStr(argv[1]);
        *strchr(addr, ',') = 0;

        if (!ExprEval(addr, address))
        {
            snprintf(err, errsize, "%s: expression error:  %s",
                                        addr, ExprError());
            *mode = ADDR_MODE_ERROR;
            free(addr);
            return;
        }

        free(addr);

        if (*address < 0 || *address > 255)
        {
            snprintf(err, errsize, "value %d outside of zero page", *address);
            *mode = ADDR_MODE_ERROR;
            return;
        }

        return;
    }


    /* (zp),y
    */
    if (argc == 3 && quoted[1] == '(')
    {
        *mode = ZERO_PAGE_INDIRECT_Y;

        if (!CompareString(argv[2], "y"))
        {
            snprintf(err, errsize, "illegal index register '%s' used for "
                                        "zero-page indirect", argv[2]);
            *mode = ADDR_MODE_ERROR;
            return;
        }

        if (!ExprEval(argv[1], address))
        {
            snprintf(err, errsize, "%s: expression error:  %s",
                                        argv[1], ExprError());
            *mode = ADDR_MODE_ERROR;
            return;
        }

        if (*address < 0 || *address > 255)
        {
            snprintf(err, errsize, "value %d outside of zero page", *address);
            *mode = ADDR_MODE_ERROR;
            return;
        }

        return;
    }
}



/* ---------------------------------------- COMMAND HANDLERS - LEGAL OPCODES
*/

static CommandStatus ADC(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0x69);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x6d);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x65);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x7d);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0x79);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x75);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0x61);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0x71);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus AND(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0x29);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x2d);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x25);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x3d);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0x39);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x35);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0x21);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0x31);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus ASL(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMPLIED:
        case ACCUMULATOR:
            PCWrite(0x0a);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x0e);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x06);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x1e);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x16);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus BIT(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0x2c);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x24);
            PCWrite(address);
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
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0xc9);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xcd);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0xc5);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0xdd);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0xd9);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0xd5);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0xc1);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0xd1);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus CPX(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0xe0);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xec);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0xe4);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus CPY(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0xc0);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xcc);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0xc4);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus DEC(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0xce);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0xc6);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0xde);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0xd6);
            PCWrite(address);
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
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0x49);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x4d);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x45);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x5d);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0x59);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x55);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0x41);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0x51);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus INC(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0xee);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0xe6);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0xfe);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0xf6);
            PCWrite(address);
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
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
        case ZERO_PAGE:
            PCWrite(0x4c);
            PCWriteWord(address);
            return CMD_OK;

        case INDIRECT:
            PCWrite(0x6c);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus JSR(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
        case ZERO_PAGE:
            PCWrite(0x20);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus LDA(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0xa9);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xad);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0xa5);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0xbd);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0xb9);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0xb5);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0xa1);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0xb1);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus LDX(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0xa2);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xae);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0xa6);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
            PCWrite(0xbe);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_Y:
            PCWrite(0xb6);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus LDY(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0xa0);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xac);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0xa4);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0xbc);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0xb4);
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
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMPLIED:
        case ACCUMULATOR:
            PCWrite(0x4a);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x4e);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x46);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x5e);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x56);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus ORA(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0x09);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x0d);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x05);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x1d);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0x19);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x15);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0x01);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0x11);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus ROL(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMPLIED:
        case ACCUMULATOR:
            PCWrite(0x2a);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x2e);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x26);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x3e);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x36);
            PCWrite(address);
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
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMPLIED:
        case ACCUMULATOR:
            PCWrite(0x6a);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x6e);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x66);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x7e);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x76);
            PCWrite(address);
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
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0xe9);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xed);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0xe5);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0xfd);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0xf9);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0xf5);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0xe1);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0xf1);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus STA(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0x8d);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x85);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x9d);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0x99);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x95);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0x81);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0x91);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus STX(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0x8e);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x86);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
            if (address < 0 || address > 255)
            {
                snprintf(err, errsize, "%s: value %d outside of zero page",
                                                            argv[0], address);
                return CMD_FAILED;
            }

            PCWrite(0x96);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_Y:
            PCWrite(0x96);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus STY(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0x8c);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x84);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            if (address < 0 || address > 255)
            {
                snprintf(err, errsize, "%s: value %d outside of zero page",
                                                            argv[0], address);
                return CMD_FAILED;
            }

            PCWrite(0x94);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x94);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

/* ---------------------------------------- HANDLERS - UNDOCUMENTED OPCODES
*/
static CommandStatus ALR(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0x4b);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus ANC(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0x0b);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus ANC2(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0x2b);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus ANE(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0x8b);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus ARR(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0x6b);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus DCP(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0xcf);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0xc7);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0xdf);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0xdb);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0xd7);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0xc3);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0xd3);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus ISC(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0xef);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0xe7);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0xff);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0xfb);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0xf7);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0xe3);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0xf3);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus LAS(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0xbb);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus LAX(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0xaf);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0xa7);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_Y:
            PCWrite(0xb7);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
            PCWrite(0xbf);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0xa3);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0xb3);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus LXA(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0xab);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus RLA(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0x2f);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x27);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x3f);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0x3b);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x37);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0x23);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0x33);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus RRA(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0x6f);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x67);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x7f);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0x7b);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x77);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0x63);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0x73);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus SAX(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0x8f);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x87);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_Y:
            PCWrite(0x97);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0x83);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus SBX(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0xcb);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus SHA(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE_INDEX_Y:
            PCWrite(0x9f);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0x93);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus SHX(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE_INDEX_Y:
            PCWrite(0x9e);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus SHY(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE_INDEX_X:
            PCWrite(0x9c);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus SLO(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0x0f);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x07);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x1f);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0x1b);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x17);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0x03);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0x13);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus SRE(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE:
            PCWrite(0x4f);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE:
            PCWrite(0x47);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x5f);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0x5b);
            PCWriteWord(address);
            return CMD_OK;

        case ZERO_PAGE_INDEX_X:
            PCWrite(0x57);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_X:
            PCWrite(0x43);
            PCWrite(address);
            return CMD_OK;

        case ZERO_PAGE_INDIRECT_Y:
            PCWrite(0x53);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus TAS(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case ABSOLUTE_INDEX_Y:
        case ZERO_PAGE_INDEX_Y:
            PCWrite(0x9b);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus USBC(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMMEDIATE:
            PCWrite(0xeb);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus JAM(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    PCWrite(0x02);
    return CMD_OK;
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
    {"NOP",      0xea},
    {"TXS",      0x9a},
    {"TSX",      0xba},
    {"PHA",      0x48},
    {"PLA",      0x68},
    {"PHP",      0x08},
    {"PLP",      0x28},
    {"CLC",      0x18},
    {"SEC",      0x38},
    {"CLI",      0x58},
    {"SEI",      0x78},
    {"CLV",      0xb8},
    {"CLD",      0xd8},
    {"SED",      0xf8},
    {"BRK",      0x00},
    {"TAX",      0xaa},
    {"TXA",      0x8a},
    {"DEX",      0xca},
    {"INX",      0xe8},
    {"TAY",      0xa8},
    {"TYA",      0x98},
    {"DEY",      0x88},
    {"INY",      0xc8},
    {"RTI",      0x40},
    {"RTS",      0x60},
    {NULL}
};


static const OpcodeTable branch_opcodes[] =
{
    {"BPL",	0x10},
    {"BMI",	0x30},
    {"BVC",	0x50},
    {"BVS",	0x70},
    {"BCC",	0x90},
    {"BCS",	0xB0},
    {"BNE",	0xD0},
    {"BEQ",	0xF0},
    {NULL}
};


static const HandlerTable handler_table[] =
{
    {"ADC",	ADC},
    {"AND",	AND},
    {"ASL",	ASL},
    {"BIT",	BIT},
    {"CMP",	CMP},
    {"CPX",	CPX},
    {"CPY",	CPY},
    {"DEC",	DEC},
    {"EOR",	EOR},
    {"INC",	INC},
    {"JMP",	JMP},
    {"JSR",	JSR},
    {"LDA",	LDA},
    {"LDX",	LDX},
    {"LDY",	LDY},
    {"LSR",	LSR},
    {"ORA",	ORA},
    {"ROL",	ROL},
    {"ROR",	ROR},
    {"SBC",	SBC},
    {"STA",	STA},
    {"STX",	STX},
    {"STY",	STY},
    {NULL}
};


static const HandlerTable undocumented_handler_table[] =
{
    {"ALR",	ALR},
    {"ASR",	ALR},
    {"ANC",	ANC},
    {"ANC2",	ANC2},
    {"ANE",	ANE},
    {"XAA",	ANE},
    {"ARR",	ARR},
    {"DCP",	DCP},
    {"DCM",	DCP},
    {"ISC",	ISC},
    {"ISB",	ISC},
    {"INS",	ISC},
    {"LAS",	LAS},
    {"LAR",	LAS},
    {"LAX",	LAX},
    {"LXA",	LXA},
    {"RLA",	RLA},
    {"RRA",	RRA},
    {"SAX",	SAX},
    {"AXS",	SAX},
    {"AAX",	SAX},
    {"SBX",	SBX},
    {"ASX",	SBX},
    {"SAX",	SBX},
    {"SHA",	SHA},
    {"AHX",	SHA},
    {"AXA",	SHA},
    {"SHX",	SHX},
    {"SXA",	SHX},
    {"XAS",	SHX},
    {"SHY",	SHY},
    {"SYA",	SHY},
    {"SAY",	SHY},
    {"SLO",	SLO},
    {"ASO",	SLO},
    {"SRE",	SRE},
    {"TAS",	TAS},
    {"XAS",	TAS},
    {"SHS",	TAS},
    {"USBC",	USBC},
    {"SBC",	USBC},
    {"JAM",	JAM},
    {"KIL",	JAM},
    {"HLT",	JAM},
    {NULL}
};




/* ---------------------------------------- PUBLIC FUNCTIONS
*/

void Init_6502(void)
{
    option.zp_mode = ZP_AUTO;
    SetNeededPasses(3);
}


const ValueTable *Options_6502(void)
{
    return options;
}

CommandStatus SetOption_6502(int opt, int argc, char *argv[],
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

CommandStatus Handler_6502(const char *label, int argc, char *argv[],       
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
            long offset;

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


    /* Check for legal opcodes
    */
    for(f = 0; handler_table[f].op; f++)
    {
        if (CompareString(argv[0], handler_table[f].op))
        {
            return handler_table[f].cmd(label, argc, argv,
                                        quoted, err, errsize);;
        }
    }


    /* Check for undocumented opcodes
    */
    for(f = 0; undocumented_handler_table[f].op; f++)
    {
        if (CompareString(argv[0], undocumented_handler_table[f].op))
        {
            return undocumented_handler_table[f].cmd(label, argc, argv,
                                                     quoted, err, errsize);;
        }
    }


    return CMD_NOT_KNOWN;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
