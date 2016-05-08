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

    Collection for macros.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "global.h"
#include "codepage.h"
#include "varchar.h"
#include "macro.h"


/* ---------------------------------------- TYPES
*/

enum option_t
{
    OPT_MACROARGCHAR
};

static const ValueTable option_set[] =
{
    {"macro-arg-char",  OPT_MACROARGCHAR},
    {NULL}
};


struct mdef
{
    char        *name;
    int         no_args;
    char        **args;
    int         no_lines;
    char        **lines;
    struct mdef *next;
};


struct macro
{
    MacroDef    *def;
    int         line;
    int         argc;
    char        **argv;
    int         *quoted;
};


/* ---------------------------------------- GLOBALS
*/
static MacroDef         *head;
static MacroDef         *tail;

static const char       *arg_chars = "ABCDEFGHIJKLMNOPQRSTUVXYZ"
                                     "abcdefghijklmnopqrstuvxyz"
                                     "0123456789_";

static struct
{
    int arg_char;
} options = 
{
    '@'
};


/* ---------------------------------------- PRIVATE FUNCTIONS INTERFACES
*/
static MacroDef *FindMacro(const char *p)
{
    MacroDef *m = head;

    while(m)
    {
        if (CompareString(m->name, p))
        {
            return m;
        }

        m = m->next;
    }

    return NULL;
}


static int CheckArgName(const char *p)
{
    while(*p)
    {
        if (!strchr(arg_chars, *p++))
        {
            return FALSE;
        }
    }

    return TRUE;
}


static MacroDef *AddMacro(const char *p, int argc, char *argv[],
                          char *err, size_t errsize)
{
    MacroDef *m = FindMacro(p);

    if (!m)
    {
        m = Malloc(sizeof *m);

        m->name = DupStr(p);
        m->no_lines = 0;
        m->lines = NULL;
        m->next = NULL;

        m->no_args = argc;

        if (m->no_args == 0)
        {
            m->args = NULL;
        }
        else
        {
            int f;

            m->args = Malloc((sizeof *m->args) * m->no_args);

            for(f = 0; f < argc; f++)
            {
                if (!argv[f][0] || !CheckArgName(argv[f]))
                {
                    snprintf(err, errsize, "illegal argument name '%s'",
                                                argv[f]);
                    return NULL;
                }

                m->args[f] = DupStr(argv[f]);
            }
        }

        if (tail)
        {
            tail->next = m;
        }

        tail = m;

        if (!head)
        {
            head = m;
        }
    }
    else
    {
        snprintf(err, errsize, "macro %s already exists", p);
        m = NULL;
    }

    return m;
}


static void AddArg(Varchar *str, Macro *macro, int arg_no)
{
    if (arg_no < macro->argc)
    {
        int quote = macro->quoted[arg_no];

        if (quote)
        {
            VarcharAddChar(str, quote);
        }

        VarcharAdd(str, macro->argv[arg_no]);

        if (quote)
        {
            if (quote == '(')
            {
                VarcharAddChar(str, ')');
            }
            else
            {
                VarcharAddChar(str, quote);
            }
        }
    }
}


static int FindArg(MacroDef *def, const char *name)
{
    int f;

    for(f = 0; f < def->no_args; f++)
    {
        if (CompareString(def->args[f], name))
        {
            return f;
        }
    }

    return def->no_args;
}

/* ---------------------------------------- INTERFACES
*/

void MacroSetDefaults(void)
{
    options.arg_char = '@';
}

const ValueTable *MacroOptions(void)
{
    return option_set;
}

CommandStatus MacroSetOption(int opt, int argc, char *argv[],
                             int quoted[], char *err, size_t errsize)
{
    CommandStatus stat = CMD_OK;

    CMD_ARGC_CHECK(1);

    switch(opt)
    {
        case OPT_MACROARGCHAR:
            options.arg_char = argv[0][0];

            if (options.arg_char == 0)
            {
                snprintf(err, errsize, "illegal character '\\0'");
                stat = CMD_FAILED;
            } else if (isalnum((unsigned char)options.arg_char))
            {
                snprintf(err, errsize, "illegal character '%c'",
                                                        options.arg_char);
                stat = CMD_FAILED;
            }
            break;

        default:
            break;
    }

    return stat;
}


MacroDef *MacroCreate(const char *name, int argc, char *argv[],
                      char *err, size_t errsize)
{
    MacroDef *macro = NULL;

    macro = AddMacro(name, argc, argv, err, errsize);

    return macro;
}

void MacroRecord(MacroDef *macro, const char *line)
{
    if (macro)
    {
        macro->no_lines++;

        macro->lines = Realloc(macro->lines, macro->no_lines *
                                                sizeof *macro->lines);
        macro->lines[macro->no_lines - 1] = DupStr(line);
    }
}

