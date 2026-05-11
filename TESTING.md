
Testing trivia

* Tool 1 - a2m_dump
how to build: make test
how to run: a2m_dump <a2m_file>
outputs: <a2m_file.reg>

* Tool 2 - adt2play_sdl/adt2_dump
how to build: go into adt2play_sdl and use Makefile and FreePascal
how to run: adt2_dump <a2m_file>
outputs: <a2m_file.reg>

Both output file in a format: "%03x %02x\n"


* Testing: compare the output of two tools with 'diff'

If there are differnences - investigate src/ and try to fix taking adt2play_sdl as reference.

adt2play_sdl should be the source of truth
