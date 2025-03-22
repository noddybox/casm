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

    Gameboy ROM output handler.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "expr.h"
#include "codepage.h"
#include "gbout.h"


/* ---------------------------------------- MACROS & TYPES
*/

enum option_t
{
    OPT_COLOUR,
    OPT_SUPER,
    OPT_CART_RAM,
    OPT_CART_TYPE,
    OPT_IRQ
};

static const ValueTable option_set[] =
{
    {"gameboy-colour",          OPT_COLOUR},
    {"gameboy-color",           OPT_COLOUR},
    {"gameboy-super",           OPT_SUPER},
    {"gameboy-cart-ram",        OPT_CART_RAM},
    {"gameboy-cart-type",       OPT_CART_TYPE},
    {"gameboy-irq",             OPT_IRQ},
    {NULL}
};

typedef enum
{
    IRQ_VBLANK,
    IRQ_LCD,
    IRQ_TIMER,
    IRQ_SERIAL,
    IRQ_JOYPAD
} IRQ_Type;

static ValueTable       irq_table[] =
{
    {"vbl",             IRQ_VBLANK},
    {"lcd",             IRQ_LCD},
    {"timer",           IRQ_TIMER},
    {"serial",          IRQ_SERIAL},
    {"joypad",          IRQ_JOYPAD},
    {NULL}
};

static struct
{
    int         is_colour;
    int         is_super;
    int         cart_ram;
    int         cart_type;
    int         irq_vector[5];
} option =
{
    FALSE, FALSE, -1, -1,
    {-1, -1, -1, -1, -1}
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


static int PokeS(Byte *mem, int addr, const char *str, int maxlen)
{
    while(*str && maxlen--)
    {
        addr = PokeB(mem, addr, CodeFromNative(CP_ASCII, *str++));
    }

    return addr;
}


static int PokeCode(Byte *mem, int addr, const int *code)
{
    while(*code != -1)
    {
        addr = PokeB(mem, addr, *code++);
    }

    return addr;
}



/* ---------------------------------------- INTERFACES
*/
const ValueTable *GBOutputOptions(void)
{
    return option_set;
}

CommandStatus GBOutputSetOption(int opt, int argc, char *argv[],
                                int quoted[], char *err, size_t errsize)
{
    const ValueTable *val;
    int f;

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_COLOUR:
            option.is_colour = ParseTrueFalse(argv[0], TRUE);
            break;

        case OPT_SUPER:
            option.is_super = ParseTrueFalse(argv[0], TRUE);
            break;

        case OPT_CART_RAM:
            CMD_EXPR_INT(argv[0], option.cart_ram);
            break;

        case OPT_CART_TYPE:
            CMD_EXPR_INT(argv[0], option.cart_type);
            break;

        case OPT_IRQ:
            CMD_ARGC_CHECK(2);
            CMD_TABLE(argv[0], irq_table, val);
            CMD_EXPR_INT(argv[1], f);
            option.irq_vector[val->value] = f;
            break;

        default:
            break;
    }

    return CMD_OK;

}

