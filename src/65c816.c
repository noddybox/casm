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

    65c816 Assembler

*/
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "expr.h"
#include "label.h"
#include "parse.h"
#include "cmd.h"
#include "codepage.h"

#include "65c816.h"


/* ---------------------------------------- TYPES AND GLOBALS
*/
enum option_t
{
    OPT_A16,
    OPT_I16
};

static const ValueTable options[] =
{
    {"a16",     OPT_A16},
    {"i16",     OPT_I16},
    {NULL}
};

static struct
{
    int         a16;
    int         i16;
} option;

/* Note some addressing modes are indistinguable and will never be returned,
   for example STACK_IMMEDIATE or STACK_PC_LONG.  They are kept here as a
   memory aid.
*/
typedef enum
{
    ACCUMULATOR,                        /* A            */
    IMPLIED,                            /* none         */
    IMMEDIATE,                          /* #$nn         */
    ABSOLUTE,                           /* $nnnn        */
    ABSOLUTE_INDEX_X_INDIRECT,          /* ($nnnn,X)    */
    ABSOLUTE_INDEX_X,                   /* $nnnn,X      */
    ABSOLUTE_INDEX_Y,                   /* $nnnn,Y      */
    ABSOLUTE_INDIRECT,                  /* ($nnnn)      */
    ABSOLUTE_INDIRECT_LONG,             /* [$nnnn]      */
    ABSOLUTE_LONG,                      /* $nnnnnn      */
    ABSOLUTE_LONG_INDEX_X,              /* $nnnnnn,X    */
    DIRECT_PAGE,                        /* $nn          */
    DIRECT_PAGE_INDEX_X,                /* $nn,X        */
    DIRECT_PAGE_INDEX_Y,                /* $nn,Y        */
    DIRECT_PAGE_INDIRECT,               /* ($nn)        */
    DIRECT_PAGE_INDIRECT_LONG,          /* [$nn]        */
    DIRECT_PAGE_INDEX_X_INDIRECT,       /* ($nn,X)      */
    DIRECT_PAGE_INDEX_Y_INDIRECT,       /* ($nn),Y      */
    DIRECT_PAGE_INDEX_Y_INDIRECT_LONG,  /* [$nn],Y      */
    RELATIVE,                           /* rr           */
    RELATIVE_LONG,                      /* rrrr         */
    STACK_RELATIVE_INDIRECT_INDEX_Y,    /* (sr,S),Y     */
    STACK_RELATIVE,                     /* sr,S         */
    STACK_IMMEDIATE,                    /* #$nnnn       */
    STACK_DIRECT_PAGE_INDIRECT,         /* ($nn)        */
    STACK_PC_LONG,                      /* #$nnnn       */
    ADDR_MODE_ERROR,
    ADDR_MODE_UNKNOWN
} address_mode_t;

