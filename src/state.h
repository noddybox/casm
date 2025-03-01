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

    Stores assembly state (passes, memory, etc).

    Memory is arranged in banks of RAM of 64K each.  This has now updated to
    support larger memory banks for 16 bit processors.

*/

#ifndef CASM_STATE_H
#define CASM_STATE_H

#include "global.h"

/* ---------------------------------------- TYPES
*/
typedef enum
{
    MSB_Word,
    LSB_Word
} WordMode;

typedef struct
{
    /* The bank number, 0 .. n
    */
    unsigned    number;

    /* The memory in that bank.  By default a 64K bank will be allocated.
       This is to maintain compatibiility with it's original defination for
       8-bit machines.  Addressing starts at zero so this will be wildly
       inefficient on 16-bit machines if high addresses are used.
    */
    Byte        *memory;

    /* The minumum address used in the bank.  Will be address space if not used.
    */
    int         min_address_used;

    /* The maximum address used in the bank.  Will be -1 if not used.
    */
    int         max_address_used;

    /* This is a field used internally by the state
    */
    long        memory_alloc_size;
} MemoryBank;

/* ---------------------------------------- INTERFACES
*/

/* Clear state to default.  This creates 64K of RAM to write into and sets
   zero as the current bank.
*/
void    ClearState(void);


/* Sets the current bank to use.  If it's never been used before it will be
   initialised as an empty bank.
*/
void    SetAddressBank(unsigned bank);


/* Get the current bank
*/
unsigned Bank(void);


/* Move onto the next pass
*/
void    NextPass(void);


/* Is final pass?
*/
int     IsFinalPass(void);


/* Is first pass?
*/
int     IsFirstPass(void);


/* Is intermediate pass?
*/
int     IsIntermediatePass(void);


/* Set number of passes needed.  This works while IsFinalPass() returns FALSE.
*/
void    SetNeededPasses(int n);


/* Get current pass.  Just used for debug.
*/
int     GetCurrentPass(void);


/* Set the current PC
*/
void    SetPC(long i);


/* Set the mode to write words in
*/
void    SetWordMode(WordMode mode);


/* Set the size of address space
*/
void    SetAddressSpace(long size);


/* Get the current PC
*/
long    PC(void);


/* Add a number to the PC
*/
void    PCAdd(int i);


/* Write a byte to the PC and increment it
*/
void    PCWrite(int i);


/* Write a word to the PC and increment it
*/
void    PCWriteWord(int i);
void    PCWriteWordMode(int i, WordMode mode);


/* Gets a list of the banks used as an array of pointers.  A NULL return means
   no memory was written to at all.
   
   *count will be updated with the number of banks used, or zero if no memory
   was written to.  Note that banks may not be contiguously numbered, but will
   be in ascending order.
*/
MemoryBank **MemoryBanks(int *count);


/* Read a byte from the current bank.
*/
Byte    ReadByte(long addr);


#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
