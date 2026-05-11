.PHONY: all clean

all: adt2play adt2_dump

adt2play: adt2play.pas a2player.pas a2scrio.pas a2fileio.pas a2depack.pas a2data.pas opl3emu.pas parserio.pas stringio.pas txtscrio.pas adt2play_sdl_audio.pas
	fpc -O2 -OpPENTIUM2 -Ccpascal -Mtp -Rintel -Fusdl adt2play.pas -oadt2play

adt2_dump: adt2_dump.pas a2player.pas a2fileio.pas a2depack.pas a2data.pas opl3emu.pas parserio.pas stringio.pas
	fpc -O2 -Ccpascal -Mtp -Rintel adt2_dump.pas -oadt2_dump

clean:
	rm -f *.o *.s *.res *.ppu *.map *.fpd *.sym *.or *.exe adt2play adt2_dump
	rm -f sdl/*.o sdl/*.ppu
