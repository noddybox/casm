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

    Collection for labels.

*/

#ifndef CASM_LABEL_H
#define CASM_LABEL_H

#include <stdio.h>

/* ---------------------------------------- INTERFACES
*/

/* Maximum size of a label.  All characters past this point will be ignored.
*/
#define MAX_LABEL_SIZE  32

/* Label types
*/
typedef enum
{
    GLOBAL_LABEL = 1,
    LOCAL_LABEL = 2,
    ANY_LABEL = 3
} LabelType;


/* A label
*/
typedef struct
{
    char        name[MAX_LABEL_SIZE+1];
    int         value;
    LabelType   type;
} Label;


/* Clear labels
*/
void            LabelClear(void);


/* Expand a label or constant value.  Returns TRUE if parsed OK.
*/
int             LabelExpand(const char *expr, long *result);


/* Utility to sanatise a label name, returning whether it's a global or local
   label.  Returns FALSE if the label is invalid, otherwise returns TRUE.
*/
int             LabelSanatise(char *label, LabelType *type);


/* Get a label.  Returns NULL for an undefined label.
*/
const Label     *LabelFind(const char *label, LabelType type);


/* Set a label.  If the label already exists, overwrites the value.
   If the label being set is a GLOBAL_LABEL also sets the scope for local
   variables and the stacked global scope is cleared.
*/
void            LabelSet(const char *label, long value, LabelType type);


/* Set a global label, pushing the current global namespace onto a stack.
*/
void            LabelScopePush(const char *label, long value);


/* Pops the last namespace that was saved with LabelScopePush().
*/
void            LabelScopePop(void);


/* Sets the current global scope for local variables to the named global
   label.
*/
void            LabelSetScope(const char *label);


/* Create a label name to be used as a namespace.  The return is static and
   must be saved to be used.  Namespaces are simple identifer created in a set
   order, so LabelResetNamespace() can be used to replay them.
*/
const char      *LabelCreateNamespace(void);


/* Reset namespace generation back to its initial point.
*/
void            LabelResetNamespace(void);


/* Dump a formatted report of labels to the passed file pointer
*/
void            LabelDump(FILE *fp, int dump_private);


/* Dump a binary blob of information containing all the labels
*/
void            LabelWriteBlob(FILE *fp);


/* Read a binary blob of information containing all the labels, adjusting values
   by the passed offset.
*/
void            LabelReadBlob(FILE *fp, int offset);

/* Set 24-bit address mode
*/
void            LabelSetAddress24(int onoff);


#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
