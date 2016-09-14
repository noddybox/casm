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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "codepage.h"
#include "stack.h"
#include "label.h"


/* ---------------------------------------- TYPES
*/

typedef struct global
{
    Label               label;
    Label               *locals;
    int                 no_locals;
    int                 local_size;
    struct global       *next;
    struct global       *next_scope;
} GlobalLabel;


/* ---------------------------------------- GLOBALS
*/
static GlobalLabel      *head;
static GlobalLabel      *tail;
static GlobalLabel      *scope;

static char             namespace[MAX_LABEL_SIZE + 1];

static Stack            *stack;


/* ---------------------------------------- PRIVATE FUNCTIONS
*/

static void WriteNumber(FILE *fp, int num)
{
    fprintf(fp, "%.11d", num);
}


static int ReadNumber(FILE *fp)
{
    char buff[12];
    int f;

    for(f= 0 ;f < 11; f++)
    {
        buff[f] = getc(fp);
    }

    buff[f] = 0;

    return atoi(buff);
}


static void WriteName(FILE *fp, const char *name)
{
    fputs(name, fp);
    putc(0, fp);
}


static char *ReadName(FILE *fp, char *name)
{
    int l = MAX_LABEL_SIZE;
    int c;
    char *p;

    p = name;

    while((c = getc(fp)) && (l--))
    {
        *p++ = c;
    }

    *p = 0;

    return name;
}


static Label *FindLocal(const char *p, GlobalLabel *in)
{
    int f;

    for(f = 0; f < in->no_locals; f++)
    {
        if (CompareString(in->locals[f].name, p))
        {
            return in->locals + f;
        }
    }

    return NULL;
}


static GlobalLabel *FindGlobal(const char *p)
{
    GlobalLabel *g = head;

    while(g)
    {
        if (CompareString(g->label.name, p))
        {
            return g;
        }

        g = g->next;
    }

    return NULL;
}


static Label *Find(const char *p, LabelType type)
{
    Label *l = NULL;

    if ((type & LOCAL_LABEL) && scope)
    {
        l = FindLocal(p, scope);
    }

    if (l == NULL && (type & GLOBAL_LABEL))
    {
        GlobalLabel *g = FindGlobal(p);

        if (g)
        {
            l = &g->label;
        }
    }

    return l;
}


static void AddGlobal(const char *p, int value)
{
    GlobalLabel *l = FindGlobal(p);

    if (!l)
    {
        l = Malloc(sizeof *l);

        CopyStr(l->label.name, p, sizeof l->label.name);

        l->label.value = value;
        l->label.type = GLOBAL_LABEL;

        l->no_locals = 0;
        l->locals = NULL;
        l->local_size = 0;
        l->next = NULL;
        l->next_scope = NULL;

        if (tail)
        {
            tail->next = l;
        }

        tail = l;

        if (!head)
        {
            head = l;
        }
    }
    else
    {
        l->label.value = value;
    }

    scope = l;
}

static void AddLocal(const char *p, int value)
{
    int i;
    Label *l;

    if (scope == NULL)
    {
        fprintf(stderr, "BUG: Tried to add local %p "
                                "with no current scope\n", p);
        exit(EXIT_FAILURE);
    }

    l = FindLocal(p, scope);

    if (!l)
    {
        if (scope->no_locals >= scope->local_size)
        {
            scope->local_size += 32;

            scope->locals =
                Realloc(scope->locals,
                              (sizeof *scope->locals) * scope->local_size);
        }

        i = scope->no_locals++;

        CopyStr(scope->locals[i].name, p, sizeof scope->locals[i].name);
        scope->locals[i].value = value;
        scope->locals[i].type = LOCAL_LABEL;
    }
    else
    {
        l->value = value;
    }
}


static int ParseBase(const char *str, int base, int *result, char last)
{
    char *p;
    int i;

    *result = (int)strtol(str, &p, base);

    return *p == last;
}


/* ---------------------------------------- INTERFACES
*/

void LabelClear(void)
{
    GlobalLabel *l;

    l = head;

    while(l)
    {
        GlobalLabel *tmp;

        tmp = l;

        l = l->next;

        if (tmp->no_locals)
        {
            free(tmp->locals);
        }

        free(tmp);
    }

    head = NULL;
    tail = NULL;
}


