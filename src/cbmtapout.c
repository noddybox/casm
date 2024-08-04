/*

    casm - Simple, portable assembler

    Copyright (C) 2003-2024  Ian Cowburn (ianc@noddybox.co.uk)

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

    Commodore TAP output handler.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "codepage.h"
#include "cbmtapout.h"
#include "expr.h"


/* ---------------------------------------- MACROS & TYPES
*/

/* ---------------------------------------- PRIVATE TYPES AND VARS
*/
enum option_t
{
    OPT_START_ADDR,
    OPT_SYSTEM_TYPE
};

static const ValueTable option_set[]=
{
    {"cbm-tap-start", OPT_START_ADDR},
    {"cbm-tap-system", OPT_SYSTEM_TYPE},
    {NULL}
};

typedef enum
{
    SYS_C64,
    SYS_VIC20,
    SYS_VIC20_8K
} system_t;

static ValueTable system_table[]=
{
    {"c64",     SYS_C64},
    {"vic20",   SYS_VIC20},
    {"vic20+8k",SYS_VIC20_8K},
    {NULL}
};

typedef struct
{
    int		start_addr;
    system_t    system;
} Options;

static Options options =
{
    -1,
    SYS_C64
};


/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static int PokeB(Byte *mem, int addr, Byte b)
{
    mem[addr++] = b;
    return (addr % 0x10000);
}


static int PokeW(Byte *mem, int addr, int w)
{
    addr = PokeB(mem, addr, w & 0xff);
    return PokeB(mem, addr, (w & 0xff00) >> 8);
}


static int PokeS(Byte *mem, int addr, const char *str)
{
    while(*str)
    {
        addr = PokeB(mem, addr, CodeFromNative(CP_CBM, *str++));
    }

    return addr;
}


static void WriteByte(FILE *fp, Byte b)
{
    putc(b, fp);
}


static void WriteWord(FILE *fp, int w)
{
    WriteByte(fp, w & 0xff);
    WriteByte(fp, (w & 0xff00) >> 8);
}


static void WriteLongWord(FILE *fp, unsigned long l)
{
    WriteByte(fp, l & 0xfflu);
    WriteByte(fp, (l & 0xff00lu) >> 8);
    WriteByte(fp, (l & 0xff0000lu) >> 16);
    WriteByte(fp, (l & 0xff000000lu) >> 24);
}

static void WriteASCII(FILE *fp, const char *str)
{
    while(*str)
    {
        WriteByte(fp, CodeFromNative(CP_ASCII, *str++));
    }
}


/* ---------------------------------------- INTERFACES
*/
const ValueTable *CBMTAPOutputOptions(void)
{
    return option_set;
}

CommandStatus CBMTAPOutputSetOption(int opt, int argc, char *argv[],
                                    int quoted[], char *err, size_t errsize)
{
    const ValueTable *val;
    CommandStatus stat = CMD_OK;

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_START_ADDR:
            CMD_EXPR(argv[0], options.start_addr);
            break;

        case OPT_SYSTEM_TYPE:
            CMD_TABLE(argv[0], system_table, val);
            options.system = val->value;
            break;

        default:
            break;
    }

    return stat;
}

int CBMTAPOutput(const char *filename, const char *filename_bank,
                 MemoryBank **bank, int count, char *error, size_t error_size)
{
    int f;

    for(f = 0; f < count; f++)
    {
        FILE *fp;
        char buff[4096];
        const char *name;
        Byte *mem;
        int min, max;
        unsigned long len;
        char sys[16];
        int addr;
        int start_addr;
        int next;
        int i;

        if (count == 1)
        {
            name = filename;
        }
        else
        {
            snprintf(buff, sizeof buff, filename_bank, bank[f]->number);
            name = buff;
        }

        if (!(fp = fopen(name, "wb")))
        {
            snprintf(error, error_size, "Failed to open %s", name);
            return FALSE;
        }

        switch(options.system)
        {
            case SYS_C64:
                addr = 0x803;
                start_addr = 0x801;
                break;
            case SYS_VIC20:
                addr = 0x1003;
                start_addr = 0x1001;
                break;
            case SYS_VIC20_8K:
                addr = 0x1203;
                start_addr = 0x1201;
                break;
        }

        mem = bank[f]->memory;
        min = bank[f]->min_address_used;
        max = bank[f]->max_address_used;

        if (min < (addr + 0x10))
        {
            snprintf(error, error_size, "Bank starts below a safe "
                                        "area to add BASIC loader");

            return FALSE;
        }

        /* We're going to prepend some BASIC
        */
        if (options.start_addr == -1)
        {
            snprintf(sys, sizeof sys, "%d", bank[f]->min_address_used);
        }
        else
        {
            snprintf(sys, sizeof sys, "%d", options.start_addr);
        }

        addr = PokeW(mem, addr, 10);
        addr = PokeB(mem, addr, 0x9e);
        addr = PokeS(mem, addr, sys);
        addr = PokeB(mem, addr, 0x00);

        next = addr;

        addr = PokeW(mem, addr, 0x00);

        PokeW(mem, start_addr, next);

        min = start_addr;        /* Start of BASIC */

        len = max - min + 1;

        /* Write out TAP file header
        */
        WriteASCII(fp, "C64-TAPE-RAW");
        WriteByte(fp, 0);
        WriteByte(fp, 0);
        WriteByte(fp, 0);
        WriteByte(fp, 0);
        WriteLongWord(fp, len);

        /* Output file data
        */
        fwrite(mem + min, len, 1, fp);

        fclose(fp);
    }

    return TRUE;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
