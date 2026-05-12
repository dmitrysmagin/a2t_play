
# Testing trivia

# Tools

* C Tool - a2m_dump
how to build: make test
how to run: a2m_dump <a2m_file>
outputs: <a2m_file.reg>
supports songend

* Pascal Tool - adt2play_sdl/adt2_dump - the reference
how to build: go into adt2play_sdl and use Makefile and FreePascal
how to run: adt2_dump <a2m_file>
outputs: <a2m_file.reg>
doesn support songend, loops forever

Both tools output 2 banks of OPL3 registers per irq frame.

# Testing
* run run_one_test.sh and compare the output of two tools with 'diff' for 100000 frames.
* compare diffs, but ignore reg dumps that occured during INIT and post songend diff

If there are differences - investigate C tool and try to fix taking adt2play_sdl as reference
until diff is zero or, at least, reduced.

adt2play_sdl should be the source of truth, C tool should match Pascal one ultimately
