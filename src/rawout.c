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
#include "rawout.h"


/* ---------------------------------------- INTERFACES
*/
const ValueTable *RawOutputOptions(void)
{
    return NULL;
}

CommandStatus RawOutputSetOption(int opt, int argc, char *argv[],
                                 int quoted[], char *error, size_t error_size)
{
    return CMD_NOT_KNOWN;
}

int RawOutput(const char *filename, const char *filename_bank,
              MemoryBank **bank, int count, char *error, size_t error_size)
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
            name = filename;
        }
        else
        {
            snprintf(buff, sizeof buff, filename_bank, bank[f]->number);
            name = buff;
        }

        if (!(fp = fopen(name, "wb")))
        {
            snprintf(error, error_size, "Failed to open %s", name);
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


/*
vim: ai sw=4 ts=8 expandtab
*/