CommandStatus MacroFind(Macro **ret, int argc, char *argv[], int quoted[],
                        char *err, size_t errsize)
{
    MacroDef *def = NULL;
    Macro *macro = NULL;
    CommandStatus status = CMD_NOT_KNOWN;

    if (argc > 0)
    {
        def = FindMacro(argv[0]);
    }

    if (def)
    {
        if (def->no_args && def->no_args != argc - 1)
        {
            snprintf(err, errsize, "%s: expected %d argument%s, got %d",
                                argv[0], def->no_args,
                                def->no_args == 1 ? "" : "s",  argc - 1);

            status = CMD_FAILED;
        }
        else
        {
            int f;

            macro = Malloc(sizeof *macro);
            macro->line = 0;
            macro->def = def;
            macro->argc = argc;
            macro->argv = Malloc(argc * sizeof *macro->argv);
            macro->quoted = Malloc(argc * sizeof *macro->quoted);

            for(f = 0; f < argc; f++)
            {
                macro->argv[f] = DupStr(argv[f]);
                macro->quoted[f] = quoted[f];
            }

            status = CMD_OK;
        }
    }

    *ret = macro;

    return status;
}


char *MacroPlay(Macro *macro)
{
    if (macro && macro->line < macro->def->no_lines)
    {
        int size = 1000;
        int rd;
        char num[64];
        char arg[128];
        const char *line;
        int in_num = -1;
        int in_arg = -1;
        Varchar *str;

        line = macro->def->lines[macro->line++];

        /* If no arguments, simply duplicate
        */
        if (!strchr(line, '\\') && !strchr(line, options.arg_char))
        {
            return DupStr(line);
        }

        /* Expand the arguments
        */
        str = VarcharCreate(NULL);
        rd = 0;

        while(line[rd])
        {
            if (line[rd] == '\\')
            {
                in_num = 0;
                rd++;
            }
            else if (line[rd] == options.arg_char)
            {
                in_arg = 0;
                rd++;
            }
            else
            {
                if (in_num != -1)
                {
                    if (isdigit((unsigned char)line[rd]) && in_num < 60)
                    {
                        num[in_num++] = line[rd++];
                    }
                    else
                    {
                        if (in_num == 0 && line[rd] == '*')
                        {
                            int f;

                            rd++;

                            for(f = 0; f < macro->argc; f++)
                            {
                                if (f > 0)
                                {
                                    VarcharAddChar(str, ',');
                                }

                                AddArg(str, macro, f);
                            }
                            in_num = -1;
                        }
                        else
                        {
                            num[in_num] = 0;
                            AddArg(str, macro, atoi(num));
                            in_num = -1;
                        }
                    }
                }
                else if (in_arg != -1)
                {
                    if (strchr(arg_chars, line[rd]) && in_arg < 125)
                    {
                        arg[in_arg++] = line[rd++];
                    }
                    else
                    {
                        arg[in_arg] = 0;
                        AddArg(str, macro, FindArg(macro->def, arg) + 1);
                        in_arg = -1;
                    }
                }
                else
                {
                    VarcharAddChar(str, line[rd++]);
                }
            }
        }

        /* Check for arguments at the end of the line
        */
        if (in_num != -1)
        {
            num[in_num] = 0;
            AddArg(str, macro, atoi(num));
            in_num = -1;
        }

        if (in_arg != -1)
        {
            arg[in_arg] = 0;
            AddArg(str, macro, FindArg(macro->def, arg) + 1);
            in_arg = -1;
        }

        return VarcharTransfer(str);
    }

    return NULL;
}


void MacroFree(Macro *macro)
{
    if (macro)
    {
        int f;

        for(f = 0; f < macro->argc; f++)
        {
            free(macro->argv[f]);
        }

        free(macro->quoted);
        free(macro->argv);
        free(macro);
    }
}


const char *MacroName(Macro *macro)
{
    if (macro)
    {
        return macro->def->name;
    }

    return "(unknown)";
}


void MacroDump(FILE *fp)
{
    MacroDef *m = head;

    while(m)
    {
        int f;

        fprintf(fp, "; %s: MACRO", m->name);

        if (m->no_args)
        {
            for(f = 0; f < m->no_args; f++)
            {
                fprintf(fp, "%s%s", f == 0 ? " " : ", ", m->args[f]);
            }
        }

        putc('\n', fp);

        for(f = 0; f < m->no_lines; f++)
        {
            fprintf(fp, "; %s\n", m->lines[f]);
        }

        fprintf(fp, "; ENDM\n");

        m = m->next;

        if (m)
        {
            fprintf(fp, ";\n");
        }
    }
}


/*
vim: ai sw=4 ts=8 expandtab
*/
