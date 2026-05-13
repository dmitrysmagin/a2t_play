
# Testing trivia

# Tools

* C Tool - a2m_dump
how to build: make test
supports songend

* Pascal Tool - adt2play_sdl/adt2_dump - the reference
how to build: go into adt2play_sdl and use Makefile and FreePascal
it is source of truth, C tool should match Pascal one
in case of bugs, they should be replicated in C and marked as 'adt2play bug'

# Output format

Both tools output 2 lines per IRQ frame:

  <frame_num> 0 <256 reg bytes as hex>
  <frame_num> 1 <256 reg bytes as hex>

frame_num starts at 0. No INIT dump is emitted.

# Testing and Fixing
* run run_one_test.sh with module filename and MAX_FRAMES=100000
* compare diffs, but ignore post songend diff
* isolate the code region in a2t.c specific to frames at which diff is found
* use invoke dump_context() from isolated frames to explore deeper, don't forget to cleanup at final phase
* analyze context dump and fix C code to match pascal and retest to check if diff is none or, at least, reduced.
* propose next possible actions if diff not resolved completely

## `A2M_DUMP_CONTEXT` (stderr trace)

GNU **`make` must rebuild `a2m_dump`** with **`DUMP_CONTEXT=1`** or **`make` will say “up to date”** and omit **`-DA2M_DUMP_CONTEXT`**. Prefer a clean binary:

```bash
rm -f a2m_dump a2m_dump.exe
TMP=/tmp make -f Makefile a2m_dump DUMP_CONTEXT=1 CC=/path/to/mingw-gcc
./a2m_dump modules/<tune>.a2m /dev/null <max_frames+margin> 2>test/<tune>_ctx.log
```

Selected IRQ frames are listed in **`src/a2m_dump.c`** (`a2m_dump_context_irq_requested` / **`want[]`**). For **top-2act**, IRQ **10893–10902** use **`a2m_dump_top2act_ch9_compact()`** only (no full **`dump_context_f`** on those lines). Example: **`wc -l test/top-2act_ctx.log`** ~**18526**; **`grep '^######## top-2act' test/top-2act_ctx.log`** → **10** blocks.


take OPL.DOC (plain text) into account to facilitate register map identification
