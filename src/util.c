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

    Common utilities

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "global.h"
#include "util.h"


/* ---------------------------------------- INTERFACES
*/

char *DupStr(const char *p)
{
    char *new;

    new = Malloc(strlen(p) + 1);
    strcpy(new, p);
    return new;
}


void *Malloc(size_t len)
{
    void *new;

    if (!(new = malloc(len)))
    {
        fprintf(stderr, "Unable to allocate %lu bytes\n", (unsigned long)len);
        exit(EXIT_FAILURE);
    }

    return new;
}


void *Realloc(void *p, size_t len)
{
    void *new;

    if (!(new = realloc(p, len)))
    {
        fprintf(stderr, "Unable to reallocate %lu bytes\n", (unsigned long)len);
        exit(EXIT_FAILURE);
    }

    return new;
}


char *RemoveNL(char *p)
{
    if (p)
    {
        size_t l = strlen(p);

        while (l > 0 && (p[l-1] == '\n'))
        {
            p[--l] = 0;
        }
    }

    return p;
}


char *Trim(char *p)
{
    if (p)
    {
        size_t l = strlen(p);

        while (l > 0 && isspace((unsigned char)p[0]))
        {
            memmove(p, p + 1, l--);
        }

        while(l > 1 && isspace((unsigned char)p[l-1]))
        {
            p[--l] = 0;
        }
    }

    return p;
}


char *CopyStr(char *dest, const char *src, size_t size)
{
    strncpy(dest, src, size);
    dest[size - 1] = 0;
    return dest;
}


int CompareString(const char *a, const char *b)
{
    while(*a && *b)
    {
        char c,d;

        c = tolower((unsigned char)*a++);
        d = tolower((unsigned char)*b++);

        if (c != d)
        {
            return FALSE;
        }
    }

    return *a == *b;
}


int CompareStart(const char *a, const char *b)
{
    while(*a && *b)
    {
        char c,d;

        c = tolower((unsigned char)*a++);
        d = tolower((unsigned char)*b++);

        if (c != d)
        {
            return FALSE;
        }
    }

    return (*b == 0);
}


int CompareEnd(const char *a, const char *b)
{
    if (strlen(a) < strlen(b))
    {
        return FALSE;
    }

    a += strlen(a) - strlen(b);

    while(*a && *b)
    {
        char c,d;

        c = tolower((unsigned char)*a++);
        d = tolower((unsigned char)*b++);

        if (c != d)
        {
            return FALSE;
        }
    }

    return (*b == 0);
}


int IsNullOrEmpty(const char *p)
{
    int empty = TRUE;

    while(p && *p && empty)
    {
        if (!isspace((unsigned char)*p++))
        {
            empty = FALSE;
        }
    }

    return empty;
}


void DebugBreakPoint(void)
{
    return;
}

/*
vim: ai sw=4 ts=8 expandtab
*/
