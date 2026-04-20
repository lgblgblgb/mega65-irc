eth.bin eth.i65: *.asm
	64tass eth.asm --nostart -L eth.lst -l eth.lab -o $@
	sed 's/^/megaip_/' < eth.lab | sort > eth.i65

clean:
	rm -f eth.bin eth.i65 eth.lab eth.lst

