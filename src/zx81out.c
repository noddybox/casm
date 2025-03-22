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

    ZX81 tape output handler.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "codepage.h"
#include "zx81out.h"


/* ---------------------------------------- MACROS & TYPES
*/

enum option_t
{
    OPT_MARGIN,
    OPT_AUTORUN,
    OPT_DFILE_COLLAPSED
};

static const ValueTable option_set[] =
{
    {"zx81-margin",             OPT_MARGIN},
    {"zx81-autorun",            OPT_AUTORUN},
    {"zx81-collapse-dfile",     OPT_DFILE_COLLAPSED},
    {NULL}
};

typedef enum
{
    PAL = 55,
    NTSC = 31
} TVFormat;

static ValueTable       format_table[] =
{
    {"pal",             PAL},
    {"NTSC",            NTSC},
    {NULL}
};

static TVFormat         tv_format = PAL;
static int              run = TRUE;
static int              collapse_dfile = FALSE;

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


/* ---------------------------------------- INTERFACES
*/
const ValueTable *ZX81OutputOptions(void)
{
    return option_set;
}

CommandStatus ZX81OutputSetOption(int opt, int argc, char *argv[],
                                     int quoted[],
                                     char *err, size_t errsize)
{
    const ValueTable *val;

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_MARGIN:
            CMD_TABLE(argv[0], format_table, val);
            tv_format = val->value;
            break;

        case OPT_AUTORUN:
            run = ParseTrueFalse(argv[0], TRUE);
            break;

        case OPT_DFILE_COLLAPSED:
            collapse_dfile = ParseTrueFalse(argv[0], TRUE);
            break;

        default:
            break;
    }

    return CMD_OK;
}

int ZX81Output(const char *filename, const char *filename_bank,
               const unsigned *banks, int count, char *error, size_t error_size)
{
    static const int line[] =
    {
        0, 10, 14, 0, 0xf9, 0xd4, 0x1d, 0x22, 0x21, 0x1d,
        0x20, 0x7e, 0x8f, 0x01, 0x04, 0x00, 0x00, 0x76, -1
    };

    FILE *fp = fopen(filename, "wb");
    Byte *mem;
    int min;
    int max;
    int len;
    int next;
    int dfile;
    int vars;
    int addr;
    int f;

    mem = MemoryGetBlock(banks[0], 0, 0x10000);
    min = GetLowWriteMarker(banks[0]);
    max = GetHighWriteMarker(banks[0]);

    len = max - min + 1;

    if (min != 16514)
    {
        snprintf(error, error_size, "Code must start at 16514 to work with the "
                                        "ZX81 output driver.");
        return FALSE;
    }

    if (!fp)
    {
        snprintf(error, error_size, "Failed to create %s", filename);
        return FALSE;
    }

    /* Create the system variables and BASIC program around the supplied code.
       BASIC is done first to calculate various system variables.
    */
    addr = 16509;

    /* Program line with REM statement
    */
    addr = PokeW(mem, addr, 0x0000);            /* Line number  */
    addr = PokeW(mem, addr, len + 2);           /* Line length  */
    addr = PokeB(mem, addr, 0xea);              /* REM token    */

    addr += len;                                /* Skip code    */

    addr = PokeB(mem, addr, 0x76);              /* NL token     */

    /* Program line to launch the code
    */
    next = addr;

    f = 0;
    while (line[f] != -1)
    {
    	addr = PokeB(mem, addr, line[f++]);
    }

    /* Display file
    */
    dfile = addr;

    if (collapse_dfile)
    {
        for(f = 0; f < 25; f++)
        {
            addr = PokeB(mem, addr, 0x76);
        }
    }
    else
    {
        addr = PokeB(mem, addr, 0x76);

        for(f = 0; f < 24; f++)
        {
            int n;

            for(n = 0; n < 32; n++)
            {
                addr = PokeB(mem, addr, 0);
            }

            addr = PokeB(mem, addr, 0x76);
        }
    }

    /* Vars
    */
    vars = addr;
    addr = PokeB(mem, addr, 0x80);

    /* System variables
    */
    addr = 0x4009;

    addr = PokeB(mem, addr, 0);                 /* VERSN        */
    addr = PokeW(mem, addr, 0);                 /* E_PPC        */
    addr = PokeW(mem, addr, dfile);             /* D_FILE       */
    addr = PokeW(mem, addr, dfile + 1);         /* DF_CC        */
    addr = PokeW(mem, addr, vars);              /* VARS         */
    addr = PokeW(mem, addr, 0);                 /* DEST         */
    addr = PokeW(mem, addr, vars + 1);          /* E_LINE       */
    addr = PokeW(mem, addr, vars - 1);          /* CH_ADD       */
    addr = PokeW(mem, addr, 0);                 /* X_PTR        */
    addr = PokeW(mem, addr, vars + 5);          /* STKBOT       */
    addr = PokeW(mem, addr, vars + 5);          /* STKEND       */
    addr = PokeB(mem, addr, 0);                 /* BREG         */
    addr = PokeW(mem, addr, 16477);             /* MEM          */
    addr = PokeB(mem, addr, 0);                 /* unused       */
    addr = PokeB(mem, addr, 2);                 /* DF_SZ        */
    addr = PokeW(mem, addr, 2);                 /* S_TOP        */
    addr = PokeW(mem, addr, 0xffff);            /* LAST_K       */
    addr = PokeB(mem, addr, 0xff);              /* LAST_K       */
    addr = PokeB(mem, addr, tv_format);         /* MARGIN       */
    addr = PokeW(mem, addr, run ? next : dfile);/* NXTLIN       */
    addr = PokeW(mem, addr, 0);                 /* OLDPPC       */
    addr = PokeB(mem, addr, 0);                 /* FLAGX        */
    addr = PokeW(mem, addr, 0);                 /* STRLEN       */
    addr = PokeW(mem, addr, 0xc8d);             /* T_ADDR       */
    addr = PokeW(mem, addr, 0);                 /* SEED         */
    addr = PokeW(mem, addr, 0x0ffff);           /* FRAMES       */
    addr = PokeW(mem, addr, 0);                 /* COORDS       */
    addr = PokeB(mem, addr, 0xbc);              /* PR_CC        */
    addr = PokeB(mem, addr, 33);                /* S_POSN       */
    addr = PokeB(mem, addr, 24);                /* S_POSN       */
    addr = PokeW(mem, addr, 0x40);              /* CDFLAG       */

    /* Write the constructed P file
    */
    fwrite(mem + 0x4009, vars - 0x4009 + 1, 1, fp);

    fclose(fp);

    return TRUE;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
