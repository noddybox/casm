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

    Spectrum TAP file output handler.

*/
#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "specout.h"


/* ---------------------------------------- PRIVATE FUNCTIONS
*/

static Byte TapByte(FILE *fp, Byte b, Byte chk)
{
    chk ^= b;
    putc(b, fp);
    return chk;
}


static Byte TapWord(FILE *fp, int w, Byte chk)
{
    chk = TapByte(fp, w & 0xff, chk);
    chk = TapByte(fp, (w & 0xff00) >> 8, chk);
    return chk;
}


static Byte TapString(FILE *fp, const char *p, int len, Byte chk)
{
    while(len--)
    {
        chk = TapByte(fp, *p ? *p++ : ' ', chk);
    }

    return chk;
}


/* ---------------------------------------- INTERFACES
*/
const ValueTable *SpecTAPOutputOptions(void)
{
    return NULL;
}

CommandStatus SpecTAPOutputSetOption(int opt, int argc, char *argv[],
                                     int quoted[],
                                     char *error, size_t error_size)
{
    return CMD_NOT_KNOWN;
}

int SpecTAPOutput(const char *filename, const char *filename_bank,
                  MemoryBank **bank, int count, char *error, size_t error_size)
{
    FILE *fp = fopen(filename, "wb");
    int f;

    if (!fp)
    {
        snprintf(error, error_size, "Failed to create %s", filename);
        return FALSE;
    }

    for(f = 0; f < count; f++)
    {
        Byte chk = 0;
        const Byte *mem;
        int min, max, len;

        mem = bank[f]->memory;
        min = bank[f]->min_address_used;
        max = bank[f]->max_address_used;
        len = max - min + 1;

        TapWord(fp, 19, 0);

        chk = TapByte(fp, 0, chk);
        chk = TapByte(fp, 3, chk);

        if (count == 1)
        {
            chk = TapString(fp, filename, 10, chk);
        }
        else
        {
            char fn[16];

            snprintf(fn, sizeof fn, filename_bank, bank[f]->number);
            chk = TapString(fp, fn, 10, chk);
        }

        chk = TapWord(fp, len, chk);
        chk = TapWord(fp, min, chk);
        chk = TapWord(fp, 32768, chk);

        TapByte(fp, chk, 0);

        /* Output file data
        */
        TapWord(fp, len + 2, 0);

        chk = 0;

        chk = TapByte(fp, 0xff, chk);

        while(min <= max)
        {
            chk = TapByte(fp, mem[min], chk);
            min++;
        }

        TapByte(fp, chk, 0);
    }

    fclose(fp);

    return TRUE;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
