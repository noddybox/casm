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

    SPC700 Assembler

*/

#ifndef CASM_SPC700_H
#define CASM_SPC700_H

void Init_SPC700(void);

const ValueTable *Options_SPC700(void);

CommandStatus SetOption_SPC700(int opt, int argc, char *argv[], int quoted[],
                               char *error, size_t error_size);

CommandStatus Handler_SPC700(const char *label, int argc, char *argv[],       
                             int quoted[], char *error, size_t error_size);

#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