static const char *address_mode_name[ADDR_MODE_UNKNOWN+1] =
{
    "Accumulator",
    "Implied",
    "Immediate",
    "Absolute",
    "Absolute Index X, Indirect",
    "Absolute, Index X",
    "Absolute, Index Y",
    "Absolute Indirect",
    "Absolute Indirect Long",
    "Absolute Long",
    "Absolute Long, Index X",
    "Direct Page",
    "Direct Page, Index X",
    "Direct Page, Index Y",
    "Direct Page Indirect",
    "Direct Page Indirect Long",
    "Direct Page Index X, Indirect",
    "Direct Page, Indirect, Index Y",
    "Direct Page, Indirect, Index Y Long",
    "Relative",
    "Relative Long",
    "Stack Relative Indirect Index Y",
    "Stack Relative",
    "Stack Immediate",
    "Stack Direct Page Indirect",
    "Stack Program Counter Relative Long",
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


#define CMD_RANGE_ADDR_MODE(mode, dp_mode, norm_mode, long_mode, value) \
do                                                                      \
{                                                                       \
    if (*value >= 0 && *value <= 0xff)                                  \
    {                                                                   \
        *mode = dp_mode;                                                \
    }                                                                   \
    else if (*value > 0xffff)                                           \
    {                                                                   \
        *mode = long_mode;                                              \
    }                                                                   \
    else                                                                \
    {                                                                   \
        *mode = norm_mode;                                              \
    }                                                                   \
    if (*mode == ADDR_MODE_ERROR && IsFinalPass())                      \
    {                                                                   \
        snprintf(err, errsize, "%s: value %d out of range of allowable "\
                                "addressing modes", argv[1], *value);   \
    }                                                                   \
} while (0)


/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static void PCWrite24(int val)
{
    val = ExprConvert(24, val);
    PCWrite(val);
    PCWriteWord(val >> 8);
}


static void PCWrite8_16_A(int val)
{
    if (option.a16)
    {
        PCWriteWord(val);
    }
    else
    {
        PCWrite(val);
    }
}


static void PCWrite8_16_XY(int val)
{
    if (option.i16)
    {
        PCWriteWord(val);
    }
    else
    {
        PCWrite(val);
    }
}


static char *HasIndex(const char *str, char *index_char)
{
    char *comma;

    comma = strchr(str, ',');
    
    if (comma)
    {
        char *a;

        a = DupStr(str);
        comma = strchr(a, ',');

        *comma++ = 0;
        *index_char = *comma;

        return a;
    }
    else
    {
        return NULL;
    }
}

static void CalcAddressMode(int argc, char *argv[], int quoted[],
                            char *err, size_t errsize,
                            address_mode_t *mode, long *address)
{
    *mode = ADDR_MODE_UNKNOWN;
    *address = 0;

    /* Handle: IMPLIED
    */
    if (argc == 1)
    {
        *mode = IMPLIED;
        return;
    }

    /* Handle: ACCUMULATOR
    */
    if (argc == 2 && CompareString(argv[1], "A"))
    {
        *mode = ACCUMULATOR;
        return;
    }

    /* Handle: IMMEDIATE
       Handle: STACK_IMMEDIATE
       Handle: STACK_PC_LONG
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


    /* Handle: ABSOLUTE
       Handle: ABSOLUTE_LONG
       Handle: DIRECT_PAGE
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

        CMD_RANGE_ADDR_MODE(mode,
                            DIRECT_PAGE,
                            ABSOLUTE,
                            ABSOLUTE_LONG,
                            address);

        return;
    }


    /* Handle: ABSOLUTE_INDIRECT
       Handle: DIRECT_PAGE_INDIRECT
       Handle: STACK_DIRECT_PAGE_INDIRECT
       Handle: ABSOLUTE_INDEX_X_INDIRECT
       Handle: DIRECT_PAGE_INDEX_X_INDIRECT
    */
    if (argc == 2 && quoted[1] == '(')
    {
        char *new;
        char index;

        if (!(new = HasIndex(argv[1], &index)))
        {
            if (!ExprEval(argv[1], address))
            {
                snprintf(err, errsize, "%s: expression error:  %s",
                                            argv[1], ExprError());
                *mode = ADDR_MODE_ERROR;
                return;
            }

            CMD_RANGE_ADDR_MODE(mode,
                                DIRECT_PAGE_INDIRECT,
                                ABSOLUTE_INDIRECT,
                                ADDR_MODE_ERROR,
                                address);

            return;
        }

        if (!CompareChar(index, 'x'))
        {
            snprintf(err, errsize, "illegal index register '%s'", argv[1]);
            *mode = ADDR_MODE_ERROR;
            free(new);
            return;
        }

        if (!ExprEval(new, address))
        {
            snprintf(err, errsize, "%s: expression error:  %s",
                                        new, ExprError());
            *mode = ADDR_MODE_ERROR;
            free(new);
            return;
        }

        free(new);

        CMD_RANGE_ADDR_MODE(mode,
                            DIRECT_PAGE_INDEX_X_INDIRECT,
                            ABSOLUTE_INDEX_X_INDIRECT,
                            ADDR_MODE_ERROR,
                            address);

        return;
    }


    /* Handle: ABSOLUTE_INDIRECT_LONG
       Handle: DIRECT_PAGE_INDIRECT_LONG
    */
    if (argc == 2 && quoted[1] == '[')
    {
        if (!ExprEval(argv[1], address))
        {
            snprintf(err, errsize, "%s: expression error:  %s",
                                        argv[1], ExprError());
            *mode = ADDR_MODE_ERROR;
            return;
        }

        CMD_RANGE_ADDR_MODE(mode,
                            DIRECT_PAGE_INDIRECT_LONG,
                            ABSOLUTE_INDIRECT_LONG,
                            ADDR_MODE_ERROR,
                            address);

        return;
    }


    /* Handle: DIRECT_PAGE_INDEX_X
       Handle: ABSOLUTE_INDEX_X
       Handle: ABSOLUTE_LONG_INDEX_X
       Handle: DIRECT_PAGE_INDEX_Y
       Handle: ABSOLUTE_INDEX_Y
       Handle: STACK_RELATIVE
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
            CMD_RANGE_ADDR_MODE(mode,
                                DIRECT_PAGE_INDEX_X,
                                ABSOLUTE_INDEX_X,
                                ABSOLUTE_LONG_INDEX_X,
                                address);
        }
        else if (CompareString(argv[2], "Y"))
        {
            CMD_RANGE_ADDR_MODE(mode,
                                DIRECT_PAGE_INDEX_Y,
                                ABSOLUTE_INDEX_Y,
                                ADDR_MODE_ERROR,
                                address);
        }
        else if (CompareString(argv[2], "S"))
        {
            *mode = STACK_RELATIVE;
        }
        else
        {
            snprintf(err, errsize, "unknown index register '%s'", argv[2]);
            *mode = ADDR_MODE_ERROR;
            return;
        }

        return;
    }


    /* Handle: DIRECT_PAGE_INDEX_Y_INDIRECT
       Handle: DIRECT_PAGE_INDEX_Y_INDIRECT_LONG
       Handle: STACK_RELATIVE_INDIRECT_INDEX_Y
    */
    if (argc == 3 && (quoted[1] == '(' || quoted[1] == '['))
    {
        if (!CompareString(argv[2], "y"))
        {
            snprintf(err, errsize, "illegal index register '%s' used for "
                                        "addressing mode", argv[2]);
            *mode = ADDR_MODE_ERROR;
            return;
        }

        if (quoted[1] == '(')
        {
            char index;
            char *new;

            if (!(new = HasIndex(argv[1], &index)))
            {
                if (!ExprEval(argv[1], address))
                {
                    snprintf(err, errsize, "%s: expression error:  %s",
                                                argv[1], ExprError());
                    *mode = ADDR_MODE_ERROR;
                    return;
                }

                CMD_RANGE_ADDR_MODE(mode,
                                    DIRECT_PAGE_INDEX_Y_INDIRECT,
                                    ADDR_MODE_ERROR,
                                    ADDR_MODE_ERROR,
                                    address);

                return;
            }

            if (!CompareChar(index, 's'))
            {
                snprintf(err, errsize, "illegal index register '%c'", index);
                *mode = ADDR_MODE_ERROR;
                free(new);
                return;
            }

            if (!ExprEval(new, address))
            {
                snprintf(err, errsize, "%s: expression error:  %s",
                                            new, ExprError());
                *mode = ADDR_MODE_ERROR;
                free(new);
                return;
            }

            CMD_RANGE_ADDR_MODE(mode,
                                STACK_RELATIVE_INDIRECT_INDEX_Y,
                                ADDR_MODE_ERROR,
                                ADDR_MODE_ERROR,
                                address);
        }
        else
        {
            if (!ExprEval(argv[1], address))
            {
                snprintf(err, errsize, "%s: expression error:  %s",
                                            argv[1], ExprError());
                *mode = ADDR_MODE_ERROR;
                return;
            }

            CMD_RANGE_ADDR_MODE(mode,
                                DIRECT_PAGE_INDEX_Y_INDIRECT_LONG,
                                ADDR_MODE_ERROR,
                                ADDR_MODE_ERROR,
                                address);
        }

        return;
    }
}



/* ---------------------------------------- COMMAND HANDLERS
*/

static CommandStatus MX8_16(const char *label, int argc, char *argv[],
                            int quoted[], char *err, size_t errsize)
{
    if (argc == 1)
    {
        if (CompareString(argv[0], "M8") || CompareString(argv[0], ".M8"))
        {
            option.a16 = FALSE;
            return CMD_OK;
        }
        else if (CompareString(argv[0], "M16") ||
                 CompareString(argv[0], ".M16"))
        {
            option.a16 = TRUE;
            return CMD_OK;
        }
        else if (CompareString(argv[0], "X8") || CompareString(argv[0], ".X8"))
        {
            option.i16 = FALSE;
            return CMD_OK;
        }
        else if (CompareString(argv[0], "X16") ||
                 CompareString(argv[0], ".X16"))
        {
            option.i16 = TRUE;
            return CMD_OK;
        }
    }
    else if (argc == 3)
    {
        if (CompareString(argv[0], "MX") || CompareString(argv[0], ".MX"))
        {
            long asize;
            long isize;

            CMD_EXPR(argv[1], asize);
            CMD_EXPR(argv[2], isize);

            if ((asize !=8 && asize != 16) || (isize != 8 && isize != 16))
            {
                snprintf(err, errsize, "%s: unsupported regsiter sizes %s,%s",
                                            argv[0], argv[1], argv[2]);
                return CMD_FAILED;
            }

            option.a16 = (asize == 16);
            option.i16 = (isize == 16);

            return CMD_OK;
        }
    }

    snprintf(err, errsize, "%s: bad directive", argv[0]);

    return CMD_FAILED;
}

#define COMMON(base)                                                    \
do {                                                                    \
    address_mode_t mode;                                                \
    long address;                                                       \
                                                                        \
    CMD_ADDRESS_MODE(mode, address);                                    \
                                                                        \
    switch(mode)                                                        \
    {                                                                   \
        case DIRECT_PAGE_INDEX_X_INDIRECT:                              \
            PCWrite(base + 0x01);                                       \
            PCWrite(address);                                           \
            return CMD_OK;                                              \
                                                                        \
        case STACK_RELATIVE:                                            \
            PCWrite(base + 0x03);                                       \
            PCWrite(address);                                           \
            return CMD_OK;                                              \
                                                                        \
        case DIRECT_PAGE:                                               \
            PCWrite(base + 0x05);                                       \
            PCWrite(address);                                           \
            return CMD_OK;                                              \
                                                                        \
        case DIRECT_PAGE_INDIRECT_LONG:                                 \
            PCWrite(base + 0x07);                                       \
            PCWrite(address);                                           \
            return CMD_OK;                                              \
                                                                        \
        case IMMEDIATE:                                                 \
            PCWrite(base + 0x09);                                       \
            PCWrite8_16_A(address);                                     \
            return CMD_OK;                                              \
                                                                        \
        case ABSOLUTE:                                                  \
            PCWrite(base + 0x0d);                                       \
            PCWriteWord(address);                                       \
            return CMD_OK;                                              \
                                                                        \
        case ABSOLUTE_LONG:                                             \
            PCWrite(base + 0x0f);                                       \
            PCWrite24(address);                                         \
            return CMD_OK;                                              \
                                                                        \
        case DIRECT_PAGE_INDEX_Y_INDIRECT:                              \
            PCWrite(base + 0x11);                                       \
            PCWrite(address);                                           \
            return CMD_OK;                                              \
                                                                        \
        case DIRECT_PAGE_INDIRECT:                                      \
            PCWrite(base + 0x12);                                       \
            PCWrite(address);                                           \
            return CMD_OK;                                              \
                                                                        \
        case STACK_RELATIVE_INDIRECT_INDEX_Y:                           \
            PCWrite(base + 0x13);                                       \
            PCWrite(address);                                           \
            return CMD_OK;                                              \
                                                                        \
        case DIRECT_PAGE_INDEX_X:                                       \
            PCWrite(base + 0x15);                                       \
            PCWrite(address);                                           \
            return CMD_OK;                                              \
                                                                        \
        case DIRECT_PAGE_INDEX_Y_INDIRECT_LONG:                         \
            PCWrite(base + 0x17);                                       \
            PCWrite(address);                                           \
            return CMD_OK;                                              \
                                                                        \
        case ABSOLUTE_INDEX_Y:                                          \
            PCWrite(base + 0x19);                                       \
            PCWriteWord(address);                                       \
            return CMD_OK;                                              \
                                                                        \
        case ABSOLUTE_INDEX_X:                                          \
            PCWrite(base + 0x1d);                                       \
            PCWriteWord(address);                                       \
            return CMD_OK;                                              \
                                                                        \
        case ABSOLUTE_LONG_INDEX_X:                                     \
            PCWrite(base + 0x1f);                                       \
            PCWrite24(address);                                         \
            return CMD_OK;                                              \
                                                                        \
        default:                                                        \
            snprintf(err, errsize, "%s: unsupported addressing mode %s",\
                                            argv[0], address_mode_name[mode]);\
            return CMD_FAILED;                                          \
    }                                                                   \
} while(0)

static CommandStatus ADC(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    COMMON(0x60);
}

static CommandStatus AND(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    COMMON(0x20);
}

static CommandStatus ASL(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
            PCWrite(0x06);
            PCWrite(address);
            return CMD_OK;

        case IMPLIED:
        case ACCUMULATOR:
            PCWrite(0x0a);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x0e);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0x16);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x1e);
            PCWriteWord(address);
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
        case DIRECT_PAGE:
            PCWrite(0x24);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x2c);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0x34);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x3c);
            PCWriteWord(address);
            return CMD_OK;

        case IMMEDIATE:
            PCWrite(0x89);
            PCWrite8_16_A(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus TRB(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
            PCWrite(0x14);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x1c);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus TSB(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
            PCWrite(0x04);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x0c);
            PCWriteWord(address);
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
    COMMON(0xc0);
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
            PCWrite8_16_XY(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xec);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE:
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
            PCWrite8_16_XY(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xcc);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE:
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
        case IMPLIED:
        case ACCUMULATOR:
            PCWrite(0x3a);
            return CMD_OK;

        case DIRECT_PAGE:
            PCWrite(0xc6);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xce);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0xd6);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0xce);
            PCWriteWord(address);
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
    COMMON(0x40);
}

