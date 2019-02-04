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

    Amstrad CPC tape output handler.

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
static Byte WriteByte(FILE *fp, Byte b, Byte chk)
{
    chk ^= b;
    putc(b, fp);
    return chk;
}


static Byte WriteWord(FILE *fp, int w, Byte chk)
{
    chk = WriteByte(fp, w & 0xff, chk);
    chk = WriteByte(fp, (w & 0xff00) >> 8, chk);
    return chk;
}


static Byte Write3Word(FILE *fp, int w, Byte chk)
{
    chk = WriteByte(fp, w & 0xff, chk);
    chk = WriteByte(fp, (w & 0xff00) >> 8, chk);
    chk = WriteByte(fp, (w & 0xff0000) >> 16, chk);
    return chk;
}


static Byte WriteString(FILE *fp, const char *p, int len,
                        Byte fill, Codepage cp, Byte chk)
{
    while(len--)
    {
        chk = WriteByte(fp, *p ? CodeFromNative(cp, *p++) :
                                 CodeFromNative(cp, fill), chk);
    }

    return chk;
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
    WriteString(fp, "ZXTape!", 7, 0, CP_ASCII, 0);
    WriteByte(fp, 0x1a, 0);
    WriteByte(fp, 1, 0);
    WriteByte(fp, 13, 0);

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

        if ((len % BLOCK_SIZE) == 0)
        {
            blocks--;
        }

        for(block = 0; block <= blocks; block++)
        {
            Byte chk;
            int first, last;

            first = 0;
            last = 0;

            WriteByte(fp, 0x11, 0);     /* Block type */

            WriteWord(fp, 0x626, 0);    /* PILOT */
            WriteWord(fp, 0x34f, 0);    /* SYNC1 */
            WriteWord(fp, 0x302, 0);    /* SYNC2 */
            WriteWord(fp, 0x33a, 0);    /* ZERO */
            WriteWord(fp, 0x673, 0);    /* ONE */
            WriteWord(fp, 0xffe, 0);    /* PILOT LEN */
            WriteByte(fp, 8, 0);        /* USED BITS */
            WriteWord(fp, 0x10, 0);     /* PAUSE */
            Write3Word(fp, 0x0041, 0);  /* LEN */

            chk = 0;

            if (f == 0)
            {
                chk = WriteString(fp, filename, 16, 0, CP_ASCII, chk);
            }
            else
            {
                char fn[16];

                snprintf(fn, sizeof fn, filename_bank, bank[f]->number);
                chk = WriteString(fp, fn, 16, 0, CP_ASCII, chk);
            }

            chk = WriteByte(fp, block+1, chk);

            if (block == 0)
            {
                first = 255;

                if (len > BLOCK_SIZE)
                {
                    blocklen = BLOCK_SIZE;
                }
                else
                {
                    blocklen = len;
                }
            }

            if (block == blocks)
            {
                last = 255;
                blocklen = len % BLOCK_SIZE;
            }

            chk = WriteByte(fp, last, chk);
            chk = WriteByte(fp, 2, chk);
            chk = WriteWord(fp, blocklen, chk);
            chk = WriteWord(fp, addr, chk);
            chk = WriteByte(fp, first, chk);
            chk = WriteWord(fp, len, chk);
            chk = WriteWord(fp, options.start_addr, chk);
            chk = WriteString(fp, "", 64 - 28, 0, CP_ASCII, chk);

            WriteByte(fp, chk, 0);

            addr += blocklen;

            /* Output file data
            */
            WriteByte(fp, 0x11, 0);             /* Block type */

            WriteWord(fp, 0x626, 0);            /* PILOT */
            WriteWord(fp, 0x34f, 0);            /* SYNC1 */
            WriteWord(fp, 0x302, 0);            /* SYNC2 */
            WriteWord(fp, 0x33a, 0);            /* ZERO */
            WriteWord(fp, 0x673, 0);            /* ONE */
            WriteWord(fp, 0xffe, 0);            /* PILOT LEN */
            WriteByte(fp, 8, 0);                /* USED BITS */
            WriteWord(fp, 0x10, 0);             /* PAUSE */
            Write3Word(fp, blocklen + 1, 0);    /* LEN */

            chk = 0;

            while(min < addr)
            {
                chk = WriteByte(fp, mem[min], chk);
                min++;
            }

            WriteByte(fp, chk, 0);
        }
    }

    fclose(fp);

    return TRUE;
}

/*
vim: ai sw=4 ts=8 expandtab
*/
