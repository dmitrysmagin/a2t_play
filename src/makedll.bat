@echo off
echo Building YMF262.DLL
echo Make sure you have the MinGW Distribution from MAME.NET installed in c:\mingw
echo.
echo Press CTRL-C to abort, any key to compile
pause
path c:\mingw\bin
gcc -c ymf262.c -o ymf262.o -DX86_ASM -DLSB_FIRST -DINLINE="static __inline__" -Dasm=__asm__ -DCRLF=3 -DLOGERROR
gcc -shared ymf262.o -o ymf262.dll
strip ymf262.dll
pause