static CommandStatus INC(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case IMPLIED:
        case ACCUMULATOR:
            PCWrite(0x1a);
            return CMD_OK;

        case DIRECT_PAGE:
            PCWrite(0xe6);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xee);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0xf6);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0xfe);
            PCWriteWord(address);
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
        case DIRECT_PAGE:
        case ABSOLUTE:
            PCWrite(0x4c);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_LONG:
            PCWrite(0x5c);
            PCWrite24(address);
            return CMD_OK;

        case ABSOLUTE_INDIRECT:
            PCWrite(0x6c);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X_INDIRECT:
            PCWrite(0x7c);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDIRECT_LONG:
            PCWrite(0xdc);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus JSL(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
        case ABSOLUTE:
        case ABSOLUTE_LONG:
            PCWrite(0x22);
            PCWrite24(address);
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
        case DIRECT_PAGE:
            PCWrite(0x20);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X_INDIRECT:
            PCWrite(0xfc);
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
    COMMON(0xa0);
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
            PCWrite8_16_XY(address);
            return CMD_OK;

        case DIRECT_PAGE:
            PCWrite(0xa6);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xae);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_Y:
            PCWrite(0xb6);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_Y:
            PCWrite(0xbe);
            PCWriteWord(address);
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
            PCWrite8_16_XY(address);
            return CMD_OK;

        case DIRECT_PAGE:
            PCWrite(0xa4);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0xac);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0xb4);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0xbc);
            PCWriteWord(address);
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
        case DIRECT_PAGE:
            PCWrite(0x46);
            PCWrite(address);
            return CMD_OK;

        case IMPLIED:
        case ACCUMULATOR:
            PCWrite(0x4a);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x4e);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0x56);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x5e);
            PCWriteWord(address);
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
    COMMON(0x00);
}

