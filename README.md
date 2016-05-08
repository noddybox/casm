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
* 65c816/Ricoh 5A22 (SNES)

Plans for:

* SPC700 (SNES sound chip)

## Output Formats

Currently **casm** supports the following output drivers:

* Raw binary output (works for Atari VCS)
* ZX Spectrum TAP file
* T64 Commodore 64 tape image
* ZX81 P file
* Gameboy ROM
* SNES ROM
* A simple library format for larger projects.

Plans for:

* NES ROM

## Major Changes in V1.2

* Added basic SNES support for processor and output.
* Added a library format for linking together of files in a larger project.
