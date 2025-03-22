/*

    casm - Simple, portable assembler

    Copyright (C) 2003-2025  Ian Cowburn (ianc@noddybox.co.uk)

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

    Stores assembly memory.

*/
#include <stdlib.h>
#include <stdio.h>

#include "global.h"
#include "memory.h"
#include "expr.h"
#include "util.h"


/* ---------------------------------------- TYPES AND GLOBALS
*/
#define PAGE_SIZE       1024

typedef struct
{
    ulong       base_address;
    Byte        memory[PAGE_SIZE];
} Page;

typedef struct
{
    unsigned    number;
    int         no_pages;
    Page        **page;
    ulong       min_address_used;
    ulong       max_address_used;
    int         used;
} Bank;

static int      num_banks = 0;
static ulong    pc = 0;
static ulong    address_space = 0;
static unsigned currbank = 0;
static WordMode wmode = LSB_Word;

static Bank             **banks;
static unsigned         *defined_banks;

/* ---------------------------------------- PRIVATE
*/
static int SortBankNumbers(const void *pa, const void *pb)
{
    const unsigned *a = pa;
    const unsigned *b = pb;

    if (*a > *b)
    {
        return 1;
    }
    else if (*a < *b)
    {
        return -1;
    }

    return 0;
}

static Bank *FindBank(unsigned n)
{
    int f;

    for(f = 0; f < num_banks; f++)
    {
        if (banks[f]->number == n)
        {
            return banks[f];
        }
    }

    return NULL;
}

static Bank *AddBank(unsigned n)
{
    num_banks++;

    banks = Realloc(banks, (sizeof *banks) * num_banks);
    banks[num_banks-1] = Malloc(sizeof **banks);
    banks[num_banks-1]->number = n;
    banks[num_banks-1]->no_pages = 0;
    banks[num_banks-1]->page = NULL;
    banks[num_banks-1]->min_address_used = address_space;
    banks[num_banks-1]->max_address_used = 0;
    banks[num_banks-1]->used = FALSE;

    defined_banks = Realloc(defined_banks, (sizeof *defined_banks) * num_banks);
    defined_banks[num_banks - 1] = n;
    qsort(defined_banks, num_banks, sizeof *defined_banks, SortBankNumbers);

    return FindBank(n);
}

static Bank *GetOrAddBank(unsigned n)
{
    Bank *b = FindBank(n);

    if (!b)
    {
        b = AddBank(n);
    }

    return b;
}

static Page *FindPage(Bank *bank, ulong address)
{
     int f;

     for(f = 0; f < bank->no_pages; f++)
     {
        if (address >= bank->page[f]->base_address &&
                address < bank->page[f]->base_address + PAGE_SIZE)
        {
            return bank->page[f];
        }
     }

     return NULL;
}

static Page *GetOrAddPage(Bank *bank, ulong address)
{
    int f;
    Page *p = FindPage(bank, address);

    if (p)
    {
        return p;
    }

    p = Malloc(sizeof *p);
    p->base_address = (address / PAGE_SIZE) * PAGE_SIZE;

    for(f = 0; f < PAGE_SIZE; f++)
    {
        p->memory[f] = 0;
    }

    bank->no_pages++;
    bank->page = Realloc(bank->page, sizeof *bank->page * bank->no_pages);
    bank->page[bank->no_pages - 1] = p;

    return p;
}

/* ---------------------------------------- INTERFACES
*/

void ClearMemoryWriteMarkers(void)
{
    int f;

    for(f = 0; f < num_banks; f++)
    {
        banks[f]->min_address_used = address_space;
        banks[f]->max_address_used = 0;
    }
}



void SetAddressBank(unsigned b)
{
    currbank = b;
}


unsigned CurrentBank(void)
{
    return currbank;
}

ulong GetLowWriteMarker(unsigned bank)
{
    Bank *b = GetOrAddBank(bank);

    return b->min_address_used;
}

ulong GetHighWriteMarker(unsigned bank)
{
    Bank *b = GetOrAddBank(bank);

    return b->max_address_used;
}

int IsBankUsed(unsigned bank)
{
    Bank *b = FindBank(bank);

    if (b)
    {
        return b->used;
    }

    return FALSE;
}

void SetWordMode(WordMode mode)
{
    wmode = mode;
}


void SetAddressSpace(ulong size)
{
    address_space = size;
}


void SetPC(ulong i)
{
    pc = i;

    if (address_space > 0)
    {
        pc = pc % address_space;
    }
}


ulong PC(void)
{
    return pc;
}


void PCAdd(int i)
{
    pc += i;
    pc %= address_space;
}


void PCWrite(int i)
{
    MemoryWrite(pc, ExprConvert(8, i));
    pc = (pc + 1) % address_space;
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


const unsigned *DefinedBanks(int *count)
{
    *count = num_banks;
    return defined_banks;
}


Byte MemoryRead(ulong addr)
{
    return MemoryReadBank(currbank, addr);
}

Byte MemoryReadBank(unsigned bank, ulong addr)
{
    Bank* b = GetOrAddBank(currbank);
    Page *p = FindPage(b, addr);

    if (p)
    {
        return p->memory[addr - p->base_address];
    }

    return 0;
}

void MemoryWrite(ulong addr, Byte value)
{
    MemoryWriteBank(currbank, addr, value);
}

void MemoryWriteBank(unsigned bank, ulong addr, Byte value)
{
    Bank *b = GetOrAddBank(currbank);
    Page *p = GetOrAddPage(b, addr);

    p->memory[addr - p->base_address] = value;

    b->used = TRUE;

    if (addr < b->min_address_used)
    {
        b->min_address_used = addr;
    }

    if (addr > b->max_address_used)
    {
        b->max_address_used = addr;
    }
}

Byte *MemoryGetBlock(unsigned bank, ulong addr, ulong length)
{
    Byte *mem = Malloc(length);

    for(ulong i = 0 ; i < length; i++)
    {
        mem[i] = MemoryReadBank(bank, addr + i);
    }

    return mem;
}

/*
vim: ai sw=4 ts=8 expandtab
*/
