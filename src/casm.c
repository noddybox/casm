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

    Main

*/
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include "global.h"
#include "expr.h"
#include "label.h"
#include "macro.h"
#include "parse.h"
#include "cmd.h"
#include "state.h"
#include "codepage.h"
#include "stack.h"
#include "listing.h"
#include "alias.h"
#include "output.h"

/* ---------------------------------------- PROCESSORS
*/
#include "z80.h"
#include "6502.h"


/* ---------------------------------------- MACROS
*/


/* ---------------------------------------- VERSION INFO
*/

static const char *casm_usage =
"Version 1.0\n"
"\n"
"This program is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"GNU General Public License (Version 3) for more details.\n"
"\n"
"usage: casm file\n";


/* ---------------------------------------- TYPES
*/

/* Defines a CPU
*/
typedef struct
{
    const char          *name;
    int                 mem_size;
    WordMode            word_mode;
    void                (*init)(void);
    const ValueTable    *(*options)(void);
    CommandStatus       (*set_option)(int o, int c, char *a[],
                                      int q[], char *e, size_t s);
    Command             handler;
} CPU;


/* ---------------------------------------- GLOBALS
*/
static const CPU cpu_table[]=
{
    {
        "Z80",
        0x10000,
        LSB_Word,
        Init_Z80, Options_Z80, SetOption_Z80, Handler_Z80
    },
    {
        "6502",
        0x10000,
        LSB_Word,
        Init_6502, Options_6502, SetOption_6502, Handler_6502
    },

    {NULL}
};

#define NO_CPU  (sizeof(cpu)/sizeof(cpu[0]))

static const CPU *cpu = cpu_table;


/* ---------------------------------------- PROTOS
*/
static void             CheckLimits(void);

static void             InitProcessors(void);        

static void             RunPass(const char *name, FILE *, int depth);
static void             ProduceOutput(void);


/* ---------------------------------------- INTERNAL COMMAND HANDLING
*/

static CommandStatus EQU(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    int result;

    CMD_LABEL_CHECK;
    CMD_ARGC_CHECK(2);
    CMD_EXPR(argv[1], result);

    LabelSet(label, result, ANY_LABEL);

    return CMD_OK;
}

static CommandStatus ORG(const char *label, int argc, char *argv[],
                         int quoted[], char *err, size_t errsize)
{
    int result;

    CMD_ARGC_CHECK(2);
    CMD_EXPR(argv[1], result);

    SetPC(result);

    /* If there was a label, set that as well
    */
    if (label)
    {
        LabelSet(label, result, ANY_LABEL);
    }

    /* See if an optional bank was supplied
    */
    if (argc > 2)
    {
        CMD_EXPR(argv[2], result);
        SetAddressBank(result);
    }

    return CMD_OK;
}

static CommandStatus BANK(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    int result;

    CMD_ARGC_CHECK(2);
    CMD_EXPR(argv[1], result);

    SetAddressBank(result);

    return CMD_OK;
}

static CommandStatus DS(const char *label, int argc, char *argv[],
                        int quoted[], char *err, size_t errsize)
{
    int count;
    int value = 0;
    int f;

    CMD_ARGC_CHECK(2);
    CMD_EXPR(argv[1], count);

    for(f = 0; f < count; f++)
    {
        if (argc > 2)
        {
            CMD_EXPR(argv[2], value);
        }

        PCWrite(value);
    }

    return CMD_OK;
}

static CommandStatus DefineMem(const char *label, int argc, char *argv[],
                               int quoted[], char *err, size_t errsize,
                               int bitsize)
{
    int f;

    CMD_ARGC_CHECK(2);

    for(f = 1; f < argc; f++)
    {
        if (quoted[f])
        {
            const char *p = argv[f];

            while(*p)
            {
                int val = CodepageConvert(*p++);

                if (bitsize == 8)
                {
                    PCWrite(val);
                }
                else
                {
                    PCWriteWord(val);
                }
            }
        }
        else
        {
            int val;

            CMD_EXPR(argv[f], val);

            if (bitsize == 8)
            {
                PCWrite(val);
            }
            else
            {
                PCWriteWord(val);
            }
        }
    }

    return CMD_OK;
}


