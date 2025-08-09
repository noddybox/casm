/*

    casm - Simple, portable assembler

    Copyright (C) 2003-2025  Ian Cowburn (ianc@noddybox.co.uk)

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

    Store the contents of the passed sources.

*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "global.h"
#include "source.h"
#include "parse.h"


/* ---------------------------------------- TYPES AND GLOBALS
*/
typedef struct SourceLine
{
    char        *line;
    const char  *filename;
    int         line_number;
    struct SourceLine *next;
} SourceLine;

static SourceLine       *head;
static SourceLine       *tail;
static SourceLine       *current;

/* ---------------------------------------- PRIVATE FUNCTIONS
*/
static void AddSourceLine(char *line, const char *filename, int line_number)
{
    SourceLine *new = Malloc(sizeof *new);

    new->line = line;
    new->filename = filename;
    new->line_number = line_number;
    new->next = NULL;

    if (tail)
    {
        tail->next = new;
        tail = new;
    }
    else
    {
        head = new;
        tail = new;
    }
}

/* ---------------------------------------- INTERFACES
*/
int SourceLoad(const char *path)
{
    int line_no = 1;
    char buff[CASM_MAX_LINE_LENGTH];
    FILE *fp;

    if (!path || strcmp(path, "-") == 0)
    {
        fp = stdin;
        path = "(stdin)";
    }
    else
    {
        fp = fopen(path, "r");

        if (!fp)
        {
            fprintf(stderr, "Failed to open '%s'\n", path);
            return FALSE;
        }
    }

    while(fgets(buff, sizeof buff, fp))
    {
        Line line;

        Trim(buff);

        if (!ParseLine(&line, buff))
        {
            fprintf(stderr, "%s:%d %s\n", path, line_no, ParseError());
            ParseFree(&line);
            return FALSE;
        }

        AddSourceLine(DupStr(buff), path, line_no);
        line_no++;
        ParseFree(&line);
    }

    if (fp != stdin)
    {
        fclose(fp);
    }

    if (head)
    {
        current = head;
    }

    return TRUE;
}

int SourceHasContents(void)
{
    return head != NULL;
}

void SourceRewind(void)
{
    current = head;
}

void SourceRead(char buff[], size_t maxlen)
{
    CopyStr(buff, current->line, maxlen);
}

int SourceAdvance(void)
{
    if (current == tail)
    {
        return 0;
    }

    current = current->next;
    return 1;
}

const char *SourceGetPath(void)
{
    return current->filename;
}

int SourceGetLineNumber(void)
{
    return current->line_number;
}

void *SourceGetBookmark(void)
{
    return current;
}

void SourceSeek(void *bookmark)
{
    current = bookmark;
}

void SourceFree(void)
{
    SourceLine *l = head;

    while(l)
    {
        SourceLine *t = l;

        free(l->line);
        l = l->next;
        free(t);
    }
}

/*
vim: ai sw=4 ts=8 expandtab
*/
