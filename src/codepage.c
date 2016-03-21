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

    Code page handling.

*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "global.h"
#include "codepage.h"

/* ---------------------------------------- TYPES
*/

typedef struct
{
    int code;
    int result;
} CodepageDef;


enum option_t
{
    OPT_CODEPAGE
};


static const ValueTable option_set[] =
{
    {"codepage",        OPT_CODEPAGE},
    {"charset",         OPT_CODEPAGE},
    {NULL}
};


static const ValueTable codepage_table[] =
{
    {"ascii",           CP_ASCII},
    {"zx81",            CP_ZX81},
    {"spectrum",        CP_SPECTRUM},
    {"cbm",             CP_CBM},
    {NULL}
};



/* ---------------------------------------- GLOBALS
*/
static Codepage cp = CP_ASCII;

static CodepageDef cp_ascii[] =
{
    {' ', 0x20}, {'!', 0x21}, {'"', 0x22}, {'#', 0x23}, 
    {'$', 0x24}, {'%', 0x25}, {'&', 0x26}, {'\'', 0x27}, 
    {'(', 0x28}, {')', 0x29}, {'*', 0x2a}, {'+', 0x2b}, 
    {',', 0x2c}, {'-', 0x2d}, {'.', 0x2e}, {'/', 0x2f}, 
    {'0', 0x30}, {'1', 0x31}, {'2', 0x32}, {'3', 0x33}, 
    {'4', 0x34}, {'5', 0x35}, {'6', 0x36}, {'7', 0x37}, 
    {'8', 0x38}, {'9', 0x39}, {':', 0x3a}, {';', 0x3b}, 
    {'<', 0x3c}, {'=', 0x3d}, {'>', 0x3e}, {'?', 0x3f}, 
    {'@', 0x40}, {'A', 0x41}, {'B', 0x42}, {'C', 0x43}, 
    {'D', 0x44}, {'E', 0x45}, {'F', 0x46}, {'G', 0x47}, 
    {'H', 0x48}, {'I', 0x49}, {'J', 0x4a}, {'K', 0x4b}, 
    {'L', 0x4c}, {'M', 0x4d}, {'N', 0x4e}, {'O', 0x4f}, 
    {'P', 0x50}, {'Q', 0x51}, {'R', 0x52}, {'S', 0x53}, 
    {'T', 0x54}, {'U', 0x55}, {'V', 0x56}, {'W', 0x57}, 
    {'X', 0x58}, {'Y', 0x59}, {'Z', 0x5a}, {'[', 0x5b}, 
    {'\\', 0x5c}, {']', 0x5d}, {'^', 0x5e}, {'_', 0x5f}, 
    {'`', 0x60}, {'a', 0x61}, {'b', 0x62}, {'c', 0x63}, 
    {'d', 0x64}, {'e', 0x65}, {'f', 0x66}, {'g', 0x67}, 
    {'h', 0x68}, {'i', 0x69}, {'j', 0x6a}, {'k', 0x6b}, 
    {'l', 0x6c}, {'m', 0x6d}, {'n', 0x6e}, {'o', 0x6f}, 
    {'p', 0x70}, {'q', 0x71}, {'r', 0x72}, {'s', 0x73}, 
    {'t', 0x74}, {'u', 0x75}, {'v', 0x76}, {'w', 0x77}, 
    {'x', 0x78}, {'y', 0x79}, {'z', 0x7a}, {'{', 0x7b}, 
    {'|', 0x7c}, {'}', 0x7d}, {'~', 0x7e},
    {0, 0}
};


