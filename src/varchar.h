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

    Simple dynamic variable length string.

*/

#ifndef CASM_VARCHAR_H
#define CASM_VARCHAR_H

typedef struct varchar Varchar;

/* ---------------------------------------- INTERFACES
*/

/* Create a new string.  Initial contents set to pointer if not NULL.
*/
Varchar *VarcharCreate(const char *initial);


/* Add a character to a string.  Returns the passed Varchar.
*/
Varchar *VarcharAddChar(Varchar *str, int c);


/* Add a string to the string.  Returns the passed Varchar.
*/
Varchar *VarcharAdd(Varchar *str, const char *c);


/* Adds a formatted string to the string.  The formatted string must be less
   than 1024 character or it will be truncated.  Returns the passed Varchar.
*/
Varchar *VarcharPrintf(Varchar *str, const char *fmt, ...);


/* Get the contents of a string, keeping ownership of it
*/
const char *VarcharContents(Varchar *str);


/* Transfer the contents of a string, so that the returned string can be
   freed.  The Varchar container is destroyed and should not be used again.
*/
char *VarcharTransfer(Varchar *str);


/* Clear a Varchar back to an empty string.
*/
void VarcharClear(Varchar *str);


/* Release the string and the container.
*/
void VarcharFree(Varchar *str);

#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
