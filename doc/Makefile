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
# Makefile for documentation
#

all: manual.html manual.pdf

manual.html: manual.asciidoc
	asciidoc manual.asciidoc

manual.pdf: manual.asciidoc
	a2x manual.asciidoc

clean:
	rm -f manual.html manual.pdf manual.xml
