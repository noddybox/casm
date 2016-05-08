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

    Common utilities

*/

#ifndef CASM_UTIL_H
#define CASM_UTIL_H

#include <stdlib.h>


/* ---------------------------------------- INTERFACES
*/


/* Duplicate a string.  Just use free() to free.
*/
char    *DupStr(const char *p);


/* Malloc wrapper.  Just use free() to free.
*/
void    *Malloc(size_t len);


/* Realloc wrapper.  Just use free() to free.
*/
void    *Realloc(void *p, size_t len);


/* Remove end-of-line characters.  Returns the passed pointer.
*/
char    *RemoveNL(char *p);


/* Remove white space from the start and end of a string.
*/
char    *Trim(char *p);


/* Compare a string, but case insensitive.  Returns TRUE for match, otherwise
   FALSE.
*/
int     CompareString(const char *a, const char *b);


/* Compare a character, but case insensitive.  Returns TRUE for match, otherwise
   FALSE.
*/
int     CompareChar(char a, char b);


/* Compare the start of a string 'a' starts with string 'b', but case
   insensitive.  Returns TRUE for match, otherwise FALSE.
*/
int     CompareStart(const char *a, const char *b);


/* Compare the end of a string 'a' ends with string 'b', but case insensitive.
   Returns TRUE for match, otherwise FALSE.
*/
int     CompareEnd(const char *a, const char *b);


/* Strncpy, with a safety nulling of the last character.  Returns the 1st
   parameter.
*/
char    *CopyStr(char *dest, const char *src, size_t size);


/* Returns TRUE if a string is either a NULL pointer or just full of white space
*/
int     IsNullOrEmpty(const char *p);


/* Null function that can be used as a handy place to hang a breakpoint.
*/
void    DebugBreakPoint(void);


#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
