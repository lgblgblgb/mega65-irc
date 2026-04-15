/* Copyright (C)2026 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef ARCH_H_INCLUDED
#define ARCH_H_INCLUDED
#ifndef MEGA65

/* -------------------- UNIX -------------------- */

#include <stdbool.h>
#include <SDL.h>

typedef Uint8 byte;
typedef Uint16 word;

extern byte screen[80 * 25];
extern byte colour[80 * 25];

extern void arch_refresh ( void );
extern byte arch_getkey ( void );

#else

/* -------------------- MEGA65 -------------------- */

typedef unsigned char byte;
typedef unsigned int word;

#define screen ((byte*)(0x0800U))
#define colour ((byte*)(0xF800U))

#define POKE(addr,val)     (*(unsigned char*) (addr) = (val))
#define POKEW(addr,val)    (*(unsigned*) (addr) = (val))
#define PEEK(addr)         (*(unsigned char*) (addr))
#define PEEKW(addr)        (*(unsigned*) (addr))

//extern void arch_refresh ( void );
#define arch_refresh()
extern byte arch_getkey ( void );
extern void mega65_scroller ( void );
extern void sprite_init ( void );

#endif
#endif
