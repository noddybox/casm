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

    Various output type handlers.

*/
#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "output.h"


/* ---------------------------------------- GLOBALS
*/

enum option_t
{
    OPT_OUTPUTFILE,
    OPT_OUTPUTFORMAT
};

static const ValueTable option_set[] =
{
    {"output-file",     OPT_OUTPUTFILE},
    {"output-format",   OPT_OUTPUTFORMAT},
    {NULL}
};

typedef enum
{
    Raw,
    SpectrumTap
} Format;

static char             output[4096] = "output";
static char             error[1024];
static Format           format = Raw;

static ValueTable       format_table[] =
{
    {"raw",             Raw},
    {"spectrum",        SpectrumTap},
    {NULL}
};


/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static int OutputRawBinary(MemoryBank **bank, int count)
{
    char buff[4096];
    int f;

    for(f = 0; f < count; f++)
    {
        FILE *fp;
        const char *name;
        const Byte *mem;
        int min, max;

        if (count == 1)
        {
            name = output;
        }
        else
        {
            snprintf(buff, sizeof buff, "%s.%u", output, bank[f]->number);
            name = buff;
        }

        if (!(fp = fopen(name, "wb")))
        {
            snprintf(error, sizeof error,"Failed to open %s\n", name);
            return FALSE;
        }

        mem = bank[f]->memory;
        min = bank[f]->min_address_used;
        max = bank[f]->max_address_used;

        fwrite(mem + min, 1, max - min + 1, fp);

        fclose(fp);
    }

    return TRUE;
}


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


static int OutputSpectrumTap(MemoryBank **bank, int count)
{
    FILE *fp = fopen(output, "wb");
    int f;

    if (!fp)
    {
        snprintf(error, sizeof error,"Failed to open %s\n", output);
        return FALSE;
    }

    /* Output header
    */
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
        chk = TapString(fp, output, 10, chk);
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

/* ---------------------------------------- INTERFACES
*/

const ValueTable *OutputOptions(void)
{
    return option_set;
}


CommandStatus OutputSetOption(int opt, int argc, char *argv[],
                              int quoted[], char *err, size_t errsize)
{
    const ValueTable *val;

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_OUTPUTFILE:
            CopyStr(output, argv[0], sizeof output);
            break;

        case OPT_OUTPUTFORMAT:
            CMD_TABLE(argv[0], format_table, val);
            format = val->value;
            break;

        default:
            break;
    }

    return CMD_OK;
}


int OutputCode(void)
{
    MemoryBank **bank;
    int count;
    int min;
    int max;
    const Byte *mem;

    bank = MemoryBanks(&count);

    if (!bank)
    {
        fprintf(stderr, "Skipping output; no written memory to write\n");
        return TRUE;
    }

    switch(format)
    {
        case Raw:
            return OutputRawBinary(bank, count);

        case SpectrumTap:
            return OutputSpectrumTap(bank, count);

        default:
            break;
    }
}


const char *OutputError(void)
{
    return error;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
