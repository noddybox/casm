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

    Collection for aliases.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "global.h"
#include "alias.h"


/* ---------------------------------------- TYPES
*/

typedef struct alias
{
    char                *command;
    char                *alias;
    struct alias        *next;
} Alias;


/* ---------------------------------------- GLOBALS
*/
static Alias    *head;
static Alias    *tail;


/* ---------------------------------------- PRIVATE FUNCTIONS INTERFACES
*/
static Alias *FindAlias(const char *p)
{
    Alias *a = head;

    while(a)
    {
        if (CompareString(a->command, p))
        {
            return a;
        }

        a = a->next;
    }

    return NULL;
}


static void AddAlias(const char *p, const char *r)
{
    Alias *a = FindAlias(p);

    if (!a)
    {
        a = Malloc(sizeof *a);

        a->command = DupStr(p);
        a->alias = DupStr(r);
        a->next = NULL;

        if (tail)
        {
            tail->next = a;
        }

        tail = a;

        if (!head)
        {
            head = a;
        }
    }
    else
    {
        free(a->alias);
        a->alias = DupStr(r);
    }
}


/* ---------------------------------------- INTERFACES
*/

void AliasClear()
{
    while(head)
    {
        Alias *a;

        free(head->command);
        free(head->alias);
        a = head;
        head = head->next;
        free(a);
    }

    head = NULL;
    tail = NULL;
}


void AliasCreate(const char *command, const char *alias)
{
    AddAlias(command, alias);
}


char *AliasExpand(char *command)
{
    Alias *a;

    if ((a = FindAlias(command)))
    {
        free(command);
        command = DupStr(a->alias);
    }

    return command;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
