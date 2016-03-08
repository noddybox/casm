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


/* ---------------------------------------- GLOBALS
*/
static int      pass = 1;
static int      maxpass = 2;
static int      pc = 0;
static int      size = 0x10000;
static Byte     *mem = NULL;
static int      minw = 0;
static int      maxw = 0;
static WordMode wmode = LSB_Word;


/* ---------------------------------------- INTERFACES
*/

void ClearState(void)
{
    pass = 1;
    pc = 0;
    minw = size;
    maxw = -1;
    wmode = LSB_Word;
    SetAddressSpace(0x10000);
}


void NextPass(void)
{
    if (pass < maxpass)
    {
        minw = size;
        maxw = -1;
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


void SetAddressSpace(int s)
{
    size = s;

    mem = Malloc(s);
    memset(mem, 0, size);
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
        pc += size;
    }

    pc %= size;
}


void PCWrite(int i)
{
    if (pc < minw)
    {
        minw = pc;
    }

    if (pc > maxw)
    {
        maxw = pc;
    }

    mem[pc] = ExprConvert(8, i);

    pc = (pc + 1) % size;
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


int GetMinAddressWritten(void)
{
    return minw;
}


int GetMaxAddressWritten(void)
{
    return maxw;
}


const Byte *AddressSpace(void)
{
    return mem;
}


Byte ReadByte(int addr)
{
    Byte b = 0;

    if (addr > -1 && addr < size)
    {
        b = mem[addr];
    }

    return b;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