int LabelExpand(const char *expr, int *result)
{
    Label *label;
    int found = FALSE;

    /* Check for special single-characters
    */
    if (expr[0] && !expr[1])
    {
        switch(expr[0])
        {
            /* Current PC
            */
            case '$':
                *result = PC();
                return TRUE;

            default:
                break;
        }
    }

    /* Find the label, or evaluate the constant if not found
    */
    if ((label = Find(expr, ANY_LABEL)))
    {
        *result = label->value;
        found = TRUE;
    }

    if (!found)
    {
        size_t len;
        char first, last;

        len = strlen(expr);
        first = expr[0];
        last = expr[len - 1];

        if (first == '$')
        {
            found = ParseBase(expr + 1, 16, result, '\0');
        }
        else if (last == 'h' || last == 'H')
        {
            found = ParseBase(expr, 16, result, last);
        }
        else if (first == '%')
        {
            found = ParseBase(expr + 1, 2, result, '\0');
        }
        else if (last == 'b' || last == 'B')
        {
            found = ParseBase(expr, 2, result, last);
        }
        else if (first == '\'' && last == '\'' && len == 3)
        {
            *result = CodepageConvert(expr[1]);
            found = TRUE;
        }
        else
        {
            found = ParseBase(expr, 0, result, '\0');
        }
    }

    return found;
}


int LabelSanatise(char *label, LabelType *type)
{
    int status = TRUE;
    size_t len;

    *type = GLOBAL_LABEL;

    len = strlen(label);

    if (len && label[0] == '.')
    {
        *type = LOCAL_LABEL;
        memmove(label, label + 1, len--);
    }

    if (len && label[len - 1] == ':')
    {
        label[--len] = 0;
    }

    if (len == 0)
    {
        status = FALSE;
    }

    return status;
}


const Label *LabelFind(const char *label, LabelType type)
{
    return Find(label, type);
}


void LabelSet(const char *label, int value, LabelType type)
{
    /* ANY_LABEL indicates that a label is being updated
    */
    switch(type)
    {
        case ANY_LABEL:
            if (!scope || CompareString(scope->label.name, label))
            {
                scope->label.value = value;
            }
            else
            {
                AddLocal(label,value);
            }
            break;

        case GLOBAL_LABEL:
            AddGlobal(label, value);
            StackClear(stack);
            break;

        case LOCAL_LABEL:
            AddLocal(label,value);
            break;
    }
}


void LabelScopePush(const char *name, int value)
{
    if (!stack)
    {
        stack = StackCreate();
    }

    StackPush(stack, scope);
    AddGlobal(name, value);
}


void LabelScopePop(void)
{
    scope = StackPop(stack);

    if (!scope)
    {
        fprintf(stderr, "ERROR: Popping the global scope left it empty");
        exit(EXIT_FAILURE);
    }
}


void LabelSetScope(const char *label)
{
    scope = FindGlobal(label);
}


const char *LabelCreateNamespace(void)
{
    int f;

    if (namespace[0] == 0)
    {
        LabelResetNamespace();
    }

    f = 1;

    while(f < MAX_LABEL_SIZE && namespace[f] == '9')
    {
        f++;
    }

    if (f < MAX_LABEL_SIZE)
    {
        namespace[f]++;
    }

    return namespace;
}

void LabelResetNamespace(void)
{
    int f;

    namespace[0] = '_';

    for(f = 1; f < MAX_LABEL_SIZE; f++)
    {
        namespace[f] = '0';
    }

    namespace[MAX_LABEL_SIZE] = 0;
}


void LabelDump(FILE *fp, int dump_private)
{
    GlobalLabel *g = head;

    while(g)
    {
        int f;

        if (g->label.name[0] != '_' || dump_private)
        {
            fprintf(fp, "; %-*s  = $%8.8x (%d)\n", MAX_LABEL_SIZE,
                                            g->label.name,
                                            (unsigned)g->label.value,
                                            g->label.value);

            for(f = 0; f < g->no_locals; f++)
            {
                fprintf(fp, "; .%-*s = $%8.8x (%d)\n", MAX_LABEL_SIZE,
                                                g->locals[f].name,
                                                (unsigned)g->locals[f].value,
                                                g->locals[f].value);
            }
        }

        g = g->next;
    }
}


void LabelWriteBlob(FILE *fp)
{
    GlobalLabel *g = head;
    int count;
    int f;

    count = 0;

    while(g)
    {
        if (g->label.name[0] != '_')
        {
            count++;
        }

        g = g->next;
    }

    WriteNumber(fp, count);

    g = head;

    while(g)
    {
        if (g->label.name[0] != '_')
        {
            WriteName(fp, g->label.name);
            WriteNumber(fp, g->label.value);
        }

        g = g->next;
    }
}


void LabelReadBlob(FILE *fp, int offset)
{
    int count;
    int f;

    count = ReadNumber(fp);

    for(f = 0; f < count; f++)
    {
        char name[MAX_LABEL_SIZE + 1];
        int value;

        ReadName(fp, name);
        value = ReadNumber(fp);

        LabelSet(name, value + offset, GLOBAL_LABEL);
    }
}


/*
vim: ai sw=4 ts=8 expandtab
*/
