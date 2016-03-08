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

#ifndef CASM_STATE_H
#define CASM_STATE_H

/* ---------------------------------------- TYPES
*/
typedef enum
{
    MSB_Word,
    LSB_Word
} WordMode;

/* ---------------------------------------- INTERFACES
*/

/* Clear state to default.  This creates 64K of RAM to write into.
*/
void    ClearState(void);


/* Sets the number of bytes in RAM.  Addressing always starts from zero.
*/
void    SetAddressSpace(int size);


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


/* Set the current PC
*/
void    SetPC(int i);


/* Set the mode to write words in
*/
void    SetWordMode(WordMode mode);


/* Get the current PC
*/
int     PC(void);


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


/* Get the minimum address written to
*/
int     GetMinAddressWritten(void);


/* Get the maximum address written to
*/
int     GetMaxAddressWritten(void);


/* Access the address space directly
*/
const Byte *AddressSpace(void);


/* Read a byte from the address space
*/
Byte    ReadByte(int addr);


#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
