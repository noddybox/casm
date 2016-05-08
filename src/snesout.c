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

    SNES ROM output handler.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "expr.h"
#include "codepage.h"
#include "snesout.h"


/* ---------------------------------------- MACROS & TYPES
*/

enum option_t
{
    OPT_ROM_TYPE,
    OPT_IRQ,
    OPT_NAME,
    OPT_START,
    OPT_RAM_SIZE,
    OPT_ROM_SIZE,
};

static const ValueTable option_set[] =
{
    {"snes-rom-type",           OPT_ROM_TYPE},
    {"snes-irq",                OPT_IRQ},
    {"snes-name",               OPT_NAME},
    {"snes-start",              OPT_START},
    {"snes-ram-size",           OPT_RAM_SIZE},
    {"snes-rom-size",           OPT_ROM_SIZE},
    {NULL}
};

typedef enum
{
    ROM_LOROM = 0x00,
    ROM_HIROM = 0x01,
    ROM_LOROM_FAST = 0x30 | ROM_LOROM,
    ROM_HIROM_FAST = 0x30 | ROM_HIROM
} ROM_Type;

static ValueTable       rom_table[] =
{
    {"lorom",           ROM_LOROM},
    {"hirom",           ROM_HIROM},
    {"lorom-fast",      ROM_LOROM_FAST},
    {"hirom-fast",      ROM_HIROM_FAST},
    {NULL}
};

typedef enum
{
    IRQ_VBLANK,
    IRQ_IRQ
} IRQ_Type;

static ValueTable       irq_table[] =
{
    {"vbl",             IRQ_VBLANK},
    {"irq",             IRQ_IRQ},
    {NULL}
};


static struct
{
    ROM_Type    rom_type;
    int         irq_vector[2];
    char        name[22];
    int         start;
    int         ram_size;
    int         rom_size;
} option =
{
    ROM_LOROM, {-1, -1}, "NONAME", 0x8000, 0, -1
};


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


static int PokeS(Byte *mem, int addr, const char *str, int maxlen, char pad)
{
    while(*str && maxlen--)
    {
        addr = PokeB(mem, addr, CodeFromNative(CP_ASCII, *str++));
    }

    while(maxlen-- > 0)
    {
        addr = PokeB(mem, addr, CodeFromNative(CP_ASCII, pad));
    }

    return addr;
}

static unsigned CalcChecksum(Byte *p, int len, unsigned csum)
{
    while(len-- > 0)
    {
        csum += *p++;
    }

    return csum & 0xffff;
}


/* ---------------------------------------- INTERFACES
*/
const ValueTable *SNESOutputOptions(void)
{
    return option_set;
}

CommandStatus SNESOutputSetOption(int opt, int argc, char *argv[],
                                  int quoted[], char *err, size_t errsize)
{
    const ValueTable *val;
    int f;

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_ROM_TYPE:
            CMD_TABLE(argv[0], rom_table, val);
            option.rom_type = val->value;
            break;

        case OPT_IRQ:
            CMD_ARGC_CHECK(2);
            CMD_TABLE(argv[0], irq_table, val);
            CMD_EXPR(argv[1], f);
            option.irq_vector[val->value] = f;
            break;

        case OPT_NAME:
            CopyStr(option.name, argv[1], sizeof option.name);
            break;

        case OPT_START:
            CMD_EXPR(argv[0], f);
            option.start = f;
            break;

        case OPT_ROM_SIZE:
            CMD_EXPR(argv[0], f);
            option.rom_size = f;
            break;

        case OPT_RAM_SIZE:
            CMD_EXPR(argv[0], f);
            option.ram_size = f;
            break;

        default:
            break;
    }

    return CMD_OK;

}

int SNESOutput(const char *filename, const char *filename_bank,
              MemoryBank **bank, int count, char *error, size_t error_size)
{
    FILE *fp;
    Byte *mem;
    int base;
    int len;
    int f;
    unsigned csum;

    /* If the ROM type is LOROM then we assume each bank holds 32Kb.  Otherwise
       each bank is a full 64Kb.
    */
    if (option.rom_type == ROM_LOROM || option.rom_type == ROM_LOROM_FAST)
    {
        for(f = 0; f < count; f++)
        {
            if (bank[f]->min_address_used < 0x8000)
            {
                snprintf(error, error_size, "Bank %u uses memory below 0x8000",
                                                bank[f]->number);
                return FALSE;
            }
        }

        base = 0x8000;
        len = 0x8000;
    }
    else
    {
        base = 0;
        len = 0x10000;
    }

    if (!(fp = fopen(filename, "wb")))
    {
        snprintf(error, error_size, "Failed to create %s", filename);
        return FALSE;
    }

    /* Setup ROM header
    */
    mem = bank[0]->memory;

    PokeS(mem, 0xffc0, option.name, 21, ' ');

    PokeB(mem, 0xffd5, option.rom_type);

    PokeW(mem, 0xfffc, option.start);

    if (option.irq_vector[IRQ_VBLANK] != -1)
    {
        PokeW(mem, 0xffea, option.irq_vector[IRQ_VBLANK]);
        PokeW(mem, 0xfffa, option.irq_vector[IRQ_VBLANK]);
    }
    else
    {
        fprintf(stderr, "WARNING: VBLANK IRQ not set\n");
    }

    if (option.irq_vector[IRQ_IRQ] != -1)
    {
        PokeW(mem, 0xffee, option.irq_vector[IRQ_IRQ]);
        PokeW(mem, 0xfffe, option.irq_vector[IRQ_IRQ]);
    }

    /* TODO: What goes in 0xffd6 - ROM type? */

    if (option.rom_size == -1)
    {
        if (option.rom_type == ROM_LOROM || option.rom_type == ROM_LOROM_FAST)
        {
            PokeB(mem, 0xffd7, count * 32);
        }
        else
        {
            PokeB(mem, 0xffd7, count * 64);
        }
    }
    else
    {
        PokeB(mem, 0xffd7, option.rom_size);
    }

    PokeB(mem, 0xffd8, option.ram_size);

    /* Calculate checksum
    */
    csum = 0;

    PokeW(mem, 0xffdc, 0xffff);

    for(f = 0; f < count; f++)
    {
        csum = CalcChecksum(bank[f]->memory + base, len, csum);
    }

    PokeW(mem, 0xffde, csum);
    PokeW(mem, 0xffdc, csum ^ 0xffff);

    /* Output ROM contents
    */
    for(f = 0; f < count; f++)
    {
        fwrite(bank[f]->memory + base, len, 1, fp);
    }

    fclose(fp);

    return TRUE;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
