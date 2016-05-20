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

    NES ROM output handler.

    Notes:

        * ROM is 0x8000 to 0xffff (just 0xc000 to 0xffff if 16K ROM)

        * VROM is at 0x0000 to 0x1fff.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "expr.h"
#include "codepage.h"
#include "nesout.h"


/* ---------------------------------------- MACROS & TYPES
*/

enum option_t
{
    OPT_VECTOR,
    OPT_TV_FORMAT,
    OPT_MAPPER
};

static const ValueTable option_set[] =
{
    {"nes-vector",              OPT_VECTOR},
    {"nes-tv-format",           OPT_TV_FORMAT},
    {"nes-mapper",              OPT_MAPPER},
    {NULL}
};

typedef enum
{
    VECTOR_RESET,
    VECTOR_NMI,
    VECTOR_BRK
} VectorType;

static ValueTable       vector_table[] =
{
    {"reset",           VECTOR_RESET},
    {"nmi",             VECTOR_NMI},
    {"brk",             VECTOR_BRK},
    {NULL}
};

typedef enum
{
    PAL = 1,
    NTSC = 0
} TVFormat;

static ValueTable       format_table[] =
{
    {"pal",             PAL},
    {"NTSC",            NTSC},
    {NULL}
};


static struct
{
    int         vector[3];
    TVFormat    tv_format;
    int         mapper;
} option =
{
    {-1, -1, -1}, PAL, 0
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


static int PokeS(Byte *mem, int addr, const char *str, int maxlen, char pad)
{
    while(*str && maxlen--)
    {
        addr = PokeB(mem, addr, CodeFromNative(CP_ASCII, *str++));
    }

    while(maxlen-- > 0)
    {
        addr = PokeB(mem, addr, CodeFromNative(CP_ASCII, pad));
    }

    return addr;
}

static unsigned CalcChecksum(Byte *p, int len, unsigned csum)
{
    while(len-- > 0)
    {
        csum += *p++;
    }

    return csum & 0xffff;
}


/* ---------------------------------------- INTERFACES
*/
const ValueTable *NESOutputOptions(void)
{
    return option_set;
}

CommandStatus NESOutputSetOption(int opt, int argc, char *argv[],
                                  int quoted[], char *err, size_t errsize)
{
    const ValueTable *val;
    int f;

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_VECTOR:
            CMD_ARGC_CHECK(2);
            CMD_TABLE(argv[0], vector_table, val);
            CMD_EXPR(argv[1], f);
            option.vector[val->value] = f;
            break;

        case OPT_TV_FORMAT:
            CMD_TABLE(argv[0], format_table, val);
            option.tv_format = val->value;
            break;

        case OPT_MAPPER:
            CMD_EXPR(argv[1], f);
            option.mapper = f;
            break;

        default:
            break;
    }

    return CMD_OK;

}

int NESOutput(const char *filename, const char *filename_bank,
              MemoryBank **bank, int count, char *error, size_t error_size)
{
    FILE *fp;
    Byte *mem;
    int num_rom;
    int num_vrom;
    int is_16k;
    int first_code;
    int start;
    int f;

    /* Go through the banks and see which are code (ROM), and which char (VROM)
    */
    num_rom = 0;
    num_vrom = 0;
    is_16k = FALSE;
    first_code = -1;

    for(f = 0; f < count; f++)
    {
        if (bank[f]->max_address_used < 0x2000)
        {
            num_vrom++;
        }
        else if (bank[f]->min_address_used >= 0xc000)
        {
            if (first_code == -1)
            {
                first_code = f;
            }

            is_16k = TRUE;
            num_rom++;
        }
        else if (bank[f]->min_address_used >= 0x8000)
        {
            if (first_code == -1)
            {
                first_code = f;
            }

            is_16k = FALSE;
            num_rom++;
        }
        else
        {
            snprintf(error, error_size,
                        "Banks should use memory in the range 0x0000 - 0x1fff "
                        "to indicate they are\nvideo ROM, 0x8000 - 0xffff to "
                        "indicate they are program ROM or 0xc000 - 0xffff\n"
                        "to indicate a single 16K ROM segment.");

            return FALSE;
        }
    }

    if (first_code == -1)
    {
        snprintf(error, error_size,
                    "No ROM code banks present;\n"
                    "Banks should use memory in the range 0x0000 - 0x1fff "
                    "to indicate they are\nvideo ROM, 0x8000 - 0xffff to "
                    "indicate they are program ROM or 0xc000 - 0xffff\n"
                    "to indicate a single 16K ROM segment.");

        return FALSE;
    }

    if (!(fp = fopen(filename, "wb")))
    {
        snprintf(error, error_size, "Failed to create %s", filename);
        return FALSE;
    }

    /* Setup vectors
    */
    mem = bank[first_code]->memory;

    start = option.vector[VECTOR_RESET];

    if (start == -1)
    {
        start = is_16k ? 0xc000 : 0x8000;

        fprintf(stderr, "WARNING: No reset vector provided; assuming 0x%4.4x\n",
                                        start);
    }

    PokeW(mem, 0xfffc, start);

    if (option.vector[VECTOR_NMI] != -1)
    {
        PokeW(mem, 0xfffa, option.vector[VECTOR_NMI]);
    }
    else
    {
        fprintf(stderr, "WARNING: NMI vector not set\n");
    }

    if (option.vector[VECTOR_BRK] != -1)
    {
        PokeW(mem, 0xfffe, option.vector[VECTOR_BRK]);
    }

    /* Output header
    */
    fputs("NES", fp);
    fputc(26, fp);

    if (is_16k)
    {
        fputc(1, fp);
    }
    else
    {
        fputc(num_rom * 2, fp);
    }

    fputc(num_vrom, fp);

    fputc((option.mapper & 0x0f) << 4, fp);
    fputc(option.mapper & 0xf0, fp);

    fputc(0, fp);

    fputc(option.tv_format, fp);

    for(f = 0; f < 6; f++)
    {
        fputc(0, fp);
    }

    /* Output ROM contents, first code segments
    */
    for(f = 0; f < count; f++)
    {
        if (bank[f]->min_address_used > 0x2000)
        {
            if (is_16k)
            {
                fwrite(bank[f]->memory + 0xc000, 0x4000, 1, fp);
            }
            else
            {
                fwrite(bank[f]->memory + 0x8000, 0x8000, 1, fp);
            }
        }
    }

    /* Then video ROM banks
    */
    for(f = 0; f < count; f++)
    {
        if (bank[f]->min_address_used < 0x2000)
        {
            fwrite(bank[f]->memory, 0x2000, 1, fp);
        }
    }

    fclose(fp);

    return TRUE;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
