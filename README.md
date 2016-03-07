# casm

Portable cross assembler.

## Usage

Simply pass it the file to assemble, i.e.

`casm source.txt`

Full documentation can be found in either [PDF](docs/manual.pdf) or
[HTML](docs/manual.html) format.

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

Plans for:

	* D64 Commodore 1571 disk image
	* T64 Commodore 64 tape image
	* SNES ROM
	* NES ROM
