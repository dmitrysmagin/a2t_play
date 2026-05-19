# Tools

* a2m_dump - C tool under development
* adt2play_sdl/adt2_dump - Pascal tool, the reference code, source of truth

# Goal

Match C tool to have the same data dump as Pascal tool

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

# Output format

Both tools output lines per IRQ frame:

  <frame_num> <pattern> <line> <ticks> <tick0> <tickD> <tickXF> 0  <256 reg bytes as hex>        — shadow_regs[0] (OPL register file 0)
  <frame_num> <pattern> <line> <ticks> <tick0> <tickD> <tickXF> 1  <256 reg bytes as hex>        — shadow_regs[1] (OPL register file 1)
  <frame_num> <pattern> <line> <ticks> <tick0> <tickD> <tickXF> ET <120 bytes as hex>            — event_table[0..19] (6 bytes/channel: note, instr_def, eff0_def, eff0_val, eff1_def, eff1_val)
  <frame_num> <pattern> <line> <ticks> <tick0> <tickD> <tickXF> VS <40 bytes as hex>             — voice_table[0..19] (current instrument per channel)
  <frame_num> <pattern> <line> <ticks> <tick0> <tickD> <tickXF> FP <140 bytes as hex>            — fmpar dump per channel (volM, volC, kslM, kslC, connect)
  <frame_num> <pattern> <line> <ticks> <tick0> <tickD> <tickXF> GV <8 bytes as hex>              — global volume state (global_volume, fade_out_volume, overall_volume, volume_scaling, percussion_mode)

Context fields (prepended to every line):
  <pattern>   — current_pattern (pattern index in the pattern order list)
  <line>      — current_line (row index within the current pattern)
  <ticks>     — total tick counter (increments each IRQ)
  <tick0>     — tick counter reset point (used for tempo/speed timing)
  <tickD>     — pattern delay tick counter (counts down during pattern delay)
  <tickXF>    — extra-fine tick counter (used for extra-fine effects)

Additional debug lines (may be added/removed during investigation):

  <frame_num> <pattern> <line> <ticks> <tick0> <tickD> <tickXF> RT <40 bytes as hex>            — retrig_table[0][0..19] (retrigger counter per channel)
  <frame_num> <pattern> <line> <ticks> <tick0> <tickD> <tickXF> RC <40 bytes as hex>            — reset_chan[0..19] (reset flag per channel, 0 or 1)
