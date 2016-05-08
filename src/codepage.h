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

    Code page handling

*/

#ifndef CASM_CODEPAGE_H
#define CASM_CODEPAGE_H

#include "parse.h"
#include "cmd.h"

/* ---------------------------------------- INTERFACES
*/
typedef enum
{
    CP_ASCII,
    CP_ZX81,
    CP_SPECTRUM,
    CP_CBM
} Codepage;


/* Codepage options
*/
const ValueTable *CodepageOptions(void);

CommandStatus CodepageSetOption(int opt, int argc, char *argv[],
                                int quoted[], char *error, size_t error_size);




/* Converts the passed character into the appropriate codepage value.
   Returns zero for unknown/unconvertable characters.
*/
int     CodepageConvert(int code);


/* Converts from the execution character set into a code from the specified
   codepage.
*/
int     CodeFromNative(Codepage page, int code);


#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
