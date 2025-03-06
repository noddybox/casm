# casm - Simple, portable assembler
#
# Copyright (C) 2003-2015  Ian Cowburn (ianc@noddybox.demon.co.uk)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# -------------------------------------------------------------------------
#
# Makefile
#

CFLAGS +=	-g

TARGET	=	casm

SOURCE	=	casm.c		\
		expr.c		\
		label.c		\
		macro.c		\
		parse.c		\
		state.c		\
		codepage.c	\
		util.c		\
		output.c	\
		varchar.c	\
		stack.c		\
		listing.c	\
		alias.c		\
		6502.c		\
		65c816.c	\
		z80.c		\
		spc700.c	\
		gbcpu.c		\
		rawout.c	\
		specout.c	\
		t64out.c	\
		zx81out.c	\
		gbout.c		\
		snesout.c	\
		libout.c	\
		nesout.c	\
		cpcout.c	\
		prgout.c        \
                hexout.c	\
		68000.c

OBJECTS	=	casm.o		\
		expr.o		\
		label.o		\
		macro.o		\
		parse.o		\
		state.o		\
		codepage.o	\
		util.o		\
		output.o	\
		varchar.o	\
		stack.o		\
		listing.o	\
		alias.o		\
		6502.o		\
		65c816.o	\
		z80.o		\
		spc700.o	\
		gbcpu.o		\
		rawout.o	\
		specout.o	\
		t64out.o	\
		zx81out.o	\
		gbout.o		\
		snesout.o	\
		libout.o	\
		nesout.o	\
		cpcout.o	\
		prgout.o        \
                hexout.o	\
		68000.o

$(TARGET): $(OBJECTS)
	$(CC) $(CLAGS) -o $(TARGET) $(OBJECTS)

clean:
	rm -f $(TARGET) $(TARGET).exe $(OBJECTS) core *.core

6502.o: 6502.c global.h basetype.h util.h state.h expr.h label.h parse.h \
 cmd.h codepage.h 6502.h
65c816.o: 65c816.c global.h basetype.h util.h state.h expr.h label.h \
 parse.h cmd.h codepage.h 65c816.h
68000.o: 68000.c global.h basetype.h util.h state.h expr.h label.h \
 parse.h cmd.h codepage.h 68000.h
alias.o: alias.c global.h basetype.h util.h state.h alias.h
casm.o: casm.c global.h basetype.h util.h state.h expr.h label.h macro.h \
 cmd.h parse.h codepage.h stack.h listing.h alias.h output.h rawout.h \
 specout.h t64out.h zx81out.h gbout.h snesout.h libout.h nesout.h \
 cpcout.h prgout.h hexout.h z80.h 6502.h gbcpu.h 65c816.h spc700.h
codepage.o: codepage.c global.h basetype.h util.h state.h codepage.h \
 parse.h cmd.h
cpcout.o: cpcout.c global.h basetype.h util.h state.h codepage.h parse.h \
 cmd.h cpcout.h expr.h
expr.o: expr.c global.h basetype.h util.h state.h expr.h label.h
gbcpu.o: gbcpu.c global.h basetype.h util.h state.h expr.h label.h \
 parse.h cmd.h codepage.h varchar.h gbcpu.h
gbout.o: gbout.c global.h basetype.h util.h state.h expr.h codepage.h \
 parse.h cmd.h gbout.h
hexout.o: hexout.c global.h basetype.h util.h state.h codepage.h parse.h \
 cmd.h hexout.h expr.h
label.o: label.c global.h basetype.h util.h state.h codepage.h parse.h \
 cmd.h stack.h label.h
libout.o: libout.c global.h basetype.h util.h state.h libout.h parse.h \
 cmd.h label.h
listing.o: listing.c global.h basetype.h util.h state.h label.h macro.h \
 cmd.h parse.h expr.h varchar.h listing.h
macro.o: macro.c global.h basetype.h util.h state.h codepage.h parse.h \
 cmd.h varchar.h macro.h
nesout.o: nesout.c global.h basetype.h util.h state.h expr.h codepage.h \
 parse.h cmd.h nesout.h
output.o: output.c global.h basetype.h util.h state.h output.h parse.h \
 cmd.h rawout.h specout.h t64out.h zx81out.h gbout.h snesout.h libout.h \
 nesout.h cpcout.h prgout.h hexout.h
parse.o: parse.c global.h basetype.h util.h state.h codepage.h parse.h \
 cmd.h
prgout.o: prgout.c global.h basetype.h util.h state.h codepage.h parse.h \
 cmd.h prgout.h expr.h
rawout.o: rawout.c global.h basetype.h util.h state.h rawout.h parse.h \
 cmd.h
snesout.o: snesout.c global.h basetype.h util.h state.h expr.h codepage.h \
 parse.h cmd.h snesout.h
spc700.o: spc700.c global.h basetype.h util.h state.h expr.h label.h \
 parse.h cmd.h codepage.h spc700.h
specout.o: specout.c global.h basetype.h util.h state.h specout.h parse.h \
 cmd.h expr.h
stack.o: stack.c global.h basetype.h util.h state.h stack.h
state.o: state.c global.h basetype.h util.h state.h expr.h
t64out.o: t64out.c global.h basetype.h util.h state.h codepage.h parse.h \
 cmd.h t64out.h expr.h
util.o: util.c global.h basetype.h util.h state.h
varchar.o: varchar.c global.h basetype.h util.h state.h codepage.h \
 parse.h cmd.h varchar.h
z80.o: z80.c global.h basetype.h util.h state.h expr.h label.h parse.h \
 cmd.h codepage.h varchar.h z80.h
zx81out.o: zx81out.c global.h basetype.h util.h state.h codepage.h \
 parse.h cmd.h zx81out.h
