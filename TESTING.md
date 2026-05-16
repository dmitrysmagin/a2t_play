# Tools

* a2m_dump - C tool under development
* adt2play_sdl/adt2_dump - Pascal tool, the reference code, source of truth

# Goal

Match C tool to have the same data dump as Pascal tool

# Output format

Both tools output 7 lines per IRQ frame:

  <frame_num> 0  <256 reg bytes as hex>        — shadow_regs[0] (OPL register file 0)
  <frame_num> 1  <256 reg bytes as hex>        — shadow_regs[1] (OPL register file 1)
  <frame_num> E  <120 bytes as hex>            — event_table[0..19] (6 bytes/channel: note, instr_def, eff0_def, eff0_val, eff1_def, eff1_val)
  <frame_num> VS <40 bytes as hex>             — voice_table[0..19] (current instrument per channel)
  <frame_num> FP <140 bytes as hex>            — fmpar dump per channel (volM, volC, kslM, kslC, connect)
  <frame_num> GV <8 bytes as hex>              — global volume state (global_volume, fade_out_volume, overall_volume, volume_scaling, percussion_mode)
  <frame_num> WR <320 bytes as hex>            — OPL write ring buffer (64 entries × 5 chars: 2 reg + 2 val + 1 separator)


# Before running
* Compare Pascal and C sources, find correlations

# Testing and Fixing a2t.c
* run run_one_test.sh with module filename and MAX_FRAMES=30000
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
