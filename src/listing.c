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

    Listing

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "global.h"
#include "state.h"
#include "label.h"
#include "macro.h"
#include "expr.h"
#include "varchar.h"
#include "listing.h"


/* ---------------------------------------- PRIVATE TYPES
*/
enum option_t
{
    OPT_LIST,
    OPT_LISTFILE,
    OPT_LISTPC,
    OPT_LISTHEX,
    OPT_LISTMACROS,
    OPT_LISTLABELS,
    OPT_LISTRMBLANK
};

static const ValueTable option_set[] =
{
    {"list",            OPT_LIST},
    {"list-file",       OPT_LISTFILE},
    {"list-pc",         OPT_LISTPC},
    {"list-hex",        OPT_LISTHEX},
    {"list-macros",     OPT_LISTMACROS},
    {"list-labels",     OPT_LISTLABELS},
    {"list-rm-blank",   OPT_LISTRMBLANK},
    {NULL}
};


typedef enum
{
    LabelsOff = 0,
    LabelsDump = 1,
    LabelsDumpPrivate = 2
} LabelMode;


typedef enum
{
    MacrosOff = 0,
    MacrosInvoke = 1,
    MacrosDump = 2
} MacroMode;


typedef struct
{
    int         enabled;
    int         dump_PC;
    int         dump_bytes;
    int         rm_blank;
    LabelMode   labels;
    MacroMode   macros;
} Options;


/* ---------------------------------------- PRIVATE DATA
*/
static int              line_PC;
static int              last_line_blank;

static FILE             *output;

static Options          options = 
{
    FALSE,
    FALSE,
    FALSE,
    TRUE,
    LabelsOff,
    MacrosOff,
};


static ValueTable       labels_table[] =
{
    {"off",     LabelsOff},
    {"no",      LabelsOff},
    {"on",      LabelsDump},
    {"yes",     LabelsDump},
    {"all",     LabelsDump | LabelsDumpPrivate},
    {NULL}
};


static ValueTable       macros_table[] =
{
    {"off",     MacrosOff},
    {"no",      MacrosOff},
    {"yes",     MacrosInvoke},
    {"exec",    MacrosInvoke},
    {"dump",    MacrosDump},
    {"all",     MacrosInvoke | MacrosDump},
    {NULL}
};


/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static void BuildArgs(Varchar *str, int argc, char *argv[], int quoted[])
{
    int f;

    for(f = 0; f < argc; f++)
    {
        if (f > 0)
        {
            VarcharAdd(str, ", ");
        }

        if (quoted[f] && quoted[f] == '(')
        {
            VarcharPrintf(str, "(%s)", argv[f]);
        }
        else if (quoted[f])
        {
            VarcharPrintf(str, "%c%s%c", quoted[f], argv[f], quoted[f]);
        }
        else
        {
            VarcharAdd(str, argv[f]);
        }
    }
}


static void Output(const char *fmt, ...)
{
    if (IsFinalPass() && options.enabled)
    {
        va_list va;

        va_start(va, fmt);

        if (output)
        {
            vfprintf(output, fmt, va);
        }
        else
        {
            vfprintf(stdout, fmt, va);
        }

        va_end(va);
    }
}



static FILE *GetOutput(void)
{
    return output ? output : stdout;
}


static int IsBlankLine(const char *p)
{
    while(*p)
    {
        if (!isspace((unsigned char)*p++))
        {
            return FALSE;
        }
    }

    return TRUE;
}


/* ---------------------------------------- INTERFACES
*/

const ValueTable *ListOptions(void)
{
    return option_set;
}

