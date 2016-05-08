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

    Expands an expression.

*/

#ifndef CASM_EXPR_H
#define CASM_EXPR_H

/* ---------------------------------------- INTERFACES
*/

/* Converts a number to a sized value.  If the number is negative then the
   result is returned in twos complement form.
*/
int     ExprConvert(int no_bits, int value);


/* Returns the result of expr and stores the answer in result.
   Returns FALSE on error.
*/
int     ExprEval(const char *expr, int *result);


/* Gets a readable reason for an error from ExprEval() or ExprParse.
*/
const char *ExprError(void);


#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
