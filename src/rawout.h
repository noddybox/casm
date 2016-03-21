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

    RAW binary file output

*/

#ifndef CASM_RAWOUT_H
#define CASM_RAWOUT_H

#include "parse.h"
#include "state.h"
#include "cmd.h"

/* ---------------------------------------- INTERFACES
*/


/* RAW Output options
*/
const ValueTable *RawOutputOptions(void);

CommandStatus RawOutputSetOption(int opt, int argc, char *argv[],
                                 int quoted[], char *error, size_t error_size);


/* RAW output of assembly.  Returns TRUE if OK, FALSE for failure.
*/
int RawOutput(const char *filename, const char *filename_bank,
              MemoryBank **bank, int count,
              char *error, size_t error_size);

#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
