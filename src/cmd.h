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

    Command callback definitions

*/

#ifndef CASM_CMD_H
#define CASM_CMD_H

/* ---------------------------------------- TYPES
*/


typedef enum
{
    CMD_OK,
    CMD_OK_WARNING,
    CMD_FAILED,
    CMD_NOT_KNOWN
} CommandStatus;

/* Used to implement a command.
*/
typedef CommandStatus (*Command)(const char *label, int argc, char *argv[],
                                 int quoted[], char *err, size_t errsize);


/* ---------------------------------------- HELPER MACROS
*/


/* Check for the precesence of a label.  Expects a variable called 'label'

   Args:
        NONE

   Expects:

        argv    - The argv passed to the command handler
        label   - The label passed to the command handler
        err     - The error location passed to the command handler
        errsize - The size of the error location passed to the command handler
*/
#define CMD_LABEL_CHECK                                                 \
do                                                                      \
{                                                                       \
    if (!label)                                                         \
    {                                                                   \
        snprintf(err, errsize, "%s: missing label", argv[0]);           \
        return CMD_FAILED;                                              \
    }                                                                   \
} while(0)



/* Check for the arg count being at least a specified number.

   Args:
        min     - The minimum number of arguments to support

   Expects:

        argc    - The argc passed to the command handler
        argv    - The argv passed to the command handler
        err     - The error location passed to the command handler
        errsize - The size of the error location passed to the command handler
*/
#define CMD_ARGC_CHECK(min)                                             \
do                                                                      \
{                                                                       \
    if (argc < min)                                                     \
    {                                                                   \
        snprintf(err, errsize, "%s: missing argument", argv[0]);        \
        return CMD_FAILED;                                              \
    }                                                                   \
} while(0)




/* Evaluate an expression.

   Args:
        expr    - The expression to evaluate
        result  - The variable to hold the result

   Expects:

        argc    - The argc passed to the command handler
        argv    - The argv passed to the command handler
        err     - The error location passed to the command handler
        errsize - The size of the error location passed to the command handler
*/
#define CMD_EXPR(expr, result)                                          \
do                                                                      \
{                                                                       \
    if (!ExprEval(expr, &result))                                       \
    {                                                                   \
        snprintf(err, errsize, "%s: expression error:  %s",             \
                                        argv[0], ExprError());          \
        return CMD_FAILED;                                              \
    }                                                                   \
} while(0)



/* Parse a table using ParseTable()

   Args:
        str     - The string to look for
        table   - The table to parse
        result  - Variable to hold the return from ParseTable()

   Expects:

        argv    - The argv passed to the command handler
        err     - The error location passed to the command handler
        errsize - The size of the error location passed to the command handler
*/
#define CMD_TABLE(str, table, result)                                   \
do                                                                      \
{                                                                       \
    if (!(result = ParseTable(str, table)))                             \
    {                                                                   \
        snprintf(err, errsize, "unknown value: \"%s\"", str);           \
        return CMD_FAILED;                                              \
    }                                                                   \
} while(0)



#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
