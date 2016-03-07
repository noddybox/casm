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

    Collection for macros.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "global.h"
#include "codepage.h"
#include "varchar.h"


/* ---------------------------------------- TYPES
*/

struct varchar
{
    size_t      len;
    size_t      size;
    char        *data;
};


/* ---------------------------------------- INTERFACES
*/

Varchar *VarcharCreate(const char *initial)
{
    Varchar *str;

    str = Malloc(sizeof *str);
    str->len = 0;
    str->size = 0;
    str->data = NULL;

    if (initial)
    {
        VarcharAdd(str, initial);
    }

    return str;
}


Varchar *VarcharAddChar(Varchar *str, int c)
{
    if ((str->size - str->len) < 2)
    {
        str->size += 1024;
        str->data = Realloc(str->data, str->size);
    }

    str->data[str->len++] = c;
    str->data[str->len] = 0;

    return str;
}


Varchar *VarcharAdd(Varchar *str, const char *c)
{
    size_t l;

    l = strlen(c);

    if ((str->size - str->len) <= (l + 1))
    {
        str->size += (l / 1024 + 1) * 1024;
        str->data = Realloc(str->data, str->size);
    }

    strcpy(str->data + str->len, c);
    str->len += l;
    str->data[str->len] = 0;

    return str;
}


Varchar *VarcharPrintf(Varchar *str, const char *fmt, ...)
{
    char buff[1025];
    va_list va;
    size_t l;

    va_start(va, fmt);
    vsnprintf(buff, sizeof buff, fmt, va);
    va_end(va);

    buff[1024] = 0;

    return VarcharAdd(str, buff);
}


const char *VarcharContents(Varchar *str)
{
    return str->data;
}


char *VarcharTransfer(Varchar *str)
{
    char *p = str->data;

    free(str);

    return p;
}

void VarcharClear(Varchar *str)
{
    if (str->data)
    {
        free(str->data);
    }

    str->len = 0;
    str->size = 0;
    str->data = NULL;
}

void VarcharFree(Varchar *str)
{
    if (str->data)
    {
        free(str->data);
    }

    free(str);
}


/*
vim: ai sw=4 ts=8 expandtab
*/
