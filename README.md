# casm

Portable cross assembler.

## Usage

Simply pass it the file to assemble, i.e.

`casm source.txt`

Full documentation can be found in <a href="doc/casm.html">HTML</a> format.

## Processors

Currently **casm** supports:

* Z80 (the default)
* 6502

Plans for:

* Gameboy Z80 derarative
* Ricoh 5A22 (SNES)
* SPC700 (SNES sound chip)

## Output Formats

Currently **casm** supports:

* Raw binary output
* ZX Spectrum TAP file
* T64 Commodore 64 tape image
* ZX81 P file

Plans for:

* SNES ROM
* NES ROM
