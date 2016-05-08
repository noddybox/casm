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

    LIB binary file output

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "libout.h"
#include "label.h"


/* Magic value for library file
*/
#define CASM_LIBRARY_MAGIC      "CASMLIBv1%"  
#define CASM_LIBRARY_MAGIC_LEN  10                


/* ---------------------------------------- PRIVATE INTERFACES
*/

static void WriteNumber(FILE *fp, int num)
{
    fprintf(fp, "%.11d", num);
}


static int ReadNumber(FILE *fp)
{
    char buff[12];
    int f;

    for(f= 0 ;f < 11; f++)
    {
        buff[f] = getc(fp);
    }

    buff[f] = 0;

    return atoi(buff);
}


/* ---------------------------------------- INTERFACES
*/
const ValueTable *LibOutputOptions(void)
{
    return NULL;
}

CommandStatus LibOutputSetOption(int opt, int argc, char *argv[],
                                 int quoted[], char *error, size_t error_size)
{
    return CMD_NOT_KNOWN;
}

int LibOutput(const char *filename, const char *filename_bank,
              MemoryBank **bank, int count, char *error, size_t error_size)
{
    FILE *fp;
    char buff[4096];
    int f;

    if (!(fp = fopen(filename, "wb")))
    {
        snprintf(error, error_size, "Failed to open %s", filename);
        return FALSE;
    }

    fwrite(CASM_LIBRARY_MAGIC, 1, CASM_LIBRARY_MAGIC_LEN, fp);

    WriteNumber(fp, count);

    for(f = 0; f < count; f++)
    {
        const Byte *mem;
        int min, max, len;

        mem = bank[f]->memory;
        min = bank[f]->min_address_used;
        max = bank[f]->max_address_used;
        len = max - min + 1;

        WriteNumber(fp, (int)bank[f]->number);
        WriteNumber(fp, min);
        WriteNumber(fp, len);

        fwrite(mem + min, 1, len, fp);
    }

    LabelWriteBlob(fp);

    fclose(fp);

    return TRUE;
}


int LibLoad(const char *filename, LibLoadOption opt,
            char *error, size_t error_size)
{
    char magic[CASM_LIBRARY_MAGIC_LEN + 1] = {0};
    FILE *fp;
    Byte buff[0x10000];
    int count;
    int f;

    if (!(fp = fopen(filename, "rb")))
    {
        snprintf(error, error_size, "Failed to open %s", filename);
        return FALSE;
    }

    fread(magic, 1, CASM_LIBRARY_MAGIC_LEN, fp);

    if (strcmp(magic, CASM_LIBRARY_MAGIC) != 0)
    {
        snprintf(error, error_size, "%s not a recognised library", filename);
        fclose(fp);
        return FALSE;
    }

    count = ReadNumber(fp);

    for(f = 0; f < count; f++)
    {
        int bank;
        int min;
        int len;
        int old_pc;
        Byte *p;

        bank = ReadNumber(fp);
        min = ReadNumber(fp);
        len = ReadNumber(fp);

        SetAddressBank(bank);

        fread(buff, 1, len, fp);

        old_pc = PC();

        SetPC(min);
        p = buff;

        while(len-- > 0)
        {
            PCWrite(*p++);
        }

        SetPC(old_pc);
    }

    LabelReadBlob(fp);

    fclose(fp);

    return TRUE;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
