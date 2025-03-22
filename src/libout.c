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
#define CASM_LIBRARY_MAGIC      "CASMLIBv2%"  
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


static void WriteUlong(FILE *fp, ulong num)
{
    fprintf(fp, "%.8lx", num);
}


static ulong ReadUlong(FILE *fp)
{
    char buff[9];
    int f;

    for(f= 0 ;f < 8; f++)
    {
        buff[f] = getc(fp);
    }

    buff[f] = 0;

    return strtoul(buff, NULL, 16);
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
              const unsigned *banks, int count, char *error, size_t error_size)
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
        Byte *mem;
        ulong min, max, len;

        min = GetLowWriteMarker(banks[f]);
        max = GetHighWriteMarker(banks[f]);
        len = max - min + 1;

        mem = MemoryGetBlock(banks[f], min, len);

        WriteUlong(fp, banks[f]);
        WriteUlong(fp, min);
        WriteUlong(fp, len);

        fwrite(mem, 1, len, fp);

        free(mem);
    }

    LabelWriteBlob(fp);

    fclose(fp);

    return TRUE;
}


int LibLoad(const char *filename, LibLoadOption opt, int offset,
            char *error, size_t error_size)
{
    char magic[CASM_LIBRARY_MAGIC_LEN + 1] = {0};
    FILE *fp;
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
        unsigned bank;
        ulong min;
        ulong len;

        bank = ReadUlong(fp);
        min = ReadUlong(fp);
        len = ReadUlong(fp);

        if (opt != LibLoadLabels)
        {
            while(len-- > 0)
            {
                MemoryWriteBank(bank, min + offset + len, fgetc(fp));
            }
        }
    }

    if (opt != LibLoadMemory)
    {
        LabelReadBlob(fp, offset);
    }

    fclose(fp);

    return TRUE;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
