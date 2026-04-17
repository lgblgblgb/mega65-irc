# Copyright (C)2026 LGB (Gábor Lénárt) <lgblgblgb@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

COBJS	= irc.o common.o build_info.o
#XEMU	= xemu-xmega65
XEMU	= ~/prog_here/xemu-next/build/bin/xmega65.native
ETHLOAD	= mega65_etherload
C1541	= c1541
BINUNIX	= irc
BINMEGA	= irc.prg
D81NAME	= irc.d81
ifneq	(,$(filter $(BINMEGA),$(MAKECMDGOALS)))
CC	= cl65
LD65	= ld65
CA65	= ca65
CFLAGS	= -DMEGA65 -O -Oi -Or -Os -t none --cpu 65c02 -r --standard c99
LDFLAGS	= --config mega65.ld --mapfile irc.map -v -vm -Ln irc.lab
ARCH	= mega65
AROBJS	= arch_mega65_lowlevel.o arch_mega65.o net_mega65.o
SYSLIB	= /usr/share/cc65/lib/none.lib
else
CC	= cc
CFLAGS	= -O2 -Wall $(shell sdl2-config --cflags)
LDFLAGS	= $(shell sdl2-config --libs)
ARCH	= native
AROBJS	= arch_unix.o net_unix.o
endif

BDIR	:= build/$(ARCH)
DOOBJS	:= $(addprefix $(BDIR)/, $(COBJS) $(AROBJS))

all:
	$(MAKE) $(BINUNIX)
	$(MAKE) $(BINMEGA)

$(BDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@rm -f $(notdir $@)
	$(CC) $(CFLAGS) -c $< -o $@
	@if [ -f $(notdir $@) ]; then mv $(notdir $@) $@ ; fi

$(BINUNIX): $(DOOBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

build_info.c:
	bash ./build_info.sh > $@

$(BDIR)/arch_mega65_lowlevel.o: arch_mega65_lowlevel.asm
	$(CA65) -t none -o $@ $<

$(BINMEGA): $(DOOBJS)
	$(LD65) $(LDFLAGS) -o $@ $^ $(SYSLIB)

mega65:
	$(MAKE) $(BINMEGA)
	$(ETHLOAD) -r $(BINMEGA)

xemu:
	$(MAKE) $(BINMEGA)
	$(XEMU) -fastboot -prg $(BINMEGA)

run:
	$(MAKE) $(BINUNIX)
	./$(BINUNIX)

$(D81NAME): $(BINMEGA)
	echo "format mega65-irc,dd d81 $(D81NAME).tmp 8\nwrite $(BINMEGA) irc\ndir" | $(C1541)
	mv $(D81NAME).tmp $(D81NAME)

publish:
	$(MAKE) clean
	$(MAKE) $(BINUNIX)
	$(MAKE) $(BINMEGA)
	$(MAKE) $(D81NAME)
	cp $(BINMEGA) $(D81NAME) bin/

clean:
	rm -f $(BINUNIX) $(BINMEGA) build/*/*.o $(D81NAME) $(D81NAME).tmp irc.lab irc.map build_info.c

.PHONY: all mega65 xemu run clean publish