static CommandStatus DB(const char *label, int argc, char *argv[],
                        int quoted[], char *err, size_t errsize)
{
    return DefineMem(label, argc, argv, quoted, err, errsize, 8);
}


static CommandStatus DW(const char *label, int argc, char *argv[],
                        int quoted[], char *err, size_t errsize)
{
    return DefineMem(label, argc, argv, quoted, err, errsize, 16);
}


static CommandStatus ALIGN(const char *label, int argc, char *argv[],
                           int quoted[], char *err, size_t errsize)
{
    int count;
    int value = 0;
    int f;

    CMD_ARGC_CHECK(2);
    CMD_EXPR(argv[1], count);

    if (count < 2 || count > 32768)
    {
        snprintf(err, errsize, "%s: Illegal align size %d", argv[0], count);
        return CMD_FAILED;
    }

    while ((PC() % count))
    {
        if (argc > 2)
        {
            CMD_EXPR(argv[2], value);
            PCWrite(value);
        }
        else
        {
            PCAdd(1);
        }
    }

    return CMD_OK;
}


static CommandStatus INCBIN(const char *label, int argc, char *argv[],
                            int quoted[], char *err, size_t errsize)
{
    FILE *fp;
    int num;
    unsigned char buff[4096];

    CMD_ARGC_CHECK(2);

    if (!(fp = fopen(argv[1], "rb")))
    {
        snprintf(err, errsize, "Failed to open '%s'", argv[1]);
        return CMD_FAILED;
    }

    while((num = fread(buff, 1, sizeof buff, fp)) > 0)
    {
        int f;

        for(f = 0; f < num; f++)
        {
            PCWrite(buff[f]);
        }
    }

    fclose(fp);

    return CMD_OK;
}


static CommandStatus ARCH(const char *label, int argc, char *argv[],
                          int quoted[], char *err, size_t errsize)
{
    int f;

    CMD_ARGC_CHECK(2);

    for(f = 0; cpu_table[f].name; f++)
    {
        if (CompareString(argv[1], cpu_table[f].name))
        {
            cpu = cpu_table + f;
            SetWordMode(cpu->word_mode);
            return CMD_OK;
        }
    }

    snprintf(err, errsize, "%s: unknown CPU '%s'\n", argv[0], argv[1]);

    return CMD_FAILED;
}


static CommandStatus OPTION(const char *label, int argc, char *argv[],
                            int quoted[], char *err, size_t errsize)
{
    const ValueTable *entry;
    char *argv_yes[2] = {"yes", NULL};
    char *argv_no[2] = {"no", NULL};
    int qu[1] = {0};
    int *q;
    char **args;
    int ac;
    char *opt;

    if (*argv[1] == '+' || *argv[1] == '-')
    {
        CMD_ARGC_CHECK(2);

        opt = argv[1] + 1;

        q = qu;
        ac = 1;

        if (*argv[1] == '+')
        {
            args = argv_yes;
        }
        else
        {
            args = argv_no;
        }
    }
    else
    {
        CMD_ARGC_CHECK(3);
        args = argv + 2;
        ac = argc - 2;
        q = quoted + 2;
        opt = argv[1];
    }

    /* TODO: There should be someway to make this better
    */
    if ((entry = ParseTable(opt, ListOptions())))
    {
        return ListSetOption(entry->value, ac, args, q, err, errsize);
    }
    else if ((entry = ParseTable(opt, MacroOptions())))
    {
        return MacroSetOption(entry->value, ac, args, q, err, errsize);
    }
    else if ((entry = ParseTable(opt, CodepageOptions())))
    {
        return CodepageSetOption(entry->value, ac, args, q, err, errsize);
    }
    else if ((entry = ParseTable(opt, OutputOptions())))
    {
        return OutputSetOption(entry->value, ac, args, q, err, errsize);
    }
    else if ((entry = ParseTable(opt, RawOutputOptions())))
    {
        return RawOutputSetOption(entry->value, ac, args, q, err, errsize);
    }
    else if ((entry = ParseTable(opt, SpecTAPOutputOptions())))
    {
        return SpecTAPOutputSetOption(entry->value, ac, args, q, err, errsize);
    }
    else if ((entry = ParseTable(opt, T64OutputOptions())))
    {
        return T64OutputSetOption(entry->value, ac, args, q, err, errsize);
    }
    else if ((entry = ParseTable(opt, ZX81OutputOptions())))
    {
        return ZX81OutputSetOption(entry->value, ac, args, q, err, errsize);
    }
    else if ((entry = ParseTable(opt, cpu->options())))
    {
        return cpu->set_option(entry->value, ac, args, q, err, errsize);
    }

    snprintf(err, errsize, "%s: unknown option %s", argv[0], opt);
    return CMD_FAILED;
}


