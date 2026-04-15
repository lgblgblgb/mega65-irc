#!/usr/bin/env bash

CC="$1"
SRC="$2"
OBJ="$3"
OPTS="$4"

echo "$0: --- Creating build info ---"

rm -f "$SRC" || exit 1
cat > "$SRC" <<EOF
const unsigned char build_info[] =
	"(C)2026 LGB (G\xa0" "bor L\x82n\xa0" "rt)\r\n"
	"Compiled by `whoami`@`uname -n` at `date`\r\n"
	"`git config --get remote.origin.url` "
	"`git branch --show-current` "
	"`git log | awk '{ print $2 ; exit }'`\r\n"
;
EOF

rm -f "`basename $OBJ`" "$OBJ"
echo "$0: $CC $OPTS -o $OBJ -c $SRC"
"$CC" $OPTS -o "$OBJ" -c "$SRC" || exit 1
if [ -f "`basename $OBJ`" ]; then
	mv "`basename $OBJ`" "$OBJ"
fi
