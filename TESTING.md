
# Testing trivia

# Tools

* C Tool - a2m_dump
how to build: make test
supports songend

* Pascal Tool - adt2play_sdl/adt2_dump - the reference
how to build: go into adt2play_sdl and use Makefile and FreePascal
doesn support songend, loops forever

# Output format

Both tools output 2 lines per IRQ frame:

  <frame_num> 0 <256 reg bytes as hex>
  <frame_num> 1 <256 reg bytes as hex>

frame_num starts at 0. No INIT dump is emitted.

# Testing and Fixing
* run run_one_test.sh with module filename and MAX_FRAMES=100000
* compare diffs, but ignore post songend diff
* isolate the code region in a2t.c specific to frames at which diff is found
* if needed, add tCHDATA context dump into a2t.c temporarily, don't forget to cleanup at final phase
* fix C code to match pascal and retest to check if diff is none or, at least, reduced.

adt2play_sdl should be the source of truth, C tool should match Pascal one ultimately

take OPL.DOC (plain text) into account to facilitate register map identification
