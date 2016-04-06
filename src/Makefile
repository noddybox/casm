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
		z80.c		\
		rawout.c	\
		specout.c	\
		t64out.c	\
		zx81out.c

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
		z80.o		\
		rawout.o	\
		specout.o	\
		t64out.o	\
		zx81out.o

$(TARGET): $(OBJECTS)
	$(CC) $(CLAGS) -o $(TARGET) $(OBJECTS)

clean:
	rm -f $(TARGET) $(TARGET).exe $(OBJECTS) core *.core

6502.o: 6502.c global.h basetype.h util.h state.h expr.h label.h parse.h \
  cmd.h codepage.h 6502.h
alias.o: alias.c global.h basetype.h util.h state.h alias.h
casm.o: casm.c global.h basetype.h util.h state.h expr.h label.h macro.h \
  cmd.h parse.h codepage.h stack.h listing.h alias.h output.h rawout.h \
  specout.h t64out.h zx81out.h z80.h 6502.h
codepage.o: codepage.c global.h basetype.h util.h state.h codepage.h \
  parse.h cmd.h
expr.o: expr.c global.h basetype.h util.h state.h expr.h label.h
label.o: label.c global.h basetype.h util.h state.h codepage.h parse.h \
  cmd.h stack.h label.h
listing.o: listing.c global.h basetype.h util.h state.h label.h macro.h \
  cmd.h parse.h expr.h varchar.h listing.h
macro.o: macro.c global.h basetype.h util.h state.h codepage.h parse.h \
  cmd.h varchar.h macro.h
output.o: output.c global.h basetype.h util.h state.h output.h parse.h \
  cmd.h rawout.h specout.h t64out.h zx81out.h
parse.o: parse.c global.h basetype.h util.h state.h codepage.h parse.h \
  cmd.h
rawout.o: rawout.c global.h basetype.h util.h state.h rawout.h parse.h \
  cmd.h
specout.o: specout.c global.h basetype.h util.h state.h specout.h parse.h \
  cmd.h
stack.o: stack.c global.h basetype.h util.h state.h stack.h
state.o: state.c global.h basetype.h util.h state.h expr.h
t64out.o: t64out.c global.h basetype.h util.h state.h codepage.h parse.h \
  cmd.h t64out.h
util.o: util.c global.h basetype.h util.h state.h
varchar.o: varchar.c global.h basetype.h util.h state.h codepage.h \
  parse.h cmd.h varchar.h
z80.o: z80.c global.h basetype.h util.h state.h expr.h label.h parse.h \
  cmd.h codepage.h varchar.h z80.h
zx81out.o: zx81out.c global.h basetype.h util.h state.h codepage.h \
  parse.h cmd.h zx81out.h
