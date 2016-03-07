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

#include "global.h"
#include "stack.h"


/* ---------------------------------------- TYPES
*/

typedef struct stackitem
{
    void                *data;
    struct stackitem    *next;
} StackItem;


struct stack
{
    StackItem   *items;
    int         size;
};


/* ---------------------------------------- INTERFACES
*/

Stack *StackCreate(void)
{
    Stack *s;

    s = Malloc(sizeof *s);

    s->items = NULL;
    s->size = 0;

    return s;
}


void StackPush(Stack *stack, void *item)
{
    if (stack)
    {
        StackItem *i;

        i = Malloc(sizeof *i);

        i->next = stack->items;
        i->data = item;
        stack->items = i;
        stack->size++;
    }
}


void *StackPop(Stack *stack)
{
    void *item = NULL;

    if (stack && stack->items)
    {
        StackItem *next = stack->items->next;

        item = stack->items->data;

        free(stack->items);
        stack->items = next;
        stack->size--;
    }

    return item;
}


void *StackPeek(Stack *stack)
{
    void *item = NULL;

    if (stack && stack->items)
    {
        item = stack->items->data;
    }

    return item;
}


int StackSize(Stack *stack)
{
    return stack ? stack->size : 0;
}


void StackFree(Stack *stack)
{
    if (stack)
    {
        StackItem *i = stack->items;

        while(i)
        {
            StackItem *t = i;

            i = i->next;

            free(t);
        }

        free(stack);
    }
}



/*
vim: ai sw=4 ts=8 expandtab
*/
