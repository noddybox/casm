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
#include <stdarg.h>

#include "global.h"
#include "specout.h"
#include "expr.h"


/* ---------------------------------------- PRIVATE TYPES
*/
enum option_t
{
    OPT_LOADER,
    OPT_START_ADDR,
};

static const ValueTable option_set[] =
{
    {"spectrum-loader", OPT_LOADER},
    {"spectrum-start",  OPT_START_ADDR},
    {NULL}
};

typedef struct
{
    int         loader;
    int         start_addr;
} Options;


/* ---------------------------------------- PRIVATE DATA
*/

/* Types for AddLine()
*/
#define TYPE_END        0
#define TYPE_TOKEN      1
#define TYPE_STRING     2

/* Constants for some tokens
*/
#define TOK_VAL         176
#define TOK_QUOTE       34
#define TOK_CODE        175
#define TOK_USR         192
#define TOK_LOAD        239
#define TOK_RAND        249
#define TOK_CLEAR       253


static Options options =
{
    FALSE,
    -1
};

/* Buffers used for BASIC loader
*/
static unsigned char    basic[0x10000];
static unsigned         endptr = 0;
static unsigned         line_no = 10;


/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static void Poke(unsigned char b, int *len)
{
    if (len)
    {
        (*len)++;
    }

    basic[endptr++]=b;
}


static void AddBASICLine(int type, ...)
{
    va_list va;
    int len;
    unsigned char *len_ptr;

    Poke(line_no>>8, NULL);
    Poke(line_no&0xff, NULL);
    line_no += 10;

    len_ptr = basic + endptr;
    endptr += 2;
    len = 0;

    va_start(va,type);

    while(type!=TYPE_END)
    {
        char *str;

        switch(type)
        {
            case TYPE_TOKEN:
                Poke(va_arg(va,int), &len);
                break;

            case TYPE_STRING:
                str = va_arg(va,char *);

                Poke(TOK_QUOTE, &len);

                while(*str)
                {
                    Poke(*str++, &len);
                }

                Poke(TOK_QUOTE, &len);

                break;

            default:
                break;
        }

        type = va_arg(va,int);
    }

    Poke(0x0d, &len);

    *len_ptr++ = len&0xff;
    *len_ptr = len>>8;
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


static unsigned char TapStream(FILE *fp, const unsigned char *p,
                               unsigned addr, unsigned len, unsigned char chk)
{
    while(len--)
    {
        chk = TapByte(fp, p[addr], chk);
        addr = (addr + 1) & 0xffff;
    }

    return chk;
}

/* ---------------------------------------- INTERFACES
*/
const ValueTable *SpecTAPOutputOptions(void)
{
    return option_set;
}

CommandStatus SpecTAPOutputSetOption(int opt, int argc, char *argv[],
                                     int quoted[],
                                     char *err, size_t errsize)
{
    CommandStatus stat = CMD_OK;

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_LOADER:
            options.loader = ParseTrueFalse(argv[0], FALSE);
            break;

        case OPT_START_ADDR:
            CMD_EXPR(argv[0], options.start_addr);
            break;

        default:
            break;
    }

    return stat;
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

    /* First get the initial address is none defined
    */
    if (options.loader && options.start_addr == -1)
    {
        options.start_addr = bank[0]->min_address_used;
    }

    /* Output the BASIC loader if set
    */
    if (options.loader)
    {
        char no[64];
        Byte chk = 0;

        snprintf(no, sizeof no, "%u", options.start_addr);

        /* CLEAR VAL "start_addr"
        */
        AddBASICLine(TYPE_TOKEN, TOK_CLEAR,
                     TYPE_TOKEN, TOK_VAL,
                     TYPE_STRING, no,
                     TYPE_END);

        /* LOAD "" CODE
        */
        for(f = 0; f < count; f++)
        {
            AddBASICLine(TYPE_TOKEN, TOK_LOAD,
                         TYPE_TOKEN, TOK_QUOTE,
                         TYPE_TOKEN, TOK_QUOTE,
                         TYPE_TOKEN, TOK_CODE,
                         TYPE_END);
        }

        /* RANDOMIZE USR VAL "start_addr"
        */
        AddBASICLine(TYPE_TOKEN, TOK_RAND,
                     TYPE_TOKEN, TOK_USR,
                     TYPE_TOKEN, TOK_VAL,
                     TYPE_STRING, no,
                     TYPE_END);

        TapWord(fp, 19, 0);
        chk = TapByte(fp, 0, chk);
        chk = TapByte(fp, 0, chk);
        chk = TapString(fp, "LOADER.BAS", 10, chk);
        chk = TapWord(fp, endptr, chk);
        chk = TapWord(fp, 10, chk);
        chk = TapWord(fp, endptr, chk);

        TapByte(fp, chk, 0);

        TapWord(fp, endptr + 2, 0);

        chk = 0;

        chk = TapByte(fp, 0xff, chk);
        chk = TapStream(fp, basic, 0, endptr, chk);
        TapByte(fp, chk, 0);
    }

    /* Output the binary files
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
