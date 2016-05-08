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

    Listing handler

*/

#ifndef CASM_LISTING_H
#define CASM_LISTING_H

#include "cmd.h"
#include "parse.h"
#include "label.h"


/* List options
*/
const ValueTable *ListOptions(void);

CommandStatus ListSetOption(int opt, int argc, char *argv[],
                            int quoted[], char *error, size_t error_size);


/* Call before start of line processing
*/
void    ListStartLine(void);

/* Output a read line to the listing.
*/
void    ListLine(const char *line);


/* Output a macro invocation to the listing
*/
void    ListMacroInvokeStart(int argc, char *argv[], int quoted[]);
void    ListMacroInvokeEnd(const char *name);


/* Output arbitary string.  A terminating new line will NOT be added.
*/
void    ListPrintf(const char *fmt, ...);


/* Output error/warning message.  The error will also be reported to stderr.
   A terminating newline will be added.
*/
void    ListError(const char *fmt, ...);


/* Finish the listing
*/
void    ListFinish(void);


#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
