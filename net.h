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

#ifndef NET_H_INCLUDED
#define NET_H_INCLUDED

#include "arch.h"

extern void net_init ( void );
extern void net_connect_init ( const byte *ip, const word port );
extern int  net_pump ( void );
extern int  net_fetch_byte ( void );
extern int  net_write ( const byte *buffer, const int size );
extern void net_do_dhcp ( void );


#endif