static CommandStatus ROL(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
            PCWrite(0x26);
            PCWrite(address);
            return CMD_OK;

        case IMPLIED:
        case ACCUMULATOR:
            PCWrite(0x2a);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x2e);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0x36);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x3e);
            PCWriteWord(address);
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
        case DIRECT_PAGE:
            PCWrite(0x66);
            PCWrite(address);
            return CMD_OK;

        case IMPLIED:
        case ACCUMULATOR:
            PCWrite(0x6a);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x6e);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0x76);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x7e);
            PCWriteWord(address);
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
    COMMON(0xe0);
}

static CommandStatus STA(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    COMMON(0x80);
}

static CommandStatus STX(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
            PCWrite(0x86);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x8e);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_Y:
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
        case DIRECT_PAGE:
            PCWrite(0x84);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x8c);
            PCWriteWord(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0x94);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus STZ(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
            PCWrite(0x64);
            PCWrite(address);
            return CMD_OK;

        case DIRECT_PAGE_INDEX_X:
            PCWrite(0x74);
            PCWrite(address);
            return CMD_OK;

        case ABSOLUTE:
            PCWrite(0x9c);
            PCWriteWord(address);
            return CMD_OK;

        case ABSOLUTE_INDEX_X:
            PCWrite(0x9e);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus COP(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
        case IMMEDIATE:
            PCWrite(0x02);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus REP(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
        case IMMEDIATE:
            PCWrite(0xc2);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus SEP(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
        case IMMEDIATE:
            PCWrite(0xe2);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus MVN_MVP(const char *label, int argc, char *argv[],
                             int quoted[], char *err, size_t errsize)
{
    /* This opcode uses two distinct arguments, rather than a single addressing
       mode
    */
    address_mode_t mode1;
    address_mode_t mode2;
    long address1;
    long address2;

    CMD_ARGC_CHECK(3);

    CalcAddressMode(2, argv, quoted, err, errsize, &mode1, &address1);

    if (mode1 == ADDR_MODE_UNKNOWN)
    {
        snprintf(err, errsize, "%s: couldn't work out "
                                        "addressing mode", argv[0]);
        return CMD_FAILED;
    }
                                                                        \
    CalcAddressMode(2, argv + 1, quoted + 1, err, errsize, &mode2, &address2);

    if (mode2 == ADDR_MODE_UNKNOWN)
    {
        snprintf(err, errsize, "%s: couldn't work out "
                                        "addressing mode", argv[0]);
        return CMD_FAILED;
    }
                                                                        \
    if (mode1 == ADDR_MODE_ERROR || mode2 == ADDR_MODE_ERROR)
    {
        return CMD_FAILED;
    }

    if (mode1 == IMMEDIATE && mode2 == IMMEDIATE)
    {
        if (CompareString(argv[0], "MVN"))
        {
            PCWrite(0x54);
        }
        else
        {
            PCWrite(0x44);
        }

        PCWrite(address1);
        PCWrite(address2);
        return CMD_OK;
    }

    snprintf(err, errsize, "%s: unsupported addressing mode(s) %s, %s",
                                    argv[0], argv[1], argv[2]);
    return CMD_FAILED;
}

static CommandStatus PEA(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
        case IMMEDIATE:
        case ABSOLUTE:
            PCWrite(0xf4);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus PEI(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
            PCWrite(0xd4);
            PCWrite(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus PER(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;

    CMD_ADDRESS_MODE(mode, address);

    switch(mode)
    {
        case DIRECT_PAGE:
        case IMMEDIATE:
        case ABSOLUTE:
            PCWrite(0x62);
            PCWriteWord(address);
            return CMD_OK;

        default:
            snprintf(err, errsize, "%s: unsupported addressing mode %s",
                                            argv[0], address_mode_name[mode]);
            return CMD_FAILED;
    }
}

static CommandStatus OP_SIGNATURE(const char *label, int argc, char *argv[],
                                  int quoted[], char *err, size_t errsize)
{
    address_mode_t mode;
    long address;
    int opcode;

    CMD_ADDRESS_MODE(mode, address);

    if (CompareString(argv[0], "WDM"))
    {
        opcode = 0x42;
    }
    else
    {
        opcode = 0x00;
    }

    switch(mode)
    {
        case IMMEDIATE:
        case ACCUMULATOR:
            PCWrite(opcode);
            PCWrite(0x00);
            return CMD_OK;

        default:
            PCWrite(opcode);
            PCWrite(address);
            return CMD_OK;
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
    {"NOP",     0xea},

    {"TAX",     0xaa},
    {"TXA",     0x8a},
    {"TAY",     0xa8},
    {"TYA",     0x98},
    {"TXS",     0x9a},
    {"TSX",     0xba},
    {"TXA",     0x8a},
    {"TXY",     0x9b},
    {"TYX",     0xbb},

    {"TCD",     0x5b},
    {"TCS",     0x1b},
    {"TDC",     0x7b},
    {"TSC",     0x3b},

    {"XBA",     0xeb},
    {"XCE",     0xfb},

    {"DEX",     0xca},
    {"DEY",     0x88},
    {"INX",     0xe8},
    {"INY",     0xc8},

    {"PHA",     0x48},
    {"PHX",     0xda},
    {"PHY",     0x5a},
    {"PHB",     0x8b},
    {"PHD",     0x0b},
    {"PHK",     0x4b},
    {"PHP",     0x08},

    {"PLA",     0x68},
    {"PLX",     0xfa},
    {"PLY",     0x7a},
    {"PLB",     0xab},
    {"PLD",     0x2b},
    {"PLP",     0x28},

    {"CLC",     0x18},
    {"SEC",     0x38},
    {"CLI",     0x58},
    {"SEI",     0x78},
    {"CLV",     0xb8},
    {"CLD",     0xd8},
    {"SED",     0xf8},
    {"RTI",     0x40},
    {"RTL",     0x6b},
    {"RTS",     0x60},
    {"STP",     0xdb},
    {"WAI",     0xcb},
    {NULL}
};


static const OpcodeTable branch_opcodes[] =
{
    {"BPL",     0x10},
    {"BMI",     0x30},
    {"BVC",     0x50},
    {"BVS",     0x70},
    {"BCC",     0x90},
    {"BCS",     0xB0},
    {"BNE",     0xD0},
    {"BEQ",     0xF0},
    {"BRA",     0x80},
    {NULL}
};


static const OpcodeTable long_branch_opcodes[] =
{
    {"BRL",     0x82},
    {NULL}
};


static const HandlerTable handler_table[] =
{
    {"M8",      MX8_16},
    {"M16",     MX8_16},
    {".M8",     MX8_16},
    {".M16",    MX8_16},
    {"X8",      MX8_16},
    {"X16",     MX8_16},
    {".X8",     MX8_16},
    {".X16",    MX8_16},
    {"MX",      MX8_16},
    {".MX",     MX8_16},

    {"ADC",     ADC},
    {"AND",     AND},
    {"ASL",     ASL},
    {"BIT",     BIT},
    {"TRB",     TRB},
    {"TSB",     TSB},
    {"CMP",     CMP},
    {"CPX",     CPX},
    {"CPY",     CPY},
    {"DEC",     DEC},
    {"EOR",     EOR},
    {"INC",     INC},
    {"JMP",     JMP},
    {"JSL",     JSL},
    {"JSR",     JSR},
    {"LDA",     LDA},
    {"LDX",     LDX},
    {"LDY",     LDY},
    {"LSR",     LSR},
    {"ORA",     ORA},
    {"ROL",     ROL},
    {"ROR",     ROR},
    {"SBC",     SBC},
    {"STA",     STA},
    {"STX",     STX},
    {"STY",     STY},
    {"STZ",     STZ},
    {"COP",     COP},
    {"REP",     REP},
    {"SEP",     SEP},
    {"MVN",     MVN_MVP},
    {"MVP",     MVN_MVP},
    {"PEA",     PEA},
    {"PER",     PER},
    {"PEI",     PEI},

    {"WDM",     OP_SIGNATURE},
    {"BRK",     OP_SIGNATURE},

    {NULL}
};




/* ---------------------------------------- PUBLIC FUNCTIONS
*/

void Init_65c816(void)
{
    option.a16 = FALSE;
    option.i16 = FALSE;
    SetNeededPasses(3);
}


const ValueTable *Options_65c816(void)
{
    return options;
}

CommandStatus SetOption_65c816(int opt, int argc, char *argv[],
                             int quoted[], char *err, size_t errsize)
{
    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_A16:
            option.a16 = ParseTrueFalse(argv[0], FALSE);
            break;

        case OPT_I16:
            option.i16 = ParseTrueFalse(argv[0], FALSE);
            break;

        default:
            break;
    }

    return CMD_OK;
}

CommandStatus Handler_65c816(const char *label, int argc, char *argv[],       
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

    for(f = 0; long_branch_opcodes[f].op; f++)
    {
        if (CompareString(argv[0], long_branch_opcodes[f].op))
        {
            long offset;

            CMD_ARGC_CHECK(2);

            CMD_EXPR(argv[1], offset);

            offset = offset - (PC() + 3);

            if (IsFinalPass() && (offset < -32768 || offset > 32767))
            {
                snprintf(err, errsize, "%s: Branch offset (%d) too big",
                                            argv[1], offset);
                return CMD_FAILED;
            }

            PCWrite(long_branch_opcodes[f].code);
            PCWriteWord(offset);

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
