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

    Collection for aliases.

*/

#ifndef CASM_ALIAS_H
#define CASM_ALIAS_H

#include <stdio.h>

/* ---------------------------------------- INTERFACES
*/

/* Clear all aliases
*/
void    AliasClear();

/* Create an alias
*/
void    AliasCreate(const char *command, const char *alias);

/* Expand an alias.  The passed pointer is returned if there is no alias for
   the command, otherwise if there is an alias the passed pointer is freed
   and a newly allocated version of the replacement is returned.
*/
char    *AliasExpand(char *command);

#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
