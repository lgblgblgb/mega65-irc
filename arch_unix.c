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

#include <stdio.h>
#include "arch.h"
#include <SDL.h>

byte screen[80 * 25];
byte colour[80 * 25];

#define	X_SIZE	(80 * 8)
#define	Y_SIZE	(25 * 16)
#define	PIXELFORMAT SDL_PIXELFORMAT_ARGB8888
#define WINDOW_TITLE "iRC"

#define UPDATE_MS 16
#define KQ_SIZE 4

static int kq_size = 0;
static byte kq[KQ_SIZE];

static SDL_Window *sdl_win = NULL;
static SDL_Renderer *sdl_ren = NULL;
static SDL_Texture *sdl_tex = NULL;
static SDL_PixelFormat *sdl_fmt = NULL;
static Uint32 sdl_pix[X_SIZE * Y_SIZE];
static bool sdl_init = false;
static Uint32 sdl_colours[16];
static Uint32 sdl_last_update = 0;

#include "font.h"


static void shutup ( void )
{
	if (sdl_fmt)	SDL_FreeFormat(sdl_fmt);
	if (sdl_tex)	SDL_DestroyTexture(sdl_tex);
	if (sdl_ren)	SDL_DestroyRenderer(sdl_ren);
	if (sdl_win)	SDL_DestroyWindow(sdl_win);
	if (sdl_init)	SDL_Quit();
	exit(-1);
}


static void add_key ( const byte c )
{
	if (!c) {
		printf("Trying to insert null byte into the keyboard queue!\n");
		return;
	}
	if (kq_size >= KQ_SIZE) {
		printf("Keyboard is full!\n");
		return;
	}
	kq[kq_size++] = c;
}


byte arch_getkey ( void )
{
	if (kq_size) {
		const byte ret = kq[0];
		kq_size--;
		if (kq_size)
			memmove(kq, kq + 1, kq_size);
		return ret;
	}
	return 0;
}


void arch_refresh ( void )
{
	const Uint32 t = SDL_GetTicks();
	if (t - sdl_last_update < UPDATE_MS)
		return;
	sdl_last_update = t;
	// *** Update screen ***
	Uint32 *p = sdl_pix;
	for (int y = 0; y < 25 * 16; y++) {
		for (int x = 0, i = (y >> 4) * 80; x < 80; x++, i++) {
			const Uint32 fg = sdl_colours[colour[i] & 15];
			const Uint32 bg = sdl_colours[colour[i] >> 4];
			const byte data = font[(screen[i] << 4) + (y & 15)];
			for (int k = 0x80; k; k >>= 1)
				*p++ = data & k ? fg : bg;
		}
	}
	SDL_UpdateTexture(sdl_tex, NULL, sdl_pix, X_SIZE * 4);
	SDL_RenderCopy(sdl_ren, sdl_tex, NULL, NULL);
	SDL_RenderPresent(sdl_ren);
	// *** Handle events ***
	SDL_PumpEvents();
	SDL_Event ev;
	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
			case SDL_QUIT:
				shutup();
				break;
			case SDL_KEYDOWN:
				if (ev.key.keysym.mod == 0 || !(ev.key.keysym.mod & ~(KMOD_LSHIFT | KMOD_RSHIFT))) {
					int k = -1;
					switch (ev.key.keysym.scancode) {
						case SDL_SCANCODE_F9:		shutup();	break;
						case SDL_SCANCODE_DELETE:
						case SDL_SCANCODE_BACKSPACE:	k = 0x1494;	break;
						case SDL_SCANCODE_RETURN:
						case SDL_SCANCODE_KP_ENTER:	k = 0x0D8D;	break;
						case SDL_SCANCODE_INSERT:	k = 0x9494;	break;
						case SDL_SCANCODE_HOME:		k = 0x1393;	break;
						case SDL_SCANCODE_END:		k = 0x0383;	break;
						case SDL_SCANCODE_ESCAPE:	k = 0x1B1B;	break;
						case SDL_SCANCODE_TAB:		k = 0x091A;	break;
						case SDL_SCANCODE_RIGHT:	k = 0x1D1D;	break;
						case SDL_SCANCODE_LEFT:		k = 0x9D9D;	break;
						case SDL_SCANCODE_UP:		k = 0x9191;	break;
						case SDL_SCANCODE_DOWN:		k = 0x1111;	break;
						default:					break;
					}
					if (k != -1)
						add_key((ev.key.keysym.mod & (KMOD_LSHIFT | KMOD_RSHIFT)) ? k & 0xFF : k >> 8);
				}
				break;
			case SDL_TEXTINPUT:
				add_key(ev.text.text[0]);
				break;
		}
	}
}


static const Uint8 cpal[] = {
	0x00,0xff,0xba,0x66,0xbb,0x55,0xd1,0xae,0x9b,0x87,0xdd,0xb5,0xb8,0x0b,0xaa,0x8b,	// -- red
	0x00,0xff,0x13,0xad,0xf3,0xec,0xe0,0x5f,0x47,0x37,0x39,0xb5,0xb8,0x4f,0xd9,0x8b,	// -- green
	0x00,0xff,0x62,0xff,0x8b,0x85,0x79,0xc7,0x81,0x00,0x78,0xb5,0xb8,0xca,0xfe,0x8b		// -- blue
};
#define NEX(a) ((((a))>>4)+(((a) & 15) << 4))

extern void main_entry ( void );

int main ( int argc, char **argv )
{
	if (SDL_Init(SDL_INIT_EVERYTHING & ~(SDL_INIT_TIMER | SDL_INIT_HAPTIC))) {
		fprintf(stderr, "Cannot initialize SDL2 library: %s\n", SDL_GetError());
		shutup();
		return -1;
	}
	sdl_init = true;
	sdl_win = SDL_CreateWindow(WINDOW_TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, X_SIZE * 2, Y_SIZE * 2, SDL_WINDOW_SHOWN);
	if (!sdl_win) {
		fprintf(stderr, "Cannot create window: %s\n", SDL_GetError());
		shutup();
		return -1;
	}
	sdl_ren = SDL_CreateRenderer(sdl_win, -1, SDL_RENDERER_ACCELERATED);
	if (!sdl_ren) {
		fprintf(stderr, "Cannot create renderer: %s\n", SDL_GetError());
		shutup();
		return -1;
	}
	SDL_RenderSetLogicalSize(sdl_ren, X_SIZE, Y_SIZE);
	sdl_tex = SDL_CreateTexture(sdl_ren, PIXELFORMAT, SDL_TEXTUREACCESS_STREAMING, X_SIZE, Y_SIZE);
	if (!sdl_tex) {
		fprintf(stderr, "Cannot create texture: %s\n", SDL_GetError());
		shutup();
		return -1;
	}
	sdl_fmt = SDL_AllocFormat(PIXELFORMAT);
	if (!sdl_fmt) {
		fprintf(stderr, "Cannot allocate format: %s\n", SDL_GetError());
		shutup();
		return -1;
	}
	//sdl_colours[i] = SDL_MapRGBA(sdl_fmt, inipal[i * 3], inipal[i * 3 + 1], inipal[i * 3 + 2], 0xFF);
	for (int i = 0; i < 16; i++)
		sdl_colours[i] = SDL_MapRGBA(sdl_fmt, NEX(cpal[i] & 0xEF), NEX(cpal[i + 0x10]), NEX(cpal[i + 0x20]), 0xFF);
	SDL_StartTextInput();
	main_entry();	// Call the main entry point
	shutup();
	return 0;
}
