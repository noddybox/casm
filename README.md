# casm

Portable cross assembler.

## Usage

Simply pass it the file to assemble, i.e.

`casm source.txt`

Full documentation can be found in
<a href="https://rawgit.com/noddybox/casm/master/doc/casm.html">HTML</a> format.

## Processors

Currently **casm** supports:

* Z80 (the default)
* 6502
* Gameboy CPU

Plans for:

* Gameboy Z80 derarative
* Ricoh 5A22 (SNES)
* SPC700 (SNES sound chip)

## Output Formats

Currently **casm** supports the following output drivers:

* Raw binary output (works for Atari VCS)
* ZX Spectrum TAP file
* T64 Commodore 64 tape image
* ZX81 P file
* Gameboy ROM

Plans for:

* SNES ROM
* NES ROM
