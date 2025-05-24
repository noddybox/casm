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
# Tests Makefile
#

all: ../src/casm compare z80test 6502test

z80test: output/z80.bin output/z80.bin.asm
	@echo ========= Begin Z80 Test =========
	@./compare z80.asm output/z80.bin.asm && echo Passed
	@echo ========= End Z80 Test =========

output/z80.bin: Makefile z80.asm ../src/casm
	../src/casm z80.asm

output/z80.bin.asm: Makefile output/z80.bin
	dasm -c z80 -a -m output/z80.bin > output/z80.bin.asm

6502test: output/6502.bin output/6502.bin.asm
	@echo ========= Begin 6502 Test =========
	@./compare 6502.asm output/6502.bin.asm && echo Passed
	@echo ========= End 6502 Test =========

output/6502.bin: Makefile 6502.asm ../src/casm
	../src/casm 6502.asm

output/6502.bin.asm: Makefile output/6502.bin
	dasm -c 6502 -a -m output/6502.bin > output/6502.bin.asm

compare: compare.c
	$(CC) -o compare compare.c

../src/casm: ../src/*.c ../src/*.h
	cd ../src ; make

clean:
	rm -f output/*
	rm -f compare
