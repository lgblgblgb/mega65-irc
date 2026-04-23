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

#include "arch.h"
#include "common.h"
#include <string.h>


void arch_set_status_bg ( const byte line_no, const byte bg_colour )
{
	// TODO: clean this up!!!
	memset((void*)1024, 0, 1024);
	memset((void*)1024, 0xFF, 56);
	POKE(0xD000, 24);			// sprite X coordinate
	POKE(0xD001, 50 + line_no * 8);		// Sprite Y coordinate
	POKE(0xD01D, 0);			// sprite X MSBs
	POKE(0xD027, bg_colour);		// sprite colour
	POKE(0xD015, 1);			// sprite enable
	POKE(0xD01B, 1);			// sprite prio
	POKE(0xD04D, PEEK(0xD04D) | 0x10);	// horizontal tiling
	POKE(0xD057, 1);			// sprite extended width
	POKE(0xff8, 1024/64);			// sprite pointer
	POKE(0xD076, 0);			// V400 sprites turned off
}