static CodepageDef cp_zx81[] =
{
    {' ', 0x00}, {'!', 0x00}, {'"', 0x0b}, {'#', 0x0c}, 
    {'$', 0x0d}, {'%', 0x00}, {'&', 0x00}, {'\'', 0x0b}, 
    {'(', 0x10}, {')', 0x11}, {'*', 0x17}, {'+', 0x15}, 
    {',', 0x1a}, {'-', 0x16}, {'.', 0x1b}, {'/', 0x24}, 
    {'0', 0x1c}, {'1', 0x1d}, {'2', 0x1e}, {'3', 0x1f}, 
    {'4', 0x20}, {'5', 0x21}, {'6', 0x22}, {'7', 0x23}, 
    {'8', 0x24}, {'9', 0x25}, {':', 0x0e}, {';', 0x19}, 
    {'<', 0x12}, {'=', 0x14}, {'>', 0x13}, {'?', 0x0f}, 
    {'@', 0x97}, {'A', 0xa6}, {'B', 0xa7}, {'C', 0xa8}, 
    {'D', 0xa9}, {'E', 0xaa}, {'F', 0xab}, {'G', 0xac}, 
    {'H', 0xad}, {'I', 0xae}, {'J', 0xaf}, {'K', 0xb0}, 
    {'L', 0xb1}, {'M', 0xb2}, {'N', 0xb3}, {'O', 0xb4}, 
    {'P', 0xb5}, {'Q', 0xb6}, {'R', 0xb7}, {'S', 0xb8}, 
    {'T', 0xb9}, {'U', 0xba}, {'V', 0xbb}, {'W', 0xbc}, 
    {'X', 0xbd}, {'Y', 0xbe}, {'Z', 0xbf}, {'[', 0x10}, 
    {'\\', 0x24}, {']', 0x11}, {'^', 0xde}, {'_', 0x80}, 
    {'`', 0x60}, {'a', 0x26}, {'b', 0x27}, {'c', 0x28}, 
    {'d', 0x29}, {'e', 0x2a}, {'f', 0x2b}, {'g', 0x2c}, 
    {'h', 0x2d}, {'i', 0x2e}, {'j', 0x2f}, {'k', 0x30}, 
    {'l', 0x31}, {'m', 0x32}, {'n', 0x33}, {'o', 0x34}, 
    {'p', 0x35}, {'q', 0x36}, {'r', 0x37}, {'s', 0x38}, 
    {'t', 0x39}, {'u', 0x3a}, {'v', 0x3b}, {'w', 0x3c}, 
    {'x', 0x3d}, {'y', 0x3e}, {'z', 0x3f}, {'{', 0x90}, 
    {'|', 0x00}, {'}', 0x91}, {'~', 0x96},
    {0, 0}
};


static CodepageDef cp_spectrum[] =
{
    {' ', 0x20}, {'!', 0x21}, {'"', 0x22}, {'#', 0x23}, 
    {'$', 0x24}, {'%', 0x25}, {'&', 0x26}, {'\'', 0x27}, 
    {'(', 0x28}, {')', 0x29}, {'*', 0x2a}, {'+', 0x2b}, 
    {',', 0x2c}, {'-', 0x2d}, {'.', 0x2e}, {'/', 0x2f}, 
    {'0', 0x30}, {'1', 0x31}, {'2', 0x32}, {'3', 0x33}, 
    {'4', 0x34}, {'5', 0x35}, {'6', 0x36}, {'7', 0x37}, 
    {'8', 0x38}, {'9', 0x39}, {':', 0x3a}, {';', 0x3b}, 
    {'<', 0x3c}, {'=', 0x3d}, {'>', 0x3e}, {'?', 0x3f}, 
    {'@', 0x40}, {'A', 0x41}, {'B', 0x42}, {'C', 0x43}, 
    {'D', 0x44}, {'E', 0x45}, {'F', 0x46}, {'G', 0x47}, 
    {'H', 0x48}, {'I', 0x49}, {'J', 0x4a}, {'K', 0x4b}, 
    {'L', 0x4c}, {'M', 0x4d}, {'N', 0x4e}, {'O', 0x4f}, 
    {'P', 0x50}, {'Q', 0x51}, {'R', 0x52}, {'S', 0x53}, 
    {'T', 0x54}, {'U', 0x55}, {'V', 0x56}, {'W', 0x57}, 
    {'X', 0x58}, {'Y', 0x59}, {'Z', 0x5a}, {'[', 0x5b}, 
    {'\\', 0x5c}, {']', 0x5d}, {'^', 0x5e}, {'_', 0x5f}, 
    {'`', 0x27}, {'a', 0x61}, {'b', 0x62}, {'c', 0x63}, 
    {'d', 0x64}, {'e', 0x65}, {'f', 0x66}, {'g', 0x67}, 
    {'h', 0x68}, {'i', 0x69}, {'j', 0x6a}, {'k', 0x6b}, 
    {'l', 0x6c}, {'m', 0x6d}, {'n', 0x6e}, {'o', 0x6f}, 
    {'p', 0x70}, {'q', 0x71}, {'r', 0x72}, {'s', 0x73}, 
    {'t', 0x74}, {'u', 0x75}, {'v', 0x76}, {'w', 0x77}, 
    {'x', 0x78}, {'y', 0x79}, {'z', 0x7a}, {'{', 0x7b}, 
    {'|', 0x7c}, {'}', 0x7d}, {'~', 0x7e},
    {0, 0}
};


