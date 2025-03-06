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

    68000 Assembler

*/

#ifndef CASM_68000_H
#define CASM_68000_H

void Init_68000(void);

const ValueTable *Options_68000(void);

CommandStatus SetOption_68000(int opt, int argc, char *argv[], int quoted[],
                             char *error, size_t error_size);

CommandStatus Handler_68000(const char *label, int argc, char *argv[],       
                           int quoted[], char *error, size_t error_size);       

#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
