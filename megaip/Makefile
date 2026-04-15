eth.bin: *.asm
	64tass eth.asm --nostart -L eth.lst -l eth.lab -o $@
	sed 's/^/megaip_/' < eth.lab > eth.exp

clean:
	rm -f eth.bin eth.exp eth.lab eth.lst

