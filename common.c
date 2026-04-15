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

#define TEXT_COLOUR		1
#define CURSOR_COLOUR		5
#define INPUT_COLOUR		14
#define STATUS_BG_COLOUR	2
#define STATUS_FG_COLOUR	7


static word line_addr = 0;
static byte x_pos = 0;
byte text_colour = TEXT_COLOUR;
static word inp_addr;


void write_char ( const byte c )
{
	if (c == 10)
		return;
	if (c == 13) {
		if (x_pos) {
			line_addr += 80;
			x_pos = 0;
		}
		return;
	}
	if (line_addr >= 23*80) {
		line_addr = 22 * 80;
#ifdef		MEGA65
		mega65_scroller();
#else
		memmove(screen, screen + 80, 22 * 80);
		memmove(colour, colour + 80, 22 * 80);
		memset(screen + 22 * 80, 32, 80);
#endif
	}
	screen[line_addr + x_pos] = c;
	colour[line_addr + x_pos] = text_colour;
	x_pos++;
	if (x_pos == 80) {
		x_pos = 0;
		line_addr += 80;
	}
}


void write_string ( const char *s )
{
	while (*s)
		write_char(*s++);
}


void write_dec ( word d )
{
	byte take = 0;
	word div;
	for (div = 10000; div; div /= 10) {
		const byte r = d / div;
		d %= div;
		if (r || take || div == 1) {
			write_char(r | '0');
			take = 1;
		}
	}
}


void write_ip ( const byte *p )
{
	write_dec(p[0]);
	write_char('.');
	write_dec(p[1]);
	write_char('.');
	write_dec(p[2]);
	write_char('.');
	write_dec(p[3]);
}


void write_ip_and_port ( const byte *ip, const word port )
{
	write_ip(ip);
	write_char(':');
	write_dec(port);
}


void press_a_key ( void )
{
	write_string("Press any key\r");
	while (arch_getkey())
		;
	while (!arch_getkey())
		;
}


void wait ( word frames )
{
#ifdef	MEGA65
	byte old = PEEK(0xD7FA);
	frames++;	// be sure at least one full frame is waited
	while (frames) {
		for (;;) {
			byte new = PEEK(0xD7FA);
			if (new != old) {
				old = new;
				break;
			}
		}
		frames--;
	}
#else
	SDL_Delay(20 * frames);
#endif
}


void clear_input ( void )
{
	inp_addr = 24 * 80;
	memset(screen + inp_addr, 32, 80);
	screen[inp_addr] = 0;
	colour[inp_addr] = CURSOR_COLOUR;
}


void add_input ( const byte c )
{
	if (c == 0x14 && inp_addr != 24 * 80) {
		screen[inp_addr] = ' ';
		inp_addr--;
		screen[inp_addr] = 0;
		colour[inp_addr] = CURSOR_COLOUR;
	}
	if (c >= 32 && c < 127 && inp_addr != 24 * 80 + 79) {
		screen[inp_addr] = c;
		colour[inp_addr] = INPUT_COLOUR;
		inp_addr++;
		screen[inp_addr] = 0;
		colour[inp_addr] = CURSOR_COLOUR;
	}
}
