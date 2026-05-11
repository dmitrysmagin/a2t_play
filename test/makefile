.PHONY: clean

all: at2_dump

at2_dump: at2_dump.pas
	fpc -O2 -OpPENTIUM2 -Ccpascal -Mtp -Rintel -Fu../at2 at2_dump.pas -oat2_dump

clean:
	rm -f *.o *.ppu *.s at2_dump at2_dump.exe
