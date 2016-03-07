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

    Z80 Assembler

*/

#ifndef CASM_Z80_H
#define CASM_Z80_H

#include "parse.h"

void Init_Z80(void);

const ValueTable *Options_Z80(void);

CommandStatus SetOption_Z80(int opt, int argc, char *argv[], int quoted[],
                            char *err, size_t errsize);

CommandStatus Handler_Z80(const char *label, int argc, char *argv[],
                          int quoted[], char *error, size_t error_size);


#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
