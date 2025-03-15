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

    Stores assembly state.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "state.h"
#include "memory.h"
#include "util.h"
#include "expr.h"


/* ---------------------------------------- TYPES AND GLOBALS
*/
static int      pass = 1;
static int      maxpass = 2;

/* ---------------------------------------- INTERFACES
*/

void ClearState(void)
{
    pass = 1;
}


void NextPass(void)
{
    if (pass < maxpass)
    {
        ClearMemoryWriteMarkers();
        pass++;
    }
}


int IsFinalPass(void)
{
    return pass == maxpass;
}


int IsFirstPass(void)
{
    return pass == 1;
}


int IsIntermediatePass(void)
{
    return pass > 1 && pass < maxpass;
}


void SetNeededPasses(int n)
{
    if (!IsFinalPass())
    {
        maxpass = n;
    }
}


int GetCurrentPass(void)
{
    return pass;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
