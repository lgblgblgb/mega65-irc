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
#include "net.h"
#include <string.h>


static byte current_ip[4] = {192,168,0,153};
static word current_port = 6667;
static char current_nick[17] = "LGBmega";
static const char space_str[] = " ";
static const char crlf[] = "\r\n";
static const char missing_parameters[] = "Missing cmd par(s)\n";
static const char already_connected[] = "Already connected\n";
static enum { FSM_OFFLINE, FSM_CONNECT, FSM_CONNECTED, FSM_ONLINE } fsm = FSM_OFFLINE;
static char rx_buffer[256];
static char tx_buffer[256];
static unsigned int rx_fill = 0;


static void update_status ( void )
{
	set_status_line_writing(true);
	write_string(" MEGA65IRC ");
	if (fsm == FSM_OFFLINE) {
		write_string("offline");
	} else {
		write_ip_and_port(current_ip, current_port);
		write_string(fsm == FSM_CONNECT ? " connecting [" : " [");
		write_string(current_nick);
		write_char(']');
	}
	set_status_line_writing(false);
}


static void do_connect ( void )
{
	fsm = FSM_CONNECT;
	//write_string("Connecting to ");
	//write_ip_and_port(current_ip, current_port);
	//write_char('\n');
	update_status();
	net_connect_init(current_ip, current_port);
}


static void show_build_info ( void )
{
	text_colour = INFO_COLOUR;
	write_string_utf8(build_info);
	text_colour = TEXT_COLOUR;
}


static void cmd_server ( void )
{
	char *r1, *r2, *r3;
	if (fsm != FSM_OFFLINE) {
		write_error(already_connected);
		return;
	}
	r1 = strtok(NULL, space_str);
	r2 = strtok(NULL, space_str);
	r3 = strtok(NULL, space_str);
	if (r1 && r2) {
		static byte ip_addr[4];
		word i, d;
		for (i = 0; i < 4; i++) {
			const char *ip = strtok(r1, ".");
			if (!ip)
				goto bad;
			r1 = NULL;	// for strtok() in non-first cases
			d = str2dec(ip);
			if ((i == 0 && d == 0) || d > 0xFF)
				goto bad;
			ip_addr[i] = (byte)d;
		}
		d = str2dec(r2);
		if (d == 0 || d == 0xFFFF)
			goto bad;
		if (r3) {
			strncpy(current_nick, r3, sizeof current_nick);
			current_nick[sizeof(current_nick) - 1] = '\0';
		}
		// Seems to be good IP and port!
		memcpy(current_ip, ip_addr, 4);
		current_port = d;
		do_connect();
		return;
	}
	write_error(missing_parameters);
	return;
bad:
	write_error("Bad IP or port\n");
}


static void cmd_nick ( void )
{
	char *nick = strtok(NULL, space_str);
	if (!nick) {
		write_error(missing_parameters);
		return;
	}
	strncpy(current_nick, nick, sizeof current_nick);
	current_nick[sizeof(current_nick) - 1] = '\0';
	update_status();
	// TODO: if connected, send to the server too!
	// TODO: or only send to the server if connected, and change nick on server's response?
}


static void cmd_join ( void )
{
	char *channel = strtok(NULL, space_str);
	if (channel) {
		byte l = strappend(tx_buffer, 0, "JOIN ");
		l = strappend(tx_buffer, l, channel);
		l = strappend(tx_buffer, l, crlf);
#ifndef		MEGA65
		net_write((byte*)tx_buffer, l);
#endif
	} else
		write_error(missing_parameters);
}


static void cmd_motd ( void )
{
	write_error("TODO\n");
}


static void cmd_help ( void );


static const struct command_st {
	const char *cmd;
	void (*callback)(void);
} commands[] = {
	{ "help",	cmd_help	},
	{ "join",	cmd_join	},
	{ "motd",	cmd_motd	},
	{ "nick",	cmd_nick	},
	{ "quit",	arch_exit	},
	{ "server",	cmd_server	},
	{ "sys",	show_build_info	},
	{ NULL,		NULL }
};


static void cmd_help ( void )
{
	const struct command_st *c;
	text_colour = INFO_COLOUR;
	write_string("Slash-cmds:");
	for (c = commands; c->cmd; c++) {
		write_char(' ');
		write_string(c->cmd);
	}
	write_char('\n');
	text_colour = TEXT_COLOUR;
}


static bool try_to_interpret_as_slash_command ( char *p )
{
	while (*p && *p <= 32)
		p++;
	if (*p == '/') {
		char *r = strtok(p + 1, space_str);
		if (r) {
			const struct command_st *c;
			for (c = commands; c->cmd; c++) {
				if (!strcmp(c->cmd, r)) {
					(c->callback)();
					return true;
				}
			}
			write_error("Unknown cmd: /");
			write_error(r);
			write_char('\n');
		}
		return true;
	}
	return false;	// was not a slash-command
}


