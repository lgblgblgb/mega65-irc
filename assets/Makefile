PETCAT		= petcat
PYTHON		= python3

VGA_FONT	= font-original-vga.bin
MEGA_FONT_BIN	= font-mega65.bin
UNIX_FONT_H	= font-unix.h
PRGS_NEEDED	= setup.prg

all: $(PRGS_NEEDED) $(MEGA_FONT_BIN) $(UNIX_FONT_H)

%.prg: %.bas
	$(PETCAT) -w65 -o $@ -- $<

$(MEGA_FONT_BIN) $(UNIX_FONT_H): $(VGA_FONT)
	$(PYTHON) ./fontconv.py $(MEGA_FONT_BIN) $(UNIX_FONT_H) $(VGA_FONT)

clean:
	rm -f $(PRGS_NEEDED) $(MEGA_FONT_BIN) $(UNIX_FONT_H)

.PHONY: all clean
