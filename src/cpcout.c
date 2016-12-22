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

    Commodore CPC tape output handler.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "codepage.h"
#include "cpcout.h"
#include "expr.h"


/* ---------------------------------------- MACROS & TYPES
*/

#define BLOCK_SIZE      2048

enum option_t
{
    OPT_START_ADDR,
};

static const ValueTable option_set[] =
{
    {"cpc-start",  OPT_START_ADDR},
    {NULL}
};

typedef struct
{
    int         start_addr;
} Options;

static Options options = {-1};

/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static void WriteByte(FILE *fp, Byte b)
{
    putc(b, fp);
}


static void WriteWord(FILE *fp, int w)
{
    WriteByte(fp, w & 0xff);
    WriteByte(fp, (w & 0xff00) >> 8);
}


static void WriteLong(FILE *fp, unsigned long l)
{
    int f;

    for(f = 0; f < 4; f++)
    {
        WriteByte(fp, l & 0xff);
        l >>= 8u;
    }
}


static void WriteString(FILE *fp, const char *p, int len,
                        Byte fill, Codepage cp)
{
    while(len--)
    {
        WriteByte(fp, *p ? CodeFromNative(cp, *p++) : CodeFromNative(cp, fill));
    }
}


/* ---------------------------------------- INTERFACES
*/
const ValueTable *CPCOutputOptions(void)
{
    return option_set;
}

CommandStatus CPCOutputSetOption(int opt, int argc, char *argv[], int quoted[],
                                 char *err, size_t errsize)
{
    CommandStatus stat = CMD_OK;

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_START_ADDR:
            CMD_EXPR(argv[0], options.start_addr);
            break;

        default:
            break;
    }

    return stat;
}

int CPCOutput(const char *filename, const char *filename_bank,
              MemoryBank **bank, int count, char *error, size_t error_size)
{
    FILE *fp = fopen(filename, "wb");
    int f;

    if (!fp)
    {
        snprintf(error, error_size, "Failed to create %s", filename);
        return FALSE;
    }

    /* First get the initial address is none defined
    */
    if (options.start_addr == -1)
    {
        options.start_addr = bank[0]->min_address_used;
    }

    /* Output the binary files
    */
    for(f = 0; f < count; f++)
    {
        const Byte *mem;
        int min, max, len, blocks, addr;
        int block, blocklen;

        mem = bank[f]->memory;
        min = bank[f]->min_address_used;
        addr = min;
        max = bank[f]->max_address_used;
        len = max - min + 1;
        blocks = len / BLOCK_SIZE;

        for(block = 0; block <= blocks; block++)
        {
            int first, last;

            first = 0;
            last = 0;

            WriteWord(fp, 0x1d);
            WriteByte(fp, 0x2c);

            if (f == 0)
            {
                WriteString(fp, filename, 16, 0, CP_ASCII);
            }
            else
            {
                char fn[16];

                snprintf(fn, sizeof fn, filename_bank, bank[f]->number);
                WriteString(fp, fn, 16, 0, CP_ASCII);
            }

            WriteByte(fp, block+1);

            if (block == 0)
            {
                first = 255;
                blocklen = BLOCK_SIZE;
            }

            if (block == blocks)
            {
                last = 255;
                blocklen = len % BLOCK_SIZE;
            }

            WriteByte(fp, last);
            WriteByte(fp, 2);
            WriteWord(fp, blocklen);
            WriteWord(fp, addr);
            WriteByte(fp, first);
            WriteWord(fp, len);
            WriteWord(fp, options.start_addr);

            addr += blocklen;

            /* Output file data
            */
            WriteWord(fp, blocklen + 3);
            WriteByte(fp, 0x16);

            while(min < addr)
            {
                WriteByte(fp, mem[min]);
                min++;
            }
        }
    }

    fclose(fp);

    return TRUE;
}

/*
vim: ai sw=4 ts=8 expandtab
*/