static CommandStatus ALIAS(const char *label, int argc, char *argv[],
                           int quoted[], char *err, size_t errsize)
{
    CMD_ARGC_CHECK(3);

    AliasCreate(argv[1], argv[2]);

    return CMD_OK;
}


static CommandStatus NULLCMD(const char *label, int argc, char *argv[],
                             int quoted[], char *err, size_t errsize)
{
    return CMD_OK;
}


static struct
{
    const char *cmd;
    Command     handler;
} command_table[] =
{
    {"equ", EQU},
    {".equ", EQU},
    {"eq", EQU},
    {".eq", EQU},
    {"org", ORG},
    {".org", ORG},
    {"bank", BANK},
    {".bank", BANK},
    {"ds", DS},
    {".ds", DS},
    {"defs", DS},
    {".defs", DS},
    {"defb", DB},
    {".defb", DB},
    {"db", DB},
    {".db", DB},
    {"byte", DB},
    {".byte", DB},
    {"text", DB},
    {".text", DB},
    {"dw", DW},
    {".dw", DW},
    {"defw", DW},
    {".defw", DW},
    {"word", DW},
    {".word", DW},
    {"align", ALIGN},
    {".align", ALIGN},
    {"incbin", INCBIN},
    {".incbin", INCBIN},
    {"cpu", ARCH},
    {".cpu", ARCH},
    {"arch", ARCH},
    {".arch", ARCH},
    {"option", OPTION},
    {".option", OPTION},
    {"opt", OPTION},
    {".opt", OPTION},
    {"alias", ALIAS},
    {".alias", ALIAS},
    {"nullcmd", NULLCMD},
    {".nullcmd", NULLCMD},
    {NULL}
};


static CommandStatus RunInternal(const char *label, int argc, char *argv[],
                                 int quoted[], char *err, size_t errsize)
{
    int f;

    for(f = 0; command_table[f].cmd; f++)
    {
        if (CompareString(command_table[f].cmd, argv[0]))
        {
            return command_table[f].handler(label, argc, argv,
                                            quoted, err, errsize);
        }
    }

    return CMD_NOT_KNOWN;
}


