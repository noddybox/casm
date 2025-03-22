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

    Intel HEX output handler.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "codepage.h"
#include "hexout.h"
#include "expr.h"


/* ---------------------------------------- MACROS & TYPES
*/

/* ---------------------------------------- PRIVATE TYPES AND VARS
*/
enum option_t
{
    OPT_NULL_BYTE
};

static const ValueTable option_set[]=
{
    {"hex-null", OPT_NULL_BYTE},
    {NULL}
};

typedef struct
{
    int		null_byte;
} Options;

static Options options =
{
    0
};


/* ---------------------------------------- PRIVATE FUNCTIONS
*/

/* ---------------------------------------- INTERFACES
*/
const ValueTable *HEXOutputOptions(void)
{
    return option_set;
}

CommandStatus HEXOutputSetOption(int opt, int argc, char *argv[],
                                 int quoted[], char *err, size_t errsize)
{
    CommandStatus stat = CMD_OK;

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_NULL_BYTE:
            CMD_EXPR_INT(argv[0], options.null_byte);
            break;

        default:
            break;
    }

    return stat;
}

int HEXOutput(const char *filename, const char *filename_bank,
              const unsigned *banks, int count, char *error, size_t error_size)
{
    int f;

    for(f = 0; f < count; f++)
    {
        FILE *fp;
        char buff[4096];
        const char *name;
        int r;

        if (count == 1)
        {
            name = filename;
        }
        else
        {
            snprintf(buff, sizeof buff, filename_bank, banks[f]);
            name = buff;
        }

        if (!(fp = fopen(name, "wb")))
        {
            snprintf(error, error_size, "Failed to open %s", name);
            return FALSE;
        }

        for(r = 0; r < 0xffff; r += 16)
        {
            int n;
            int found = 0;

            for(n = 0; n < 16 && !found; n++)
            {
                if (MemoryReadBank(banks[f], r+n) != options.null_byte)
                {
                    found = 1;
                }
            }

            if (found)
            {
                Byte csum = 0;

                fprintf(fp, ":10%4.4X00", r);

                for(n = 0; n < 16; n++)
                {
                    Byte b = MemoryReadBank(banks[f], r + n);

                    fprintf(fp, "%2.2X", (unsigned)b);
                    csum += b;
                }

                csum = ~csum;
                csum++;

                fprintf(fp, "%2.2X\n", csum);
            }
        }

        fprintf(fp, ":00000001FF\n");
        fclose(fp);
    }

    return TRUE;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
