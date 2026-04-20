# Silly iRC client for MEGA65 using MEGA-IP for TCP/IP networking

This repository contains a silly iRC client for MEGA65 which can be compiled and
used on UNIX-like systems as well (SDL2). This allows easier development, since
most of the main C code is shared between the two versions, only the low level
code differs.

!! TODO, currently does not work too much other than experimenting !!

Copyright (C)2026 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version. See file `LICENSE`.

This client uses MEGA-IP (https://github.com/mega65-c65/mega-ip) which was not
written by me, but it's "Released under the PUBLIC DOMAIN".

## Quick usage howto

TODO, if it does something useful ;)

## Just download

Look into the `bin/` directory for a D81 image or for a `PRG` file.

## Compile yourself

To compile, you need the `CC65` suite. The provided MEGA-IP package requires
however 64TASS, but it's already pre-compiled, so unless you want to modify
MEGA-IP itself, you don't need that at all. You also need GNU make.

On the UNIX side, you need a UNIX-like OS (like Linux), SDL2 development
package, C compiler.

Simple `make` command (`gmake` on BSD systems for GNU make) compiles both of
the UNIX and the MEGA65 version. `irc.prg` is the MEGA65 program, while `irc`
is the UNIX executable.

## Memory layout (on MEGA65)

"All RAM" config without mapping, except for the last 8K which is mapped
to the last 8K in bank1, to have the 2K colour RAM (without the need to
fiddle with the I/O feature to switch 2K colour RAM on and off).

The lower 32K RAM is normally in not mapped state, unless MEGA-IP (`eth.bin`)
is called, when it's mapped to bank 4, expect for the very first 8K. Since
the program code itself is there too, `eth.bin` functions are called via
the `HIMEM` segment, which starts at `$E000`, but since the last 8K in the
upper half of 64K is mapped to bank1, physically it's in bank1.


This program written in assembly and C. The size of the binary is quite large,
the main reasons:

* It contains a 4K long 8x16 pixel "VGA-style" font
* It contains the ~16K long `eth.bin` embedded (MEGA-IP)
* C code compiled by `CC65`, is not so short ...

## Source files

### arch_mega65_lowlevel.asm

MEGA65 specific low level assembly routines, and the BASIC stub itself with the
init code. It also contains things like the "gateway" to call MEGA-IP (`eth.bin`)
routines (which requires memory mapping change, so those routines are in the
HIMEM segment). It also takes care of `font.bin` to copy into the "character RAM"
of MEGA65, also putting `eth.bin` into its place (band 4, from `$2000`).

### arch_mega65.c

MEGA65 specific C code.

### arch_unix.c

UNIX specific C code, including the SDL2 level initialization, event handling,
and screen rendering.

### common.c

Common C routines between the MEGA65 and UNIX versions, mainly screen write
routines and such.

### assets/font-mega65.bin

Binary 4K font (8x16 pixels), in MEGA65 "interlaced" format. It's a standard
VGA font, which is needed to use ASCII directly, without dealing PETSCII-ASCII
translation all the time and other problems. Also, a 16 pixel height font looks
way nicer.

The font itself comes from the SeaBIOS open source project which includes this
comment:

These fonts come from ftp://ftp.simtel.net/pub/simtelnet/msdos/screen/fntcol16.zip
The package is (c) by Joseph Gil. The individual fonts are public domain.

### assets/font-unix.h

The same font as in font.bin but in continous format and in C source code
syntax.

### irc.c

The client's main logic, shared between MEGA65 and UNIX versions.

### net_unix.c

Networking implementation for the UNIX version using non blocking I/O and `poll()`.

### net_mega65.c

Networking implementation for the MEGA65 versions, using MEGA-IP, through the
"bridge" implemented in `arch_mega65_lowlevel.asm`.

### megaip/*.asm

The MEGA-IP (`eth.bin`) source code, pre-compiled already into `eth.bin` and labels
to `eth.i65`. If you ever need to modify these files, you must run `make` in the
megaip/ directory (you also need 64tass assembler for that), then going back
to the project root, you need `make clean` and `make` to safely rebuild things.

For more information on this project, please visit: https://github.com/mega65-c65/mega-ip

NOTE: MEGA-IP was not written by me.

### assets/mega65.ld

Linker configuration file for `ld65` (from the `CC65` package). It's basically a
hacked C64 default config, adjusted to MEGA65 and expanded with my needs.