/* ---------------------------------------- MAIN
*/
int main(int argc, char *argv[])
{
    FILE *fp = NULL;
    int done = FALSE;
    int f;

    CheckLimits();

    if (!argv[1])
    {
        fprintf(stderr,"%s\n", casm_usage);
        exit(EXIT_FAILURE);
    }

    fp = fopen(argv[1], "r");

    if (!fp)
    {
        fprintf(stderr,"Failed to read from %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    ClearState();

    SetPC(0);
    InitProcessors();

    while(!done)
    {
        RunPass(argv[1], fp, 0);
        rewind(fp);

        SetPC(0);
        MacroSetDefaults();
        AliasClear();
        InitProcessors();
        LabelResetNamespace();

        if (IsFinalPass())
        {
            done = TRUE;
        }
        else
        {
            NextPass();
        }
    }

    ProduceOutput();

    return EXIT_SUCCESS;
}


/* ---------------------------------------- UTIL
*/

/* Checks compiler limits
*/
static void CheckLimits(void)
{
    if (CHAR_BIT != 8)
    {
        fprintf(stderr,"Sorry - char must be 8 bits for correct operation\n\n");
        fprintf(stderr,"Actually, it may work - recompile with this\n");
        fprintf(stderr,"test disabled and let the author know please!\n");
        exit(EXIT_FAILURE);
    }

#if (INT_MAX < 0x10000)
    fprintf(stderr,"sorry - maximum int must be bigger than 0xffff\n");
    exit(EXIT_FAILURE);
#endif

#if (INT_MIN > -0x10000)
    fprintf(stderr,"sorry - minimum int must be bigger than -0xffff\n");
    exit(EXIT_FAILURE);
#endif
}

static void InitProcessors(void)
{
    int f;

    for(f = 0; cpu_table[f].name; f++)
    {
        cpu_table[f].init();
    }

    SetWordMode(cpu->word_mode);
}


/* ---------------------------------------- ASSEMBLY PASS
*/
static CommandStatus RunLine(const char *label, int argc, char *argv[],
                             int quoted[], char *err, size_t errsize)
{
    CommandStatus cmdstat = CMD_OK;

    /* Run internal then CPU commands
    */
    cmdstat = RunInternal(label, argc, argv, quoted, err, errsize);

    if (cmdstat == CMD_NOT_KNOWN)
    {
        cmdstat = cpu->handler(label, argc, argv, quoted, err, errsize);
    }

    return cmdstat;
}


static void RunPass(const char *name, FILE *fp, int depth)
{
    char src[4096];
    char err[4096];
    int line_no = 1;
    MacroDef *macro_def = NULL;
    Stack *macro_stack;
    Macro *macro = NULL;
    int skip_macro = FALSE;

    if (depth == 1024)
    {
        fprintf(stderr, "Include files too deep\n");
        exit(EXIT_FAILURE);
    }

    macro_stack = StackCreate();

    while(TRUE)
    {
        Line line;
        char *label = NULL;
        LabelType type;
        int cmd_offset = 0;
        CommandStatus cmdstat;
        char **argv;
        int argc;
        int *quoted;

        ListStartLine();

        if (macro)
        {
            char *next;

            next = MacroPlay(macro);

            if (next)
            {
                CopyStr(src, next, sizeof src);
                free(next);
            }
            else
            {
                ListMacroInvokeEnd(MacroName(macro));
                MacroFree(macro);
                macro = StackPop(macro_stack);
                LabelScopePop();
                goto next_line;
            }
        }
        
        if (!macro)
        {
            if (!fgets(src, sizeof src, fp))
            {
                if (macro_def)
                {
                    snprintf(err, sizeof err,"Unterminated macro");
                    cmdstat = CMD_FAILED;
                    goto error_handling;
                }

                StackFree(macro_stack);
                return;
            }
        }

        RemoveNL(src);

        if (!ParseLine(&line, src))
        {
            snprintf(err, sizeof err,"%s\n%s", src, ParseError());
            cmdstat = CMD_FAILED;
            goto error_handling;
        }

        /* Check for labels
        */
        if (line.first_column)
        {
            label = line.token[0];
            cmd_offset = 1;

            if (!LabelSanatise(label, &type))
            {
                snprintf(err, sizeof err, "Invalid label '%s'", label);
                cmdstat = CMD_FAILED;
                goto error_handling;
            }

            /* Check for setting global labels in macros.  This could have
               unexpected consequences.
            */
            if (macro && type == GLOBAL_LABEL)
            {
                snprintf(err, sizeof err, "Don't set global labels in macros");
                cmdstat = CMD_FAILED;
                goto error_handling;
            }

            /* This may well be updated by a command, but easier to set anyway
            */
            LabelSet(label, PC(), type);
        }

        /* Check for no command/label only.  Still record for macro though.
        */
        if (line.no_tokens == cmd_offset)
        {
            if (macro_def)
            {
                MacroRecord(macro_def, src);
            }

            ListLine(src);

            goto next_line;
        }

        argv = line.token + cmd_offset;
        argc = line.no_tokens - cmd_offset;
        quoted = line.quoted + cmd_offset;

        /* Expand aliases
        */
        argv[0] = AliasExpand(argv[0]);

        /* Check for END
        */
        if (CompareString(argv[0], "end") || CompareString(argv[0], ".end"))
        {
            ListLine(src);
            return;
        }

        /* Check for include
        */
        if (CompareString(argv[0], "include") ||
                CompareString(argv[0], ".include"))
        {
            FILE *fp;

            if (argc < 2)
            {
                snprintf(err, sizeof err, "%s: missing argument", argv[0]);
                cmdstat = CMD_FAILED;
                goto error_handling;
            }

            if (!(fp = fopen(argv[1], "r")))
            {
                snprintf(err, sizeof err, "%s: failed to open '%s'",
                                                    argv[0], argv[1]);
                cmdstat = CMD_FAILED;
                goto error_handling;
            }

            RunPass(argv[1], fp, depth + 1);

            goto next_line;
        }

        /* Check for macro definition
        */
        if (CompareString(argv[0], "macro"))
        {
            /* Only define macros on the first pass
            */
            if (IsFirstPass())
            {
                if (macro_def)
                {
                    snprintf(err, sizeof err,
                                        "macro: can't nest macro definitions");
                    cmdstat = CMD_FAILED;
                    goto error_handling;
                }

                if (!label)
                {
                    snprintf(err, sizeof err, "macro: missing name");
                    cmdstat = CMD_FAILED;
                    goto error_handling;
                }

                macro_def = MacroCreate(label, argc - 1,
                                               argc > 1 ? argv + 1 : NULL,
                                        err, sizeof err);

                if (!macro_def)
                {
                    cmdstat = CMD_FAILED;
                    goto error_handling;
                }
            }
            else
            {
                skip_macro = TRUE;
            }

            goto next_line;
        }

        if (CompareString(argv[0], "endm"))
        {
            if (!macro_def && IsFirstPass())
            {
                snprintf(err, sizeof err, "endm: No macro started");
                cmdstat = CMD_FAILED;
                goto error_handling;
            }

            macro_def = NULL;
            skip_macro = FALSE;
            goto next_line;
        }

        if (macro_def)
        {
            MacroRecord(macro_def, src);
            goto next_line;
        }

        if (skip_macro)
        {
            goto next_line;
        }

        /* Run internal then CPU commands.  Then if that fails try a macro.
        */
        cmdstat = RunInternal(label, argc, argv, quoted, err, sizeof err);

        if (cmdstat == CMD_NOT_KNOWN)
        {
            cmdstat = cpu->handler(label, argc, argv, quoted, err, sizeof err);
        }

        ListLine(src);

        if (cmdstat == CMD_NOT_KNOWN)
        {
            Macro *m;

            cmdstat = MacroFind(&m, argc, argv, quoted, err, sizeof err);

            /* If we get a macro then create a new top-level label for it
            */
            if (m)
            {
                if (macro)
                {
                    StackPush(macro_stack, macro);

                    if (StackSize(macro_stack) > 1023)
                    {
                        snprintf(err, sizeof err, "Macro invocation too deep");
                        cmdstat = CMD_FAILED;
                        goto error_handling;
                    }
                }

                macro = m;

                ListMacroInvokeStart(argc, argv, quoted);

                LabelScopePush(LabelCreateNamespace(), PC());
                cmdstat = CMD_OK;
            }
        }

error_handling:
        switch(cmdstat)
        {
            case CMD_OK:
                break;

            case CMD_OK_WARNING:
                ListError("%s(%d): WARNING %s", name, line_no, err);
                ListError("%s(%d): %s", name, line_no, src);

                if (macro)
                {
                    ListError("%s(%d): In macro '%s'",
                                        name, line_no, MacroName(macro));
                }

                break;

            case CMD_NOT_KNOWN:
                ListError("%s(%d): Unknown command/opcode '%s'",
                                        name, line_no, line.token[cmd_offset]);

                if (macro)
                {
                    ListError("%s(%d): In macro '%s'",
                                        name, line_no, MacroName(macro));
                }

                exit(EXIT_FAILURE);

                break;

            case CMD_FAILED:
                ListError("%s(%d): ERROR %s", name, line_no, err);
                ListError("%s(%d): %s", name, line_no, src);

                if (macro)
                {
                    ListError("%s(%d): In macro '%s'",
                                        name, line_no, MacroName(macro));
                }

                exit(EXIT_FAILURE);

                break;
        }

next_line:
        if (!macro)
        {
            line_no++;
        }

        ParseFree(&line);
    }
}


/* ---------------------------------------- OUTPUT
*/
static void ProduceOutput(void)
{
    ListFinish();

    if (!OutputCode())
    {
        fprintf(stderr, "%s\n", OutputError());
    }
}

/*
vim: ai sw=4 ts=8 expandtab
*/
