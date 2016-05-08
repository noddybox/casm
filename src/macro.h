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

#ifndef CASM_MACRO_H
#define CASM_MACRO_H

#include <stdlib.h>
#include <stdio.h>
#include "cmd.h"
#include "parse.h"

typedef struct mdef MacroDef;
typedef struct macro Macro;

/* ---------------------------------------- INTERFACES
*/

/* Macro options
*/
void MacroSetDefaults(void);

const ValueTable *MacroOptions(void);

CommandStatus MacroSetOption(int opt, int argc, char *argv[],
                             int quoted[], char *error, size_t error_size);



/* Create a new macro definition.  Returns NULL if the macro already exists
   or cannot be created and updates the passed error string.
   argc/argv is a list of the parameters.  argv can be NULL if argc is zero.
*/
MacroDef        *MacroCreate(const char *name, int argc, char *argv[],
                             char *err, size_t errsize);


/* Record a macro's contents. 
*/
void            MacroRecord(MacroDef *macro, const char *line);


/* Find a macro, placing a pointer to it in the passed macro or NULL if not
   known, or an error occurred.  Records the passed arguments for playing back
   the macro.  argv[0] is the macro name.

   Will return CMD_NOT_KNOWN if the macro is unknown.  Returns CMD_FAILED if
   there is a problem with the macro, e.g. insufficient arguments.  Returns
   CMD_OK when the macro if found and OK.
*/
CommandStatus   MacroFind(Macro **macro, int argc, char *argv[], int quoted[],
                          char *err, size_t errsize);


/* Playback a found macro.  Returns the next line, or NULL if the macro has
   finished.  The returned line is argument expanded and must be free()ed
   after use.
*/
char            *MacroPlay(Macro *macro);


/* Free a macro once it's finished playing.
*/
void            MacroFree(Macro *macro);


/* Get the name of a running macro
*/
const char      *MacroName(Macro *macro);


/* Dump out the macro definitions, commented
*/
void            MacroDump(FILE *fp);

#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
