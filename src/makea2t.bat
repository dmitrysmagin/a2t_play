@echo off
echo Building A2T_Play
echo Make sure you have FPC in your path
echo.
echo Press CTRL-C to abort, any key to compile
pause
ppc386 a2t_play.pas -B -Sd -XXs -OGr3 
pause
