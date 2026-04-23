#!/usr/bin/env python3

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

import sys

def doit(megafile, unixfile, inputfile):
    with open(inputfile, "rb") as f:
        font = bytearray(f.read())
    if len(font) != 4096:
        raise RuntimeError("Bad input font size")
    for a in range(16): # A hack, character zero will be a block (cursor)
        font[a] = 0xFF
    ufont = """/* Taken From the SeaBIOS open source project, HOWEVER SeaBIOS states the followings
 * for this file (unmodified comment following):
 * ----------------------------------------------------------------------------------
 * These fonts come from ftp://ftp.simtel.net/pub/simtelnet/msdos/screen/fntcol16.zip
 * The package is (c) by Joseph Gil
 * The individual fonts are public domain
 */

static const unsigned char font[] = {
"""
    for a in range(len(font)):
        if a % 16 == 0:
            ufont += " "
        ufont += "0x{:02x}".format(font[a])
        if a != len(font) - 1:
           ufont += ","
        if a % 16 == 15:
            ufont += "\n"
        else:
            ufont += " "
    ufont += "};\n"
    mfont = bytearray(len(font))
    for a in range(16):
        for c in range(256):
            mfont[c * 8 + (a >> 1) + (2048 if a & 1 else 0)] = font[c * 16 + a]
    with open(unixfile, "w") as f:
        f.write(ufont)
    with open(megafile, "wb") as f:
        f.write(mfont)

if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Bad usage")
        sys.exit(1)
    doit(megafile = sys.argv[1], unixfile = sys.argv[2], inputfile = sys.argv[3])
    sys.exit(0)
