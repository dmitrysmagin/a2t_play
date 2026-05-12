
# Testing trivia

# Tools

* C Tool - a2m_dump
how to build: make test
supports songend

* Pascal Tool - adt2play_sdl/adt2_dump - the reference
how to build: go into adt2play_sdl and use Makefile and FreePascal
doesn support songend, loops forever

Both tools output 2 banks of OPL3 registers per irq frame.

# Testing and Fixing
* run run_one_test.sh with module filename and MAX_FRAMES=100000
* compare diffs, but ignore reg dumps that occured during INIT and post songend diff
* isolate the code region in a2t.c specific to frames at which diff is found
* if needed, add additional dumping into a2t.c temporarily, don't forget to cleanup at final phase
* fix C code to match pascal and retest to check if diff is none or, at least, reduced.

adt2play_sdl should be the source of truth, C tool should match Pascal one ultimately
