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
#include "net.h"
#include "common.h"

// From arch_mega65_lowlevel.asm

extern void eth_call ( void );
extern byte eth_call_id, eth_call_f, eth_call_a, eth_call_x, eth_call_y, eth_call_z;
extern void copy_to_megaip_ ( void );
extern byte copy_to_megaip_length;
extern word copy_to_megaip_source;
extern void copy_ip_info ( void );
extern byte *copy_ip_info_addr;

// from mega-ip's jump table, only the the lowest byte!
// must be set eth_call_id with any of these values before calling eth_call()

#define ETH_INIT		0x00
#define ETH_SET_GATEWAY_IP	0x03
#define ETH_SET_LOCAL_IP	0x06
#define ETH_SET_LOCAL_PORT	0x09
#define ETH_SET_REMOTE_IP	0x0C
#define ETH_SET_REMOTE_PORT	0x0F
#define ETH_SET_SUBNET_MASK	0x12
#define ETH_SET_CHAR_XLATE	0x15
#define ETH_TCP_SEND		0x18
#define ETH_TCP_SEND_STRING	0x1B
#define ETH_RBUF_GET		0x1E
#define ETH_TCP_DISCONNECT	0x21
#define ETH_STATUS_POLL		0x24
#define ETH_TCP_CONNECT_START	0x27
#define ETH_CONNECT_POLL	0x2A
#define ETH_CONNECT_CANCEL	0x2D
#define ETH_DNS_LOOKUP		0x30
#define ETH_GET_DNS_RESULT_IP	0x33
#define ETH_GET_DNS_STATE	0x36
#define ETH_TCP_LISTEN_START	0x39
#define ETH_TCP_LISTEN_STOP	0x3C
#define ETH_ACCEPT_POLL		0x3F
#define ETH_DHCP_START		0x42
#define ETH_DHCP_POLL		0x45
#define ETH_GET_DHCP_STATE	0x48
#define ETH_SET_PRIMARY_DNS	0x4B




void net_init ( void )
{
	eth_call_id = ETH_INIT;
	eth_call();
}

#if 0
void net_set_gw_ip ( const byte *ip )
{
	eth_call_a = ip[0];
	eth_call_x = ip[1];
	eth_call_y = ip[2];
	eth_call_z = ip[3];
	eth_call_id = ETH_SET_GATEWAY_IP;
	eth_call();
}

void net_set_local_ip ( const byte *ip )
{
	eth_call_a = ip[0];
	eth_call_x = ip[1];
	eth_call_y = ip[2];
	eth_call_z = ip[3];
	eth_call_id = ETH_SET_LOCAL_IP;
	eth_call();
}

void net_set_local_port ( const word port )
{
	eth_call_a = port >> 8;
	eth_call_x = port & 0xFF;
	eth_call_id = ETH_SET_LOCAL_PORT;
	eth_call();
}

void net_set_remote_ip ( const byte *ip )
{
	eth_call_a = ip[0];
	eth_call_x = ip[1];
	eth_call_y = ip[2];
	eth_call_z = ip[3];
	eth_call_id = ETH_SET_REMOTE_IP;
	eth_call();
}

void net_set_remote_port ( const word port )
{
	eth_call_a = port >> 8;
	eth_call_x = port & 0xFF;
	eth_call_id = ETH_SET_REMOTE_PORT;
	eth_call();
}

void net_set_subnet_mask ( const byte *mask )
{
	eth_call_a = mask[0];
	eth_call_x = mask[1];
	eth_call_y = mask[2];
	eth_call_z = mask[3];
	eth_call_id = ETH_SET_SUBNET_MASK;
	eth_call();
}

void net_set_char_xlate ( const byte mode )
{
	eth_call_a = mode;
	eth_call_id = ETH_SET_CHAR_XLATE;
	eth_call();
}

void net_send ( const byte *ptr, const byte len )
{
	// Copy data
	copy_to_megaip_length = len;
	copy_to_megaip_source = (word)ptr;
	copy_to_megaip_();
	// Send
	eth_call_id = ETH_TCP_SEND;
	eth_call();
}
#endif


static byte ip_info[24];


void net_do_dhcp ( void )
{
	byte old_status;
again:
	old_status = 0;
	write_string("DHCP: ");
	eth_call_id = ETH_DHCP_START;
	eth_call();
	for (;;) {
		eth_call_id = ETH_STATUS_POLL;
		eth_call();
		eth_call_id = ETH_DHCP_POLL;
		eth_call();
		if (eth_call_a != old_status) {
			if (eth_call_a == 127) {
				write_string("Timeout\n");
				goto again;
			}
			write_char(eth_call_a + '0');
			if (eth_call_a == 4)
				break;
			old_status = eth_call_a;
		}
		screen_mem[24*80 - 2] = screen_mem[24*80 - 1];
	}
	write_char(' ');
	copy_ip_info_addr = ip_info;
	copy_ip_info();
	write_ip(ip_info);
	write_char('/');
	write_ip(ip_info + 16);
	write_char(' ');
	write_ip(ip_info + 12);
	write_char(' ');
	write_ip(ip_info + 20);
	write_char('\n');
}


void net_connect_init ( const byte *ip, const word port )
{
	// Set remote IP
	eth_call_a = ip[0];
	eth_call_x = ip[1];
	eth_call_y = ip[2];
	eth_call_z = ip[3];
	eth_call_id = ETH_SET_REMOTE_IP;
	eth_call();
	// Set remote port
	eth_call_a = port >> 8;
	eth_call_x = port & 0xFF;
	eth_call_id = ETH_SET_REMOTE_PORT;
	eth_call();
	// Set local port
	eth_call_a = 0xC0;
	eth_call_x = 0x00;
	eth_call_id = ETH_SET_LOCAL_PORT;
	eth_call();
	// Start connection
	eth_call_id = ETH_TCP_CONNECT_START;
	eth_call();
	for (;;) {
		// Polling
		eth_call_id = ETH_CONNECT_POLL;
		eth_call();
		if (eth_call_a & 1) {
			write_string("Connected.\n");
			return;
		}
		if (eth_call_a & 2) {
			write_string("Failed\n");
			for(;;) ;
		}
		wait(1);	// without this, it does not work??
		screen_mem[24*80 - 2] = screen_mem[24*80 - 1];
	}
}
