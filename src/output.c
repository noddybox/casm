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
    OPT_OUTPUTBANK,
    OPT_OUTPUTFORMAT
};

static const ValueTable option_set[] =
{
    {"output-file",     OPT_OUTPUTFILE},
    {"output-bank",     OPT_OUTPUTBANK},
    {"output-format",   OPT_OUTPUTFORMAT},
    {NULL}
};

typedef enum
{
    RAW,
    TAP,
    T64,
    ZX81,
    GAMEBOY,
    SNES,
    LIBRARY,
    NES,
    CPC,
    PRG,
    HEX
} Format;

static char             output[4096] = "output";
static char             output_bank[4096] = "output.%u";
static char             error[1024];
static Format           format = RAW;

static ValueTable       format_table[] =
{
    {"raw",             RAW},
    {"spectrum",        TAP},
    {"t64",             T64},
    {"zx81",            ZX81},
    {"gameboy",         GAMEBOY},
    {"snes",            SNES},
    {"lib",             LIBRARY},
    {"nes",             NES},
    {"cpc",             CPC},
    {"prg",             PRG},
    {"hex",             HEX},
    {NULL}
};


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

        case OPT_OUTPUTBANK:
            CopyStr(output_bank, argv[0], sizeof output_bank);
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
        case RAW:
            return RawOutput(output, output_bank, bank, count,
                             error, sizeof error);

        case TAP:
            return SpecTAPOutput(output, output_bank, bank, count,
                                 error, sizeof error);

        case T64:
            return T64Output(output, output_bank, bank, count,
                             error, sizeof error);

        case ZX81:
            return ZX81Output(output, output_bank, bank, count,
                              error, sizeof error);

        case GAMEBOY:
            return GBOutput(output, output_bank, bank, count,
                            error, sizeof error);

        case SNES:
            return SNESOutput(output, output_bank, bank, count,
                              error, sizeof error);

        case LIBRARY:
            return LibOutput(output, output_bank, bank, count,
                             error, sizeof error);

        case NES:
            return NESOutput(output, output_bank, bank, count,
                             error, sizeof error);

        case CPC:
            return CPCOutput(output, output_bank, bank, count,
                             error, sizeof error);

        case PRG:
            return PRGOutput(output, output_bank, bank, count,
                             error, sizeof error);

        case HEX:
            return HEXOutput(output, output_bank, bank, count,
                             error, sizeof error);

        default:
            break;
    }

    return FALSE;
}


const char *OutputError(void)
{
    return error;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
