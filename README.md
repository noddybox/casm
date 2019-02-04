# casm

Portable cross assembler.  Source control for this is now located in Subversion
at <a href="https://noddybox.co.uk/svn/casm/trunk">
https://noddybox.co.uk/svn/casm/trunk</a>.  Sorry, I tried to like GIT but 
found it's workflow not as comfortable as SVN for me.

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
* SPC700 (SNES sound chip - VERY untested)


## Output Formats

Currently **casm** supports the following output drivers:

* Raw binary output (works for Atari VCS)
* ZX Spectrum TAP file
* T64 Commodore 64 tape image
* ZX81 P file
* Gameboy ROM
* SNES ROM
* A simple library format for larger projects.
* NES ROM

## Latest changes

* Started trying to add Amstrad CPC output
* Made duplicate labels an error
* Fixes to 24-bit addresses
