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

    Various output type handlers

*/

#ifndef CASM_OUTPUT_H
#define CASM_OUTPUT_H

#include "parse.h"
#include "cmd.h"

/* Pull in the output drivers
*/
#include "rawout.h"
#include "specout.h"
#include "t64out.h"
#include "zx81out.h"
#include "gbout.h"
#include "snesout.h"
#include "libout.h"

/* ---------------------------------------- INTERFACES
*/


/* Output options
*/
const ValueTable *OutputOptions(void);

CommandStatus OutputSetOption(int opt, int argc, char *argv[],
                              int quoted[], char *error, size_t error_size);


/* Outputs the result of assembly.  Returns TRUE if OK, FALSE for failure.
*/
int     OutputCode(void);


/* Returns a reason for the last failure.
*/
const char *OutputError(void);


#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
