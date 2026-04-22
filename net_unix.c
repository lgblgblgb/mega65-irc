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
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>


static int sock = -1;
static struct sockaddr_in servaddr;
static enum net_fsm_enum { NET_IDLE, NET_CONNECT_DO, NET_CONNECT_WAIT, NET_CONNECTED, NET_ERROR } fsm = NET_IDLE;
static int wr_count, rd_count;
static byte rd_buffer[256];
static byte wr_buffer[256];

bool net_event = false;


static void close_previous ( const char *msg )
{
	if (sock < 0)
		return;
	printf("NET: closing previous open socket on <%s>\n", msg);
	close(sock);
	sock = -1;
}


void net_init ( void )
{
	close_previous("init");
	puts("NET: init, do noting ;)");
}


static void fsm_ok ( const char *msg, const enum net_fsm_enum next )
{
	net_event = true;
	if (msg)
		printf("NET: OK: %s\n", msg);
	fsm = next;
}


static void fsm_net_error ( const char *msg, const int error )
{
	net_event = true;
	//err_val = errno;
	fsm = NET_ERROR;
	if (msg)
		printf("NET: ERROR: %s: %s\n", msg, strerror(error));
	close_previous("net error");
}


void net_connect_init ( const byte *ip, const word port )
{
	printf("NET: CONNECT: connecting to %d.%d.%d.%d:%d\n", ip[0], ip[1], ip[2], ip[3], port);
	fsm = NET_IDLE;
	close_previous("connect");
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		fsm_net_error("cannot create socket", errno);
		return;
	}
	const int flags = fcntl(sock, F_GETFL);
	if (flags < 0) {
		fsm_net_error("cannot get socket flags", errno);
		return;
	}
	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
		fsm_net_error("cannot set socket flags", errno);
		return;
	}
	memset(&servaddr, 0, sizeof(struct sockaddr_in));
	servaddr.sin_addr.s_addr = (ip[3] << 24) + (ip[2] << 16) + (ip[1] << 8) + ip[0];
	servaddr.sin_port = htons(port);
	servaddr.sin_family = AF_INET;
	fsm_ok("FSM moves to NET_CONNECT_DO state", NET_CONNECT_DO);
}



static void fsm_connect_do ( void )
{
	if (connect(sock, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in))) {
		if (errno == EINTR)
			return;
		if (errno != EINPROGRESS) {
			fsm_net_error("connect() error", errno);
			return;
		}
		fsm_ok("FSM moves to NET_CONNECT_WAIT state (connect=EINPROGRESS)", NET_CONNECT_WAIT);
	} else {
		fsm_ok("FSM moves to NET_CONNECTED state (wow, connect worked instantly)", NET_CONNECTED);
	}
}


static inline bool is_recoverable_error ( const int error )
{
	return error == EAGAIN || error == EWOULDBLOCK || error == EINTR;
}


static void fsm_connect_wait ( void )
{
	struct pollfd pfd;
	pfd.fd = sock;
	pfd.events = POLLOUT;
	const int ret = poll(&pfd, 1, 0);
	if (ret < 0) {
		if (!is_recoverable_error(errno))
			fsm_net_error("poll() for connect() error", errno);
		return;
	}
	if (ret > 0 && (pfd.revents & (POLLOUT | POLLERR | POLLHUP))) {
		int error = 0;
		socklen_t len = sizeof(error);
		if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
			fsm_net_error("getsockopt() failed", errno);
		} else if (error) {
			fsm_net_error("connection failed as reported by getsockopt", error);
		} else {
			fsm_ok("FSM moves to NET_CONNECTED state", NET_CONNECTED);
			rd_count = 0;
			wr_count = 0;
		}
	}
}


static void fsm_connected ( void )
{
	if (wr_count) {
		const int ret = write(sock, wr_buffer, wr_count);
		if (ret < 0) {
			if (!is_recoverable_error(errno)) {
				fsm_net_error("write() error", errno);
				return;
			}
		} else if (ret == 0) {
			fsm_net_error("write() got back zero!", 0);
			return;
		} else {
			printf("NET: successfully wrote %d bytes of %d buffer\n", ret, wr_count);
			if (ret != wr_count)
				memmove(wr_buffer, wr_buffer + ret, wr_count - ret);
			wr_count -= ret;
			net_event = true;
		}
	}
	const int rd_need = sizeof(rd_buffer) - rd_count;
	if (rd_need > 0) {
		const int ret = read(sock, rd_buffer + rd_count, rd_need);
		if (ret < 0) {
			if (!is_recoverable_error(errno)) {
				fsm_net_error("read() error", errno);
				return;
			}
		} else if (ret == 0) {
			fsm_net_error("read() got back zero!", 0);
			return;
		} else {
			printf("NET: successfully read %d bytes of %d\n", ret, rd_need);
			rd_count += ret;
			net_event = true;
		}
	}
}


int net_pump ( void )
{
	switch (fsm) {
		case NET_CONNECT_DO:
			fsm_connect_do();
			break;
		case NET_CONNECT_WAIT:
			fsm_connect_wait();
			break;
		case NET_CONNECTED:
			fsm_connected();
			break;
		case NET_IDLE:
			break;
		case NET_ERROR:
			break;
	}
	if (fsm == NET_ERROR)
		return -1;
	if (fsm == NET_CONNECTED) {
		return
			(rd_count ? 1 : 0) +
			(wr_count < sizeof(wr_buffer) ? 2 : 0)
		;
	}
	return 0;
}


int net_fetch_byte ( void )
{
	if (fsm != NET_CONNECTED)
		return -1;
	if (rd_count == 0)
		return -1;
	const int ret = rd_buffer[0];
	rd_count--;
	if (rd_count)
		memmove(rd_buffer, rd_buffer + 1, rd_count);
	return ret;
}


int net_get_write_buffer_size ( void )
{
	if (fsm != NET_CONNECTED)
		return 0;
	return sizeof(wr_buffer) - wr_count;
}


int net_write ( const byte *buffer, const byte size )
{
	if (fsm != NET_CONNECTED)
		return -1;
	net_event = true;
	if (size > sizeof(wr_buffer) - wr_count)
		return -1;
	memcpy(wr_buffer + wr_count, buffer, size);
	wr_count += size;
	return sizeof(wr_buffer) - wr_count;
}


int net_write_byte ( const byte b )
{
	if (fsm != NET_CONNECTED)
		return -1;
	net_event = true;
	if (wr_count < sizeof(wr_buffer))
		wr_buffer[wr_count++] = b;
	return sizeof(wr_buffer) - wr_count;
}
