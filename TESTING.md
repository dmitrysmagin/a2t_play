# Tools

* a2m_dump - C tool under development
* adt2play_sdl/adt2_dump - Pascal tool, the reference code, source of truth

# Goal

Match C tool to have the same data dump as Pascal tool

# Output format

Both tools output 2 lines per IRQ frame:

  <frame_num> 0 <256 reg bytes as hex>
  <frame_num> 1 <256 reg bytes as hex>
  <frame_num> E <event_table bytes as hex string>
  <frame_num> F <freq_table bytes as hex string>
  <frame_num> ET <effect_table/effect_table2 as hex string>
  <frame_num> PT <porta_table/porta_table2 as hex string>


# Before running
* Compare Pascal and C sources, find correlations

# Testing and Fixing a2t.c
* run run_one_test.sh with module filename and MAX_FRAMES=50000
* compare diffs, note the frames region at which diff is found
* a2m_dump: modify to isolate the frame region at which diff starts
* a2m_dump: add additional dumping information if needed
* a2m_dump: build with trace enabled A2M_DUMP_CONTEXT=1 and rerun test
* analyze trace output to determine root cause
* apply fix and retest to check if diff is smaller or none
* don't edit out comments!
* propose next possible actions if diff not resolved completely
* update MODULES_TESTED.md in any case - success (leave notes void) or fail (add notes then)
* cleanup any trace or dump or debug code in a2t.c
