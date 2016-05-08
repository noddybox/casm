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

    Tokeniser and parser.

*/

#ifndef CASM_PARSE_H
#define CASM_PARSE_H

/* ---------------------------------------- TYPES
*/

/* Represents a tokenised line.
   
   no_tokens is the number of tokens that can be accessed via token[i].

   If quoted[i] is non-zero it contains the opening character that was used to
   quote argument [i].  Note that '(' counts as a quote in command arguments.

   If first_column is true then the first column held a parsable character.

*/
typedef struct Line
{
    int         no_tokens;
    int         first_column;
    int         *quoted;
    char        **token;
    char        *comment;
} Line;


/* Used to represent a table of string/value pairs.  The table is ended with
   a NULL in str.
*/
typedef struct ValueTable
{
    const char  *str;
    int         value;
} ValueTable;


/* ---------------------------------------- MACROS
*/


/* Defines the set of values allowed for boolean strings and generates table
   entries using the passed 'tv' and 'fv' for true and false respictively.
*/
#define YES_NO_ENTRIES(tv, fv)  \
    {"on",      tv},            \
    {"true",    tv},            \
    {"yes",     tv},            \
    {"off",     fv},            \
    {"false",   fv},            \
    {"no",      fv}

/* ---------------------------------------- INTERFACES
*/

/* Parses a line.  Returns TRUE if line parsed OK, otherwise returns FALSE.
   ParseError() will return the reason for any failures.

   Remember that it may be possible to get a line with no tokens.
*/
int             ParseLine(Line *line, const char *source);


/* Free up the dynamic parts of a tokenised line
*/
void            ParseFree(Line *line);


/* Returns a reason for the last failure.
*/
const char      *ParseError(void);


/* Return a value from a table.  The check is done case insensitive.  Returns
   the value item, or NULL if not found.
*/
const ValueTable *ParseTable(const char *str, const ValueTable *table);


/* Parse a true/false value from a built-in table.  Returns def if the value is
   invalid.

   The mappings generated from YES_NO_ENTRIES(TRUE, FALSE) are used.
};

*/
int             ParseTrueFalse(const char *str, int def);


/* Returns true if the passed text is in the passed NULL terminated array of
   names.
*/
int             ParseInTable(const char *str, const char *names[]);

#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
