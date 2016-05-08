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

    Basic, global integral types

*/

#ifndef CASM_BASETYPE_H
#define CASM_BASETYPE_H

/* Used to represent a byte
*/
typedef unsigned char   Byte;

/* Used to represent a word
*/
typedef unsigned int    Word;

/* Get byte of word
*/
#define LOBYTE(w)       ((w)&0xff)
#define HIBYTE(w)       (((w)>>8)&0xff)

/* Set byte of word
*/
#define SET_LOBYTE(w,v) w=(w&0xff00)|(v)
#define SET_HIBYTE(w,v) w=(w&0x00ff)|((v)<<8)

#endif

/*
vim: ai sw=4 ts=8 expandtab
*/
