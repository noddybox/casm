# casm

Portable cross assembler.

## Usage

Simply pass it the file to assemble, i.e.

`casm source.txt`

Full documentation can be found in [HTML](https://deathstation9000.org.uk/cgit/cgit.cgi/casm/plain/doc/casm.html) format.

## Processors

Currently **casm** supports:

* Z80 (the default)
* 6502
* Gameboy CPU
* 65c816/Ricoh 5A22 (SNES)
* SPC700 (SNES sound chip - VERY untested)


## Output Formats

Currently **casm** supports the following output drivers:

* Raw binary output (works for Atari VCS)
* ZX Spectrum TAP file
* T64 Commodore 64 tape image
* ZX81 P file
* Gameboy ROM
* SNES ROM
* A simple library format for larger projects
* NES ROM
* Amstrad CPC CDT tape file
* Intel HEX format files

## Latest changes

* Added Intel HEX format output
