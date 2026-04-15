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


static const char demo[] = "MEGA65 iRC";


static word inp_addr;


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


#define input_string ((char*)screen + 24 * 80)


#ifdef MEGA65
static void sprite_init ( void )
{
	// TODO: clean this up!!!
	memset((void*)1024, 0, 1024);
	memset((void*)1024, 0xFF, 56);
	POKE(0xD000, 24);			// sprite X coordinate
	POKE(0xD001, 43 + 24 * 8 - 1);		// Sprite Y coordinate
	POKE(0xD01D, 0);			// sprite X MSBs
	POKE(0xD027, STATUS_BG_COLOUR);		// sprite colour
	POKE(0xD015, 1);			// sprite enable
	POKE(0xD01B, 1);			// sprite prio
	POKE(0xD04D, PEEK(0xD04D) | 0x10);	// horizontal tiling
	POKE(0xD057, 1);			// sprite extended width
	POKE(0xff8, 1024/64);			// sprite pointer
	POKE(0xD076, 0);			// V400 sprites turned off
}
#endif


const byte irc_server_ip[] = {192, 168, 0, 153};
#define IRC_PORT 6667

#include "net.h"


void main_entry ( void )
{
	memset(colour, 1, 80*25);
	memset(screen, 32, 80*25);
	memcpy(screen + 23 * 80 + 1, demo, sizeof(demo) - 1);
#ifdef	MEGA65
	sprite_init();
	memset(colour + 23 * 80, STATUS_FG_COLOUR, 80);
#else
	memset(colour + 23 * 80, (STATUS_BG_COLOUR << 4) | STATUS_FG_COLOUR, 80);
#endif
	write_string("Resetting ETH ctrl\r");
	net_init();
#ifdef	MEGA65
	net_do_dhcp();
	press_a_key();
#endif
	write_string("Connecting to ");
	write_ip_and_port(irc_server_ip, IRC_PORT);
	write_char(13);
	net_connect_init(irc_server_ip, IRC_PORT);



	clear_input();

	text_colour = CURSOR_COLOUR;
	write_char(13);
	write_char(13);
	write_dec(65535U);
	write_char(13);
	write_dec(9);
	write_char(13);
	write_dec(0);
	write_char(13);
	text_colour = TEXT_COLOUR;


	for (;;) {
#ifndef		MEGA65
		static int init_sent = false;
		const int n = net_pump();
		if (n != -1 && (n & 1)) {
			if (!init_sent) {
				write_string("Conntected.\r");
				static const char init_cmds[] = "user lgb +iw lgb :lgb is here\r\nnick lgbx\r\n";
				if (net_write((byte*)init_cmds, sizeof(init_cmds) - 1) != -1) {
					text_colour = CURSOR_COLOUR;
					write_string(init_cmds);
					text_colour = TEXT_COLOUR;
					init_sent = true;
				}
			}
			for (;;) {
				static byte rx_buffer[256];
				static int rx_fill = 0;
				const int b = net_fetch_byte();
				if (b < 0)
					break;
				if (b == 13 || b == 10) {
					if (rx_fill) {
						rx_buffer[rx_fill] = 0;
						printf("IRC: new message received %d bytes: %s\n", rx_fill, rx_buffer);
						if (!strncmp("PING ", (char*)rx_buffer, 5)) {
							puts("IRC: ping message, let's pong it!");
							rx_buffer[1] = 'O';		// change "PING" to PONG with 'I'->'O' change
							rx_buffer[rx_fill++] = '\r';
							rx_buffer[rx_fill++] = '\n';
							net_write(rx_buffer, rx_fill);	// then transmit as answer ...
						} else {
							write_string((char*)rx_buffer);
							write_char(13);
						}
						rx_fill = 0;
					}
				} else if (rx_fill < sizeof(rx_buffer) - 3) {
					if (b)
						rx_buffer[rx_fill++] = b;
				} else {
					printf("IRC: TOO LONG MESSAGE - TRUNCATING\n");
				}
			}

		}
#endif

		//printf("Hi!\n");
		const byte b = arch_getkey();
		if (b == 13) {
			write_string(input_string);
			write_char(13);
			clear_input();
		} else if (b == 27) {
			clear_input();
		} else if (b) {
			//write_char(b);
			add_input(b);
		}
		arch_refresh();
#ifndef		MEGA65
		SDL_Delay(1);
#endif
	}
}