CommandStatus ListSetOption(int opt, int argc, char *argv[],
                            int quoted[], char *err, size_t errsize)
{
    const ValueTable *val;
    CommandStatus stat = CMD_OK;

    if (!IsFinalPass())
    {
        return CMD_OK;
    }

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_LIST:
            options.enabled = ParseTrueFalse(argv[0], FALSE);
            break;

        case OPT_LISTFILE:
            if (output)
            {
                snprintf(err, errsize, "output file already set");
                stat = CMD_FAILED;
            }
            else
            {
                output = fopen(argv[0], "w");

                if (!output)
                {
                    snprintf(err, errsize, "couldn't open \"%s\"", argv[0]);
                    stat = CMD_FAILED;
                }
            }
            break;

        case OPT_LISTPC:
            options.dump_PC = ParseTrueFalse(argv[0], FALSE);
            break;

        case OPT_LISTHEX:
            options.dump_bytes = ParseTrueFalse(argv[0], FALSE);
            break;

        case OPT_LISTMACROS:
            CMD_TABLE(argv[0], macros_table, val);
            options.macros = val->value;
            break;

        case OPT_LISTLABELS:
            CMD_TABLE(argv[0], labels_table, val);
            options.labels = val->value;
            break;

        case OPT_LISTRMBLANK:
            options.rm_blank = ParseTrueFalse(argv[0], FALSE);
            break;

        default:
            break;
    }

    return stat;
}


void ListStartLine(void)
{
    line_PC = PC();
}


void ListLine(const char *line)
{
    if (IsFinalPass() && options.enabled)
    {
        if (options.rm_blank && last_line_blank && IsBlankLine(line))
        {
            return;
        }

        last_line_blank = IsBlankLine(line);

        Output("%s\n", line);

        /* Generate PC and hex dump and add to comment
        */
        if ((options.dump_PC || options.dump_bytes) && (PC() != line_PC))
        {
            Varchar *hex;
            int f;

            hex = VarcharCreate(NULL);

            VarcharAdd(hex, ";");

            if (options.dump_PC)
            {
                VarcharPrintf(hex, " $%4.4X:", (unsigned)line_PC);
            }

            if (options.dump_bytes && (PC() - line_PC < 256))
            {
                for(f = line_PC; f < PC(); f++)
                {
                    VarcharPrintf(hex, " $%2.2X", (unsigned)ReadByte(f));
                }
            }

            Output("%s\n", VarcharContents(hex));

            VarcharFree(hex);
        }
    }
}


void ListMacroInvokeStart(int argc, char *argv[], int quoted[])
{
    if (IsFinalPass() && options.enabled && options.macros & MacrosInvoke)
    {
        Varchar *s;

        s = VarcharCreate(NULL);

        VarcharPrintf(s, "; START MACRO %s ", argv[0]);

        BuildArgs(s, argc - 1, argv + 1, quoted + 1);

        Output("%s\n", VarcharContents(s));
        VarcharFree(s);
    }
}


void ListMacroInvokeEnd(const char *name)
{
    if (IsFinalPass() && options.enabled && options.macros & MacrosInvoke)
    {
        Varchar *s;

        s = VarcharCreate(NULL);

        VarcharAdd(s, "; END MACRO ");
        VarcharAdd(s, name);

        Output("%s\n", VarcharContents(s));
        VarcharFree(s);
    }
}


void ListError(const char *fmt, ...)
{
    char buff[4096];
    va_list va;

    va_start(va, fmt);
    vsnprintf(buff, sizeof buff, fmt, va);
    va_end(va);

    fprintf(stderr, "%s\n", buff);

    if (IsFinalPass() && options.enabled && output)
    {
        Output("%s\n", buff);
    }
}


void ListFinish(void)
{
    if (IsFinalPass() && options.enabled)
    {
        if (options.labels)
        {
            Output("\n;\n; LABELS:\n;\n");
            LabelDump(GetOutput(), options.labels & LabelsDumpPrivate);
        }

        if (options.macros & MacrosDump)
        {
            Output("\n;\n; MACROS:\n;\n");
            MacroDump(GetOutput());
        }
    }

    if (output)
    {
        if (output != stdout)
        {
            fclose(output);
        }

        output = NULL;
    }

    options.enabled = FALSE;
}


/*
vim: ai sw=4 ts=8 expandtab
*/
