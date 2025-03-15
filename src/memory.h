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

    Memory is arranged in banks of RAM of 64K each.  This has now updated to
    support larger memory banks for 16 bit processors.

*/

#ifndef CASM_MEMORY_H
#define CASM_MEMORY_H

#include "global.h"

/* ---------------------------------------- TYPES
*/
typedef enum
{
    MSB_Word,
    LSB_Word
} WordMode;

/* ---------------------------------------- INTERFACES
*/

/* Clear memory markers so that all memory is seen as unused.
*/
void    ClearMemoryWriteMarkers(void);


/* Sets the current bank to use.  If it's never been used before it will be
   initialised as an empty bank.
*/
void    SetAddressBank(unsigned bank);


/* Get the current bank
*/
unsigned CurrentBank(void);


/* Get the low byte marker for the passed bank.  It will return the address
   space value if nothing has been written.
*/
ulong   GetLowWriteMarker(unsigned bank);


/* Get the high byte marker for the passed bank.  It will return zero if
   nothing has been written, but obviously that could be valid.  The low write
   marker should be used to determine if anything has been written.
*/
ulong   GetHighWriteMarker(unsigned bank);


/* Set the mode to write words in
*/
void    SetWordMode(WordMode mode);


/* Set the size of address space
*/
void    SetAddressSpace(ulong size);


/* Set the current PC
*/
void    SetPC(ulong value);


/* Get the current PC
*/
ulong   PC(void);


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


/* Get a list of the current backs.  count is updated with the count.
*/
const unsigned *DefinedBanks(int *count);


/* Read a byte from the current bank.
*/
Byte    MemoryRead(ulong addr);


/* Read a byte from the passed bank.
*/
Byte    MemoryReadBank(unsigned bank, ulong addr);


/* Write a byte to the current bank.
*/
void    MemoryWrite(ulong addr, Byte value);


/* Write a byte to the passed bank.
*/
void    MemoryWriteBank(unsigned bank, ulong addr, Byte value);


#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
