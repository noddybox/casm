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

    Stores the contents of the supplied sources.

*/

#ifndef CASM_SOURCE_H
#define CASM_SOURCE_H

/* ---------------------------------------- INTERFACES
*/

/* Read file data from passed file.  If the path is "-" then stdin is read.
   Will interpret and process include files.  Returns TRUE if the file was
   read OK, otherwise FALSE.
*/
int     SourceLoad(const char *path);

/* Check whether the sources have any lines (TRUE) or not (FALSE)
*/
int     SourceHasContents(void);

/* Rewind the sources to the start of the input
*/
void    SourceRewind(void);

/* Get the current line
*/
void    SourceRead(char buff[], size_t maxlen);

/* Move to the next line.  Returns FALSE when there is no next line.
*/
int     SourceAdvance(void);

/* Get the source filename for the current line
*/
const char *SourceGetPath(void);

/* Get the line number for the current line
*/
int     SourceGetLineNumber(void);

/* Get a bookmark to the current line
*/
void    *SourceGetBookmark(void);

/* Set the current line to the passed bookmark
*/
void    SourceSeek(void *bookmark);

/* Free any allocated dynamic memory
*/
void    SourceFree(void);


#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
