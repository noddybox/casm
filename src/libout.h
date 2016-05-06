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

    LIB binary file output

*/

#ifndef CASM_LIBOUT_H
#define CASM_LIBOUT_H

#include "parse.h"
#include "state.h"
#include "cmd.h"

typedef enum
{
    LibLoadAll,
    LibLoadLabels,
    LibLoadMemory
} LibLoadOption;

/* ---------------------------------------- INTERFACES
*/


/* LIB Output options
*/
const ValueTable *LibOutputOptions(void);

CommandStatus LibOutputSetOption(int opt, int argc, char *argv[],
                                 int quoted[], char *error, size_t error_size);


/* LIB output of assembly.  Returns TRUE if OK, FALSE for failure.
*/
int LibOutput(const char *filename, const char *filename_bank,
              MemoryBank **bank, int count,
              char *error, size_t error_size);


/* Load a libray.  Returns TRUE if OK, FALSE for failure and updates error.
*/
int LibLoad(const char *filename, LibLoadOption opt,
            char *error, size_t error_size);

#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
