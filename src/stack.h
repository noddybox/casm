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

    Simple stack

*/

#ifndef CASM_STACK_H
#define CASM_STACK_H

typedef struct stack Stack;

/* ---------------------------------------- INTERFACES
*/

/* Create a new stack.
*/
Stack   *StackCreate(void);


/* Push a value to the stack.
*/
void    StackPush(Stack *stack, void *item);


/* Pop a value from the stack.  Returns NULL if the stack is empty.
*/
void    *StackPop(Stack *stack);


/* Like a pop, but doesn't remove the entry.  Returns NULL if the stack is
   empty.
*/
void    *StackPeek(Stack *stack);


/* Remove all entries from the stack
*/
#define StackClear(stack)       do {} while(StackPop(stack))


/* Number of items on stack
*/
int     StackSize(Stack *stack);


/* Free the stack.
*/
void    StackFree(Stack *stack);

#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
