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

    Commodore T64 tape output handler.

*/
#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "codepage.h"
#include "t64out.h"


/* ---------------------------------------- MACROS & TYPES
*/

#define NOT_USED        0

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
const ValueTable *T64OutputOptions(void)
{
    return NULL;
}

CommandStatus T64OutputSetOption(int opt, int argc, char *argv[],
                                     int quoted[],
                                     char *error, size_t error_size)
{
    return CMD_NOT_KNOWN;
}

int T64Output(const char *filename, const char *filename_bank,
              MemoryBank **bank, int count, char *error, size_t error_size)
{
    FILE *fp = fopen(filename, "wb");
    int f;
    int offset;

    if (!fp)
    {
        snprintf(error, error_size, "Failed to create %s\n", filename);
        return FALSE;
    }

    /* Write signature
    */
    WriteString(fp, "C64 tape image file", 32, 0, CP_ASCII);

    /* Write directory header
    */
    WriteWord(fp, 0x1010);
    WriteWord(fp, count);
    WriteWord(fp, count);
    WriteWord(fp, NOT_USED);
    WriteString(fp, filename, 24, ' ', CP_CBM);

    /* Offset to tape data
    */
    offset = 64 + 32 * count;

    /* Write directory entries
    */
    for(f = 0; f < count; f++)
    {
        WriteByte(fp, 1);
        WriteByte(fp, 0x82);
        WriteWord(fp, bank[f]->min_address_used);
        WriteWord(fp, bank[f]->max_address_used);
        WriteWord(fp, NOT_USED);
        WriteLong(fp, offset);
        WriteWord(fp, NOT_USED);
        WriteWord(fp, NOT_USED);

        offset += bank[f]->max_address_used - bank[f]->min_address_used + 1;
    }

    /* Write actual contents
    */
    for(f = 0; f < count; f++)
    {
        Byte *mem;
        int min, max, len;

        mem = bank[f]->memory;
        min = bank[f]->min_address_used;
        max = bank[f]->max_address_used;
        len = max - min + 1;

        fwrite(mem, len, 1, fp);
    }

    fclose(fp);

    return TRUE;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
