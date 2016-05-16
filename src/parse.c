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

    Tokeniser.

*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "global.h"
#include "codepage.h"
#include "parse.h"

/* ---------------------------------------- MACROS/TYPES
*/
typedef enum
{
    CONSUME_WS,
    CONSUME_TOKEN,
    FINISHED
} State;


/* ---------------------------------------- GLOBALS
*/
static char     error[1024];

static const ValueTable bool_table[] =
{
    YES_NO_ENTRIES(TRUE, FALSE),
    {NULL}
};



/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static void AddToken(const char *start, const char *end, Line *line, int quoted)
{
    char *tok = Malloc(end - start + 2);
    char *p;

    p = tok;

    while(start != end + 1)
    {
        *p++ = *start++;
    }

    *p = 0;

    Trim(tok);

    /* Special case to convert single characters in quotes to their character
       code.
    */
    if (*tok && *(tok+1) == 0 && (quoted == '\'' || quoted == '"'))
    {
        char b[64];

        snprintf(b, sizeof b, "%d", CodepageConvert(*tok));
        free(tok);

        tok = DupStr(b);
        quoted = 0;
    }

    line->no_tokens++;

    line->token = Realloc(line->token, (sizeof *line->token) * line->no_tokens);

    line->quoted = Realloc(line->quoted,
                            (sizeof *line->quoted) * line->no_tokens);

    line->token[line->no_tokens - 1] = tok;
    line->quoted[line->no_tokens - 1] = quoted;
}


static int IsClosure(const char *p, const char *sep)
{
    while(*p && isspace(*p))
    {
	p++;
    }

    return !*p || strchr(sep, *p);
}


/* ---------------------------------------- INTERFACES
*/

int ParseLine(Line *line, const char *source)
{
    static const char *sep_chars[2] =
    {
        " \t:",
        ","
    };

    static const char *quote_start_chars[2] =
    {
        "",
        "\"'(["
    };

    static const char *quote_end_chars[2] =
    {
        "",
        "\"')]"
    };

    const char *p = NULL;
    const char *start = NULL;
    State state = CONSUME_WS;
    int stage = 0;
    char open_quote = 0;
    char quote = 0;
    int status = TRUE;

    p = source;

    line->first_column = FALSE;
    line->token = NULL;
    line->comment = NULL;
    line->quoted = NULL;
    line->no_tokens = 0;

    while(state != FINISHED)
    {
        switch(state)
        {
            case CONSUME_WS:
                if (!*p)
                {
                    state = FINISHED;
                }
                else if (*p == ';')
                {
                    state = FINISHED;
                    line->comment = DupStr(p + 1);
                    Trim(line->comment);
                }
                else
                {
                    const char *ptr;

                    if ((ptr = strchr(quote_start_chars[stage], *p)))
                    {
                        open_quote = *p;
                        quote = quote_end_chars
                                        [stage]
                                        [ptr - quote_start_chars[stage]];

                        state = CONSUME_TOKEN;
                        start = ++p;
                    }
                    else
                    {
                        if (isspace((unsigned char)*p) || 
                                strchr(sep_chars[stage], *p))
                        {
                            p++;
                        }
                        else
                        {
                            state = CONSUME_TOKEN;
                            start = p;
                            open_quote = 0;
                            quote = 0;
                        }
                    }

                    if (source == start && state == CONSUME_TOKEN)
                    {
                        line->first_column = TRUE;
                    }
                }
                break;

            case CONSUME_TOKEN:
                if (!*p)
                {
                    if (quote)
                    {
                        snprintf
                            (error, sizeof error,
                             "Unterminated quoted string; expected %c", quote);

                        status = FALSE;
                    }
                    else if (start != p)
                    {
                        AddToken(start, p, line, FALSE);
                    }

                    state = FINISHED;
                }
                else
                {
                    if (quote)
                    {
                        /* When we find a closing quote check if the following
                           character is a seperator or end-of-line.  If not
                           this wasn't really a quoted string.
                        */
                        if (*p == quote)
                        {
			    if (IsClosure(p + 1, sep_chars[stage]))
                            {
                                state = CONSUME_WS;
                            }
                            else
                            {
                                /* Wasn't a real closue, so add the quote as
                                   a character onto the string.
                                */
                                quote = 0;
                                open_quote = 0;
                                start--;
                            }
                        }
                    }
                    else if (strchr(sep_chars[stage], *p) || *p == ';')
                    {
                        state = CONSUME_WS;
                    }

                    if (state == CONSUME_WS)
                    {
                        AddToken(start, p - 1, line, open_quote);

                        if (quote)
                        {
                            open_quote = 0;
                            quote = 0;
                            p++;
                        }

                        if ((line->no_tokens == 2 && line->first_column) ||
                            (line->no_tokens == 1 && !line->first_column))
                        {
                            stage = 1;
                        }
                    }
                    else
                    {
                        p++;
                    }
                }
                break;

            default:
                break;
        }
    }

    return status;
}


void ParseFree(Line *line)
{
    int f;

    if (line->token)
    {
        for(f = 0; f < line->no_tokens; f++)
        {
            free(line->token[f]);
        }

        free(line->token);
        free(line->quoted);
    }

    if (line->comment)
    {
        free(line->comment);
    }

    line->token = NULL;
    line->quoted = NULL;
    line->comment = NULL;
}


const char *ParseError(void)
{
    return error;
}

const ValueTable *ParseTable(const char *str, const ValueTable *table)
{
    while(table && table->str)
    {
        if (CompareString(str, table->str))
        {
            return table;
        }

        table++;
    }

    return NULL;
}


int ParseTrueFalse(const char *str, int def)
{
    const ValueTable *val;

    val = ParseTable(str, bool_table);

    return val == NULL ? def : val->value;
}


int ParseInTable(const char *str, const char *names[])
{
    int f;

    for(f = 0; names[f]; f++)
    {
        if (CompareString(str, names[f]))
        {
            return TRUE;
        }
    }

    return FALSE;
}



/*
vim: ai sw=4 ts=8 expandtab
*/