int GBOutput(const char *filename, const char *filename_bank,
             const unsigned  *banks, int count, char *error, size_t error_size)
{
    static const int logo[] =
    {
        0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00,
        0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89,
        0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB,
        0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F,
        0xBB, 0xB9, 0x33, 0x3E, -1
    };

    FILE *fp = fopen(filename, "wb");
    int f;
    int offset;
    int rom_size;
    Word global_csum = 0;
    Byte hdr_csum = 0;
    Byte *mem;

    if (!fp)
    {
        snprintf(error, error_size, "Failed to create %s", filename);
        return FALSE;
    }

    /* Check bank sizes and layouts
    */
    if (count == 1 && (GetLowWriteMarker(banks[0]) < 0x150 ||
                        GetHighWriteMarker(banks[0]) > 0x7fff))
    {
        snprintf(error, error_size, "A simple ROM must be in the address "
                                        "space 0x150 to 0x7fff");
        return FALSE;
    }

    if (count > 1 && (GetLowWriteMarker(banks[0]) < 0x150 ||
                        GetHighWriteMarker(banks[0]) > 0x3fff))
    {
        snprintf(error, error_size, "Bank zero of a banked ROM must be in the "
                                        "address space 0x150 to 0x3fff");
        return FALSE;
    }

    for(f = 1 ; f < count; f++)
    {
        if (GetLowWriteMarker(banks[f]) < 0x4000 ||
               GetHighWriteMarker(banks[f]) > 0x7fff)
        {
            snprintf(error, error_size,
                        "Bank %u must be in the address space "
                                    "0x4000 to 0x7fff", banks[f]);
            return FALSE;
        }
    }

    if (option.cart_type == -1)
    {
        if (count == 1)
        {
            option.cart_type = 0;
        }
        else
        {
            option.cart_type = 1;
        }

        if (option.cart_ram != -1)
        {
            option.cart_type = 3;
        }
    }

    if (count == 1)
    {
        rom_size = 0;
    }
    else
    {
        rom_size = (count / 4) + 1;
    }

    mem = MemoryGetBlock(banks[f], 0, 0x10000);

    /* Create the log
    */
    for(f = 0; logo[f] != -1; f++)
    {
        PokeB(mem, 0x104 + f, logo[f]);
    }

    /* Create the RST handlers
    */
    for(f = 0; f < 8; f++)
    {
        int addr = f * 8;

        addr = PokeB(mem, addr, 0xc3);
        addr = PokeW(mem, addr, 0x100);
    }

    /* Create the IRQ handlers
    */
    for(f = 0; f < 5; f++)
    {
        int addr = (f * 8) + 0x40;

        if (option.irq_vector[f] == -1)
        {
            addr = PokeB(mem, addr, 0xd9);
        }
        else
        {
            addr = PokeB(mem, addr, 0xc3);
            addr = PokeW(mem, addr, option.irq_vector[f]);
        }
    }

    /* Create the entry point
    */
    PokeB(mem, 0x100, 0);
    PokeB(mem, 0x101, 0xc3);
    PokeW(mem, 0x102, GetLowWriteMarker(banks[0]));

    /* Title (new smaller size)
    */
    PokeS(mem, 0x134, filename, 11);

    /* GBC flag
    */
    if (option.is_colour)
    {
        PokeB(mem, 0x143, 0xc0);
    }

    /* Super Gameboy flag
    */
    if (option.is_super)
    {
        PokeB(mem, 0x146, 3);
    }
    else
    {
        PokeB(mem, 0x146, 0);
    }

    /* Type/ROM size
    */
    PokeB(mem, 0x147, option.cart_type);
    PokeB(mem, 0x148, rom_size);

    /* RAM size
    */
    switch(option.cart_ram)
    {
        case 2:
            PokeB(mem, 0x149, 1);
            break;

        case 8:
            PokeB(mem, 0x149, 2);
            break;

        case 32:
            PokeB(mem, 0x149, 3);
            break;

        case 128:
            PokeB(mem, 0x149, 4);
            break;

        case 64:
            PokeB(mem, 0x149, 5);
            break;

        default:
            PokeB(mem, 0x149, 0);
            break;
    }

    /* Non-Japanese ROM
    */
    PokeB(mem, 0x14a, 1);

    /* Old licensee
    */
    if (option.is_super)
    {
        PokeB(mem, 0x14b, 0x33);
    }
    else
    {
        PokeB(mem, 0x14b, 0);
    }

    /* Header checksum
    */
    hdr_csum = 0;

    for(f = 0x134 ; f < 0x14d; f++)
    {
        hdr_csum = hdr_csum - mem[f] - 1u;
    }

    PokeB(mem, 0x14d, hdr_csum);

    /* Global checksum
    */
    global_csum = 0;

    if (count == 1)
    {
        for(f = 0; f < 0x14e; f++)
        {
            global_csum += mem[f];
        }

        for(f = 0x150; f < 0x8000; f++)
        {
            global_csum += mem[f];
        }
    }
    else
    {
        int r;

        for(f = 0; f < 0x14e; f++)
        {
            global_csum += mem[f];
        }

        for(f = 0x150; f < 0x4000; f++)
        {
            global_csum += mem[f];
        }

        for(r = 1; r < count; r++)
        {
            for(f = 0x4000; f < 0x8000; f++)
            {
                global_csum += MemoryReadBank(banks[r], f);
            }
        }
    }

    PokeB(mem, 0x14e, global_csum >> 8);
    PokeB(mem, 0x14f, global_csum);

    /* Output the ROM contents
    */
    if (count == 1)
    {
        fwrite(mem, 0x8000, 1, fp);
    }
    else
    {
        int r;

        fwrite(mem, 0x4000, 1, fp);

        for(r = 1; r < count; r++)
        {
            Byte *bank = MemoryGetBlock(banks[r], 0x4000, 0x4000);

            fwrite(bank, 0x4000, 1, fp);

            free(bank);
        }
    }

    free(mem);

    fclose(fp);

    return TRUE;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