static void do_kbd_stuff ( void )
{
	const byte b = arch_getkey();
	if (!b)			// Do nothing, if no key is pressed
		return;
	if (b == 13) {		// Return
		if (!try_to_interpret_as_slash_command(input_string)) {
			// If not a slash command, interpret it as a general channel message
			const byte l = strlen(input_string);
			write_string(input_string);
			write_char('\n');
#ifndef			MEGA65
			input_string[l] = '\r';
			input_string[l + 1] = '\n';
			input_string[l + 2] = 0;
			net_write((byte*)input_string, l + 2);
#endif
		}
		clear_input();
		return;
	}
	if (b == 27) {		// Escape
		clear_input();
		return;
	}
	if (b == 0xF1) {	// F1
		if (fsm == FSM_OFFLINE)
			do_connect();
		else
			write_error(already_connected);
		return;
	}
	add_input(b);
}


static void do_net_stuff ( void )
{
	static int init_sent = false;
	const int n = net_pump();
	switch (fsm) {
		case FSM_OFFLINE:
			break;
		case FSM_CONNECT:
		case FSM_CONNECTED:
		case FSM_ONLINE:
			break;
	}
	if (n != -1 && (n & 1)) {
		if (!init_sent) {
			byte l = strappend(tx_buffer, 0, "USER ");
			l = strappend(tx_buffer, l, current_nick);
			l = strappend(tx_buffer, l, " +iw ");
			l = strappend(tx_buffer, l, current_nick);
			l = strappend(tx_buffer, l, " :MEGA65 ruleZ\r\nNICK ");
			l = strappend(tx_buffer, l, current_nick);
			l = strappend(tx_buffer, l, crlf);
			write_string("Connected.\n");
			fsm = FSM_CONNECTED;
			update_status();
			// static const char init_cmds[] = "USER lgb +iw lgb :MEGA65 ruleZ\r\nNICK lgbx\r\n";
			if (net_write((byte*)tx_buffer, l) != -1) {
				write_colour_string(tx_buffer, CURSOR_COLOUR);
				init_sent = true;
			}
		}
		// Emptying received data byte-by-byte
		for (;;) {
			const int b = net_fetch_byte();
			if (b < 0)
				break;
			if (b == 13 || b == 10) {
				if (rx_fill) {
					rx_buffer[rx_fill] = 0;
#ifndef					MEGA65
					printf("IRC: new message received %d bytes: %s\n", rx_fill, rx_buffer);
#endif
					if (!strncmp("PING ", rx_buffer, 5)) {
#ifndef						MEGA65
						puts("IRC: ping message, let's pong it!");
#endif
						rx_buffer[1] = 'O';		// change "PING" to PONG with 'I'->'O' change
						rx_buffer[rx_fill++] = '\r';
						rx_buffer[rx_fill++] = '\n';
						net_write((byte*)rx_buffer, rx_fill);	// then transmit as answer ...
					} else {
						write_string_utf8((char*)rx_buffer);
						write_char('\n');
					}
					rx_fill = 0;
				}
			} else if (rx_fill < sizeof(rx_buffer) - 3) {
				if (b)
					rx_buffer[rx_fill++] = b;
			} else {
#ifndef				MEGA65
				printf("IRC: TOO LONG MESSAGE - TRUNCATING\n");
#endif
			}
		}
	}
}


/* ---- instead of "main", we have the control at "main_entry" after startup ---- */


void main_entry ( void )
{
#ifndef	MEGA65
	arch_set_background_colour(BACKGROUND_COLOUR);
	arch_set_status_bg_emulation(23, STATUS_BG_COLOUR);
#else
	POKE(0xD020, BORDER_COLOUR);
	POKE(0xD021, BACKGROUND_COLOUR);
	arch_set_status_bg(23, STATUS_BG_COLOUR);
#endif
	memset(screen_mem, 32, 2000);	// clear screen
	update_status();
	show_build_info();
#ifdef	MEGA65
	memcpy(screen_mem + 23*80 + 38, "WAIT", 4);
	write_string("Resetting ethernet controller\n");
#endif
	net_init();
#ifdef	MEGA65
	net_do_dhcp();
	update_status();		// will clear the "WAIT" message
	if (PEEK(0xF700) && PEEKW(0xF704) && PEEK(0xF706)) {
		memcpy(current_ip, (void*)0xF700, 4);
		current_port = *(word*)0xF704;
		strncpy(current_nick, (char*)0xF706, sizeof current_nick);
		current_nick[sizeof(current_nick) - 1] = '\0';
		write_string("Found default config: ");
		write_ip_and_port(current_ip, current_port);
		write_char(' ');
		write_string(current_nick);
		write_colour_string("\nPress F1 to connect with that config. Or:\n", INSTRUCTION_COLOUR);
	} else {
		write_colour_string("Hint: run prg 'ircsetup' for default parameters\n", INSTRUCTION_COLOUR);
	}
#endif
	write_colour_string(
		"Connect a server (no DNS yet): /server IP port YourNick\n"
		"(list all slash cmds: /help)\n",
		INSTRUCTION_COLOUR
	);
	clear_input();	// also initializes input stuff and shows "cursor"
	consume_keys();	// make sure keyboard is empty
	// Main client loop
	for (;;) {
		do_kbd_stuff();
		do_net_stuff();
		arch_refresh();
#ifndef		MEGA65
		SDL_Delay(1);
#endif
	}
}
