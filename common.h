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

#ifndef	COMMON_H_INCLUDED
#define	COMMON_H_INCLUDED

#include "arch.h"

#define BORDER_COLOUR		0
#define BACKGROUND_COLOUR	0

#define TEXT_COLOUR		1
#define CURSOR_COLOUR		5
#define INPUT_COLOUR		14
#define STATUS_BG_COLOUR	2
#define STATUS_FG_COLOUR	7
#define INFO_COLOUR		14
#define ERROR_COLOUR		2
#define INSTRUCTION_COLOUR	7

#define UTF8_OFF		-1
#define UTF8_ON			0

#define input_string ((char*)screen_mem + 24 * 80)

extern byte text_colour;
extern int  utf8;

extern void write_char ( byte c );
extern void write_string ( const char *s );
extern void write_string_utf8 ( const char *s );
extern void write_colour_string ( const char *s, const byte temp_colour );
extern void write_error ( const char *s );
extern void write_dec ( word d );
extern void write_ip ( const byte *p );
extern void write_ip_and_port ( const byte *ip, const word port );
extern void press_a_key ( void );
extern void consume_keys ( void );
extern void wait ( word frames );
extern void clear_input ( void );
extern void add_input ( const byte c );
extern void set_status_line_writing ( const bool status );
extern word str2dec ( const char *s );
extern byte strappend ( char *base, byte pos, const char *add );

extern const char build_info[];

#endif
