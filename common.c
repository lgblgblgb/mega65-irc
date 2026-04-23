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



static word line_addr = 0;
static byte x_pos = 0;
byte text_colour = TEXT_COLOUR;
static word inp_addr;
static bool to_scroll = true;

static word line_addr_saved;
static byte x_pos_saved;
static byte text_colour_saved;
static byte text_colour_restore = 0x80;

int utf8 = UTF8_OFF;


void set_status_line_writing ( const bool status )
{
	if (status) {
		to_scroll = false;
		x_pos_saved = x_pos;
		x_pos = 0;
		line_addr_saved = line_addr;
		line_addr = 23 * 80;
		text_colour_saved = text_colour;
		text_colour = STATUS_FG_COLOUR;
		memset(screen_mem + 23 * 80, ' ', 80);	// clear previous status line
		memset(colour_mem + 23*80, STATUS_FG_COLOUR, 80);
	} else {
		to_scroll = true;
		x_pos = x_pos_saved;
		line_addr = line_addr_saved;
		text_colour = text_colour_saved;
	}
}


void write_char ( byte c )
{
	if (c == 13 || c == 10) {
		if (x_pos) {
			if (to_scroll)
				line_addr += 80;
			x_pos = 0;
		}
		return;
	}
	// Support only 2 byte long utf8 sequences with limited characters!
	// Not a standard-compliant utf8 parser at all ;-)
	if ((c & 0x80) && utf8 >= 0) {
		if ((c & 0xE0) == 0xC0) {
			utf8 = (c & 0x1F) << 6;
			return;
		}
		if ((c & 0xC0) == 0x80) {
			utf8 |= c & 0x3F;
#ifndef MEGA65
			printf("UTF8 code point: %d\n", utf8);
#endif
			switch (utf8) {
				// TODO: add more codepoint mapping which is in VGA font at all to map to
				// case values: unicode code point, 'c' assignments: VGA font mapping position
				case 0xE1: c = 0xA0; break;	// á
				case 0xE9: c = 0x82; break;	// é
				default:   c = '~' ; break;
			}
		} else
			c = '~';
		utf8 = 0;
	}
	if (to_scroll && line_addr >= 23*80) {
		line_addr = 22 * 80;
#ifdef		MEGA65
		mega65_scroller();
#else
		memmove(screen_mem, screen_mem + 80, 22 * 80);
		memmove(colour_mem, colour_mem + 80, 22 * 80);
		memset(screen_mem + 22 * 80, 32, 80);
#endif
	}
	screen_mem[line_addr + x_pos] = c;
	colour_mem[line_addr + x_pos] = text_colour;
	x_pos++;
	if (x_pos == 80) {
		x_pos = 0;
		if (to_scroll)
			line_addr += 80;
	}
}


void write_string ( const char *s )
{
	while (*s)
		write_char(*s++);
	utf8 = UTF8_OFF;
	if ((text_colour_restore & 0x80))
		return;
	text_colour = text_colour_restore;
	text_colour_restore = 0x80;
}


void write_string_utf8 ( const char *s )
{
	utf8 = UTF8_ON;
	write_string(s);
}


void write_error ( const char *s )
{
	write_colour_string(s, ERROR_COLOUR);
}


void write_colour_string ( const char *s, const byte temp_colour )
{
	text_colour_restore = text_colour;
	text_colour = temp_colour;
	write_string(s);
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


void consume_keys ( void )
{
	while (arch_getkey())
		;
}


void press_a_key ( void )
{
	write_string("Press any key\n");
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
	memset(screen_mem + inp_addr, 32, 80);
	screen_mem[inp_addr] = 0;
	colour_mem[inp_addr] = CURSOR_COLOUR;
}


void add_input ( const byte c )
{
	if (c == 0x14 && inp_addr != 24 * 80) {
		screen_mem[inp_addr] = ' ';
		inp_addr--;
		screen_mem[inp_addr] = 0;
		colour_mem[inp_addr] = CURSOR_COLOUR;
	}
	if (c >= 32 && c < 127 && inp_addr != 24 * 80 + 79) {
		screen_mem[inp_addr] = c;
		colour_mem[inp_addr] = INPUT_COLOUR;
		inp_addr++;
		screen_mem[inp_addr] = 0;
		colour_mem[inp_addr] = CURSOR_COLOUR;
	}
}


// Yes, stupid. However using CC65's functions costs almost 2K of code space for the proper strtol()
word str2dec ( const char *s )
{
	int r = 0;
	while (*s) {
		if (*s < '0' || *s > '9')
			return 0xFFFF;
		r = r * 10 + (*s & 0xF);
		s++;
	}
	return r;
}


// Silly function and only works <256 byte long strings
// But good enough as MEGA-IP does not support send more anyway
byte strappend ( char *base, byte pos, const char *add )
{
	while (*add)
		base[pos++] = *add++;
	base[pos] = '\0';
	return pos;
}
