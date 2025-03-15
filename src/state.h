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

    Stores assembly state (passes, etc).

*/

#ifndef CASM_STATE_H
#define CASM_STATE_H

/* ---------------------------------------- INTERFACES
*/

/* Clear state to default.  
*/
void    ClearState(void);


/* Move onto the next pass
*/
void    NextPass(void);


/* Is final pass?
*/
int     IsFinalPass(void);


/* Is first pass?
*/
int     IsFirstPass(void);


/* Is intermediate pass?
*/
int     IsIntermediatePass(void);


/* Set number of passes needed.  This works while IsFinalPass() returns FALSE.
*/
void    SetNeededPasses(int n);


/* Get current pass.  Just used for debug.
*/
int     GetCurrentPass(void);

#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
