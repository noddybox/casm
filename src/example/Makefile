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
# Makefile for examples
#

ALL	= spectrum.tap t64.t64 zx81.p gb.gb vcs.bin snes.sfc nes.nes cpc.cdt \
	  prg.prg hex.hex vic20.prg vic20+8k.prg c64.tap vic20.tap vic20+8k.tap
CASM	= ../casm

all: $(ALL) $(CASM)

$(CASM): ../*.[ch]
	cd .. ; make

remake: clean all

spectrum.tap: spectrum.asm $(CASM)
	$(CASM) spectrum.asm

t64.t64: t64.asm $(CASM)
	$(CASM) t64.asm

cpc.cdt: cpc.asm $(CASM)
	$(CASM)	cpc.asm

zx81.p: zx81.asm $(CASM)
	$(CASM) zx81.asm

gb.gb: gb.asm $(CASM)
	$(CASM) gb.asm

vcs.bin: vcs.asm $(CASM)
	$(CASM) vcs.asm

snes.sfc: snes.asm $(CASM)
	$(CASM) snes.asm

nes.nes: nes.asm tiles.chr nes.pal $(CASM)
	$(CASM) nes.asm

prg.prg: prg.asm $(CASM)
	$(CASM) prg.asm

hex.hex: emucpm.z80 $(CASM)
	$(CASM) emucpm.z80

vic20.prg: vic20.asm $(CASM)
	$(CASM) vic20.asm

vic20+8k.prg: vic20+8k.asm $(CASM)
	$(CASM) vic20+8k.asm

c64.tap: c64-tap.asm $(CASM)
	$(CASM) c64-tap.asm

vic20.tap: vic20-tap.asm $(CASM)
	$(CASM) vic20-tap.asm

vic20+8k.tap: vic20+8k-tap.asm $(CASM)
	$(CASM) vic20+8k-tap.asm

clean:
	rm -f $(ALL)

