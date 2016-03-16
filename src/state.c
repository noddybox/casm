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

    Stores assembly state.

*/
#include <stdlib.h>
#include <string.h>

#include "global.h"
#include "state.h"
#include "util.h"
#include "expr.h"


/* ---------------------------------------- TYPES AND GLOBALS
*/
#define BANK_SIZE       0x10000u

static int      pass = 1;
static int      maxpass = 2;
static int      pc = 0;
static int      num_banks = 0;
static unsigned currbank = 0;
static WordMode wmode = LSB_Word;

static MemoryBank       **bank;
static MemoryBank       *current;

/* ---------------------------------------- PRIVATE
*/
static int SortBank(const void *a, const void *b)
{
    const MemoryBank *ma = a;
    const MemoryBank *mb = b;

    return (int)ma->number - (int)mb->number;
}


static void RemoveBanks(void)
{
    int f;

    for(f = 0; f < num_banks; f++)
    {
        free(bank[f]);
    }

    free(bank);

    currbank = 0;
    bank = NULL;
    current = NULL;
}

static MemoryBank *FindBank(unsigned n)
{
    int f;

    for(f = 0; f < num_banks; f++)
    {
        if (bank[f]->number == n)
        {
            return bank[f];
        }
    }

    return NULL;
}

static void ClearBankWriteMarkers(void)
{
    int f;

    for(f = 0; f < num_banks; f++)
    {
        bank[f]->min_address_used = BANK_SIZE;
        bank[f]->max_address_used = -1;
    }
}

static MemoryBank *AddBank(unsigned n)
{
    current = NULL;
    num_banks++;

    bank = Realloc(bank, (sizeof *bank) * num_banks);
    bank[num_banks-1] = Malloc(sizeof **bank);
    bank[num_banks-1]->number = n;

    qsort(bank, num_banks, sizeof *bank, SortBank);

    return FindBank(n);
}

static MemoryBank *GetOrAddBank(unsigned n)
{
    MemoryBank *b = FindBank(n);

    if (!b)
    {
        b = AddBank(n);
    }

    return b;
}

/* ---------------------------------------- INTERFACES
*/

void ClearState(void)
{
    pass = 1;
    pc = 0;
    wmode = LSB_Word;
    RemoveBanks();
    SetAddressBank(0);
}


void NextPass(void)
{
    if (pass < maxpass)
    {
        ClearBankWriteMarkers();
        pass++;
    }
}


int IsFinalPass(void)
{
    return pass == maxpass;
}


int IsFirstPass(void)
{
    return pass == 1;
}


int IsIntermediatePass(void)
{
    return pass > 1 && pass < maxpass;
}


void SetNeededPasses(int n)
{
    if (!IsFinalPass())
    {
        maxpass = n;
    }
}


void SetAddressBank(unsigned b)
{
    currbank = b;
    current = NULL;
}


void SetWordMode(WordMode mode)
{
    wmode = mode;
}


void SetPC(int i)
{
    pc = i;
}


int PC(void)
{
    return pc;
}


void PCAdd(int i)
{
    pc += i;

    while(pc < 0)
    {
        pc += BANK_SIZE;
    }

    pc %= BANK_SIZE;
}


void PCWrite(int i)
{
    if (!current)
    {
        current = GetOrAddBank(currbank);
    }

    if (pc < current->min_address_used)
    {
        current->min_address_used = pc;
    }

    if (pc > current->max_address_used)
    {
        current->max_address_used = pc;
    }

    current->memory[pc] = ExprConvert(8, i);

    pc = (pc + 1) % BANK_SIZE;
}


void PCWriteWord(int i)
{
    PCWriteWordMode(i, wmode);
}


void PCWriteWordMode(int i, WordMode mode)
{
    int w;
    int lsb;
    int msb;

    w = ExprConvert(16, i);

    lsb = LOBYTE(w);
    msb = HIBYTE(w);

    switch(mode)
    {
        case MSB_Word:
            PCWrite(msb);
            PCWrite(lsb);
            break;

        case LSB_Word:
            PCWrite(lsb);
            PCWrite(msb);
            break;
    }
}


MemoryBank **MemoryBanks(int *count)
{
    *count = num_banks;
    return bank;
}


Byte ReadByte(int addr)
{
    Byte b = 0;

    if (!current)
    {
        current = GetOrAddBank(currbank);
    }

    if (addr > -1 && addr < BANK_SIZE && current)
    {
        b = current->memory[addr];
    }

    return b;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