static CodepageDef cp_cbm[] =
{
    {' ', 0x20}, {'!', 0x21}, {'"', 0x22}, {'#', 0x23}, 
    {'$', 0x24}, {'%', 0x25}, {'&', 0x26}, {'\'', 0x27}, 
    {'(', 0x28}, {')', 0x29}, {'*', 0x2a}, {'+', 0x2b}, 
    {',', 0x2c}, {'-', 0x2d}, {'.', 0x2e}, {'/', 0x2f}, 
    {'0', 0x30}, {'1', 0x31}, {'2', 0x32}, {'3', 0x33}, 
    {'4', 0x34}, {'5', 0x35}, {'6', 0x36}, {'7', 0x37}, 
    {'8', 0x38}, {'9', 0x39}, {':', 0x3a}, {';', 0x3b}, 
    {'<', 0x3c}, {'=', 0x3d}, {'>', 0x3e}, {'?', 0x3f}, 
    {'@', 0x40}, {'A', 0x41}, {'B', 0x42}, {'C', 0x43}, 
    {'D', 0x44}, {'E', 0x45}, {'F', 0x46}, {'G', 0x47}, 
    {'H', 0x48}, {'I', 0x49}, {'J', 0x4a}, {'K', 0x4b}, 
    {'L', 0x4c}, {'M', 0x4d}, {'N', 0x4e}, {'O', 0x4f}, 
    {'P', 0x50}, {'Q', 0x51}, {'R', 0x52}, {'S', 0x53}, 
    {'T', 0x54}, {'U', 0x55}, {'V', 0x56}, {'W', 0x57}, 
    {'X', 0x58}, {'Y', 0x59}, {'Z', 0x5a}, {'[', 0x5b}, 
    {'\\', 0x2f}, {']', 0x5d}, {'^', 0x5e}, {'_', 0x60}, 
    {'`', 0x27}, {'a', 0x41}, {'b', 0x42}, {'c', 0x43}, 
    {'d', 0x44}, {'e', 0x45}, {'f', 0x46}, {'g', 0x47}, 
    {'h', 0x48}, {'i', 0x49}, {'j', 0x4a}, {'k', 0x4b}, 
    {'l', 0x4c}, {'m', 0x4d}, {'n', 0x4e}, {'o', 0x4f}, 
    {'p', 0x50}, {'q', 0x51}, {'r', 0x52}, {'s', 0x53}, 
    {'t', 0x54}, {'u', 0x55}, {'v', 0x56}, {'w', 0x57}, 
    {'x', 0x58}, {'y', 0x59}, {'z', 0x5a}, {'{', 0x5b}, 
    {'|', 0x7d}, {'}', 0x5d}, {'~', 0x7e},
    {0, 0}
};

static CodepageDef *cp_table[] =
{
    cp_ascii,
    cp_zx81,
    cp_spectrum,
    cp_cbm
};


/* ---------------------------------------- INTERFACES
*/

const ValueTable *CodepageOptions(void)
{
    return option_set;
}


CommandStatus CodepageSetOption(int opt, int argc, char *argv[],
                                int quoted[], char *err, size_t errsize)
{
    const ValueTable *val;

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_CODEPAGE:
            CMD_TABLE(argv[0], codepage_table, val);
            cp = val->value;
            break;

        default:
            break;
    }

    return CMD_OK;
}



int CodepageConvert(int code)
{
    int f;

    for(f = 0; cp_table[cp][f].code; f++)
    {
        if (cp_table[cp][f].code == code)
        {
            return cp_table[cp][f].result;
        }
    }

    return 0;
}


int CodeFromNative(Codepage page, int code)
{
    int f;

    for(f = 0; cp_table[page][f].code; f++)
    {
        if (cp_table[page][f].code == code)
        {
            return cp_table[page][f].result;
        }
    }

    return 0;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
