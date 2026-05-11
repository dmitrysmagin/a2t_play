# C vs Pascal Code Flow Comparison

## Reference Sources

- **Pascal (full tracker):** `adt2unit.pas` (player engine), `adt2opl3.pas` (SDL audio callback) from `adt2_sourcecode_12-28-2024\`
- **C port:** `src\a2t.c` from `a2t_play`

Excludes: GO32V2/DOS-specific code, WAV dumping, fast_forward/rewind, replay_forbidden.

---

## Entry Point: `play_callback_proc` vs `a2t_update`

### Pascal (`adt2opl3.pas:508`)
```
sample_frame_size := ROUND(sdl_sample_rate / 50 * (1 + sdl_timer_slowdown / 100))
                                                 // computed once in snd_init(), fixed at 50 Hz
for counter := 0 to PRED(len DIV 4) do
    Inc(counter_idx)
    if counter_idx >= sample_frame_size then
        counter_idx := 0
        if (ticklooper > 0) then
            if (fast_forward or rewind) and NOT replay_forbidden then poll_proc
        else if NOT replay_forbidden then
            poll_proc
        if (macro_ticklooper = 0) then macro_poll_proc
        Inc(ticklooper)
        if ticklooper >= IRQ_freq_val DIV tempo then ticklooper := 0
        Inc(macro_ticklooper)
        if macro_ticklooper >= IRQ_freq_val DIV (tempo*macro_speedup) then macro_ticklooper := 0
    OPL3EMU_PollProc(...)
```

### C (`a2t.c:4305`)
```
framesmpl = freqhz / irq_freq                         // recomputed inside callback when tempo changes
for cntr = 0; cntr < len; cntr += 4 do
    if cnt >= framesmpl then
        cnt = 0
        if ticklooper == 0 then
            poll_proc()
            if irq_freq != tempo * _macro_speedup() then
                irq_freq = (tempo < 18 ? 18 : tempo) * _macro_speedup()
                framesmpl = freqhz / irq_freq
        if macro_ticklooper == 0 then macro_poll_proc()
        ticklooper++
        if ticklooper >= irq_freq / tempo then ticklooper = 0
        macro_ticklooper++
        if macro_ticklooper >= irq_freq / (tempo * _macro_speedup()) then macro_ticklooper = 0
    OPL3_GenerateStream(&opl, (int16_t *)(stream + cntr), 1)
    cnt++
```

### Differences

| Aspect | Pascal | C |
|--------|--------|----|
| Frame size | Static: `ROUND(sdl_sample_rate/50*...)` — hardcoded to 50 Hz | Dynamic: `freqhz / irq_freq` — adapts to actual IRQ frequency |
| Frequency variable | Single `IRQ_freq_val` (derived from global `IRQ_freq`) | **BUG**: Dual variables — `IRQ_freq` (global, modulus-aligned) and `irq_freq` (local static, unaligned). Ticklooper wrap uses `irq_freq`, player logic uses `IRQ_freq`. |
| irq_freq formula | `IRQ_freq` set by `update_timer()` with modulus alignment loop | `irq_freq = (tempo < 18 ? 18 : tempo) * _macro_speedup()` — no alignment |
| Ticklooper condition | `ticklooper > 0 + fast_forward/rewind` or `ticklooper == 0` | `ticklooper == 0` only |
| Reactive recalculation | No (frame size fixed at init) | Yes: recalculates `irq_freq`/`framesmpl` when `tempo` or `macro_speedup` changes |

---

## `poll_proc`

### Pascal (`adt2unit.pas:4040`)
```
if NOT pattern_delay and (ticks-tick0+1 >= speed) or fast_forward or rewind or single_play then
    if debugging and not single_play and not pattern_break and
       (NOT space_pressed or no_step_debugging) then EXIT
    if not single_play and not play_single_patt then
        if order > $7f then calc_order_jump check
        current_pattern := pattern_order[current_order]
    play_line
    if not (single_play or fast_forward or rewind) then update_effects
    else for temp := 1 to speed do
             update_effects
             if temp MOD 4 = temp then update_extra_fine_effects
             ticks++
    tick0 := ticks
    if not single_play and (fast_forward or rewind or not pattern_delay) then
        update_song_position
    // fast_forward/rewind: synchronize_song_timer + auto-stop
else
    update_effects
    ticks++
    if pattern_delay and tickD > 1 then Dec(tickD)
    else begin
        if pattern_delay and not single_play then
            tick0 := ticks
            update_song_position
        pattern_delay := false
    end
tickXF++
if tickXF MOD 4 = 0 then
    update_extra_fine_effects
    Dec(tickXF, 4)
```

### C (`a2t.c:2865`)
```
if pattern_delay then
    update_effects()
    if tickD > 1 then tickD--
    else pattern_delay = false
else
    if ticks == 0 then
        play_line()
        ticks = speed
        update_song_position()
    update_effects()
    ticks--
tickXF++
if tickXF % 4 == 0 then
    update_extra_fine_effects()
    tickXF -= 4
```

### Differences

| Aspect | Pascal | C | Verdict |
|--------|--------|----|---------|
| Line advance trigger | `ticks-tick0+1 >= speed` | `ticks == 0` (countdown from `speed`) | Same net timing |
| pattern_delay branch | Full handling with `tick0` sync and `single_play` guard | Simplified: just decrement `tickD` or clear flag | Missing `single_play` guard, but no impact on normal playback |
| fast_forward/rewind | Full support | Not present | Feature gap |
| single_play/play_single_patt | Full support | Not present | Feature gap |
| debugging/step mode | Full support | Not present | Feature gap |
| Pattern advance guard | `order > $7f` → `calc_order_jump` | Handled in `update_song_position` → `set_current_order` | Same |

---

## `play_line`

### Pascal (`adt2unit.pas:1418`)

**Pass 1 — Effect state:**
```
For each channel:
    Save last_effect from effect_table
    Apply glfsld → effect_table, glfsld2 → effect_table2
    Clear ftune_table
    Read event from pattdata^[pattern div 8][pattern mod 8][chan][line]
    Fix note: BYTE_NULL → keyoff, fixed_note_flag → raw
    Set event_new[chan] flag
    Copy effect_def/effect to event_table
```

**Pass 2 — Instrument & state reset:**
```
For each channel:
    set_ins_data (with release_sustaining_sound for empty instruments)
    Reset vibr_table/trem_table unless relevant effect persists
    Reset retrig_table unless RetrigNote/MultiRetrigNote
    Reset trem_table unless Tremolo/ExtraFineTremolo
    Reset arpeggio state unless Arpeggio/ExtraFineArpeggio active
    Handle GlobalFSlideUp/Down with sub-command dispatch
    Same for slot 2
```

**Pass 3 — 4-op & tremor:**
```
For each channel:
    Propagate event_new to 4-op pair channel
    Restore tremor volume if tremor effect ended
```

**Pass 4 — Effect-specific setup (large switch):**
```
For each channel:
    Arpeggio → set arpgg_table.state/note/add1/add2
    FSlideUp/Down → set effect_table, fslide_table
    FSlideUpVSlide family → set effect_table with last-value memory
    TonePortamento → set effect_table, porta_table.speed/freq
    Vibrato → set effect_table, vibr_table
    Tremolo → set effect_table, trem_table
    VolSlide → set effect_table (with last-value)
    RetrigNote → set retrig_table
    Tremor → save current volume to tremor_table
    NoteDelay → set notedel_table
    Extended/Extended2 → sub-command dispatch
    Same for slot 2
```

**Pass 5 — Note output:**
```
For each channel:
    process_effects_setup
    new_process_note
    check_swap_arp_vibr
    update_fine_effects
```

### C (`a2t.c:2002`)

**Pass 1 — Effect state:**
```
For each channel:
    Save effect_table → last_effect
    Apply glfsld → effect_table, glfsld2 → effect_table2
    Clear ftune_table
```

**Pass 2 — Event & note:**
```
For each channel:
    Read event via get_event_p(current_pattern, chan, current_line)
    Fix note: 0xFF → keyoff, fixed_note_flag → raw
    Copy effect_def/effect to event_table
    set_ins_data(event->instr_def, chan)         // NO release_sustaining_sound guard
    process_effects(event, 0, chan)              // handles effect setup + dispatching
    process_effects(event, 1, chan)
    new_process_note(event, chan)
    check_swap_arp_vibr(event, 0, chan)
    check_swap_arp_vibr(event, 1, chan)
    update_fine_effects(0, chan)
    update_fine_effects(1, chan)
```

### Differences

| Missing in C | Location | Impact |
|-------------|----------|--------|
| `release_sustaining_sound` when loading empty instrument | `set_ins_data` | **Bug**: sustained note doesn't release when switching to empty instrument |
| `event_new` array | Pass 1 | **Missing**: 4-op propagation depends on this (may be handled elsewhere) |
| `reset_adsrw` processing | Pass 1 | Low: ADSR reset is editor-initiated |
| Vibrato/Tremolo/Retrig table reset | Pass 2 | **Bug**: stale vibrato/tremolo state may persist when effect is removed |
| Tremor volume save/restore | Pass 2 | **Bug**: tremor mutes with `set_ins_volume(63,63)` instead of saved volume, audible if instrument volume != 63 |
| Arpeggio state reset | Pass 2 | Low: handled in `process_effects` but without last-effect memory edge cases |
| GlobalFSlideUp/Down with sub-commands | Pass 2 | Editor feature, no playback impact |
| 4-op event_new propagation | Pass 3 | Low: 4-op pair sync handled elsewhere |

---

## `update_effects`

### Pascal (`adt2unit.pas:3341`)
```
For each channel:
    eLo = LO(effect_table[chan]), eHi = HI(effect_table[chan])
    Case eLo of
        ef_Arpeggio+ef_fix1:         arpeggio(chan)
        ef_ArpggVSlide:              volume_slide + arpeggio
        ef_FSlideUp:                 portamento_up
        ef_FSlideDown:               portamento_down
        ef_FSlideUpVSlide:           portamento_up + volume_slide
        ef_FSlideDownVSlide:         portamento_down + volume_slide
        ef_TonePortamento:           tone_portamento
        ef_TPortamVolSlide:          volume_slide + tone_portamento
        ef_Vibrato:                  vibrato (if not fine)
        ef_Tremolo:                  tremolo (if not fine)
        ef_VibratoVolSlide:          volume_slide + vibrato
        ef_VolSlide:                 volume_slide
        ef_RetrigNote:               retrig counter → output_note
        ef_MultiRetrigNote:          retrig counter + volume slide → output_note
        ef_Tremor:                   tremor on/off
        ef_Extended/ef_Extended2:    sub-commands (note delay, note cut, etc.)
        ...and more (full effect set)
    Case eLo2 of  (same dispatch for slot 2)
        ...and more
```

### C (`a2t.c:2665`)
```
For each channel:
    update_effects_slot(0, chan)
    update_effects_slot(1, chan)
```

`update_effects_slot` dispatches on `effect_table[slot][chan].def` with the identical effect case list: arpeggio, portamento up/down, tone portamento, vibrato, tremolo, volume slide, retrig, multi-retrig, tremor, note delay, note cut, global volume slide, etc.

**Verdict:** Functionally identical. Same effect IDs, same handler functions, same effect semantics. C structure is cleaner with slot-based dispatch instead of copy-pasted `eLo`/`eLo2` blocks.

---

## `update_extra_fine_effects`

### Pascal (`adt2unit.pas:3836`)
```
For each channel:
    Case eLo of:
        ef_ex2_GlVolSldUpXF:    global_volume_slide
        ef_ex2_GlVolSldDnXF:    global_volume_slide
        ef_ex2_VolSlideUpXF:    volume_slide
        ef_ex2_VolSlideDnXF:    volume_slide
        ef_ex2_FreqSlideUpXF:   portamento_up
        ef_ex2_FreqSlideDnXF:   portamento_down
        ef_ExtraFineArpeggio:   arpeggio
        ef_ExtraFineVibrato:    vibrato (if not fine)
        ef_ExtraFineTremolo:    tremolo (if not fine)
    Case eLo2 of:  (same for slot 2)
        ...and more
```

### C (`a2t.c:2744`)
```
For each channel:
    update_extra_fine_effects_slot(0, chan)
    update_extra_fine_effects_slot(1, chan)
```

Same extra-fine effect dispatch. Identical handler functions.

**Verdict:** Functionally identical.

---

## `update_song_position`

### Pascal (`adt2unit.pas:3957`)
```
1. Update play_pos_buf history ring buffer (8 entries)
2. If NOT rewind:
   a. If not pattern end and not pattern_break → Inc(current_line)
   b. Else:
      - If pattern_loop condition and repeat_pattern → line = 0
      - Else:
        if not pattern_loop and order < $7f → Inc(order), clear loop tables
        if pattern_loop (ZCx/ZDx): next_line = loopbck_table[chan]
        if pattern_break (Bxx): current_order = event_table[chan].effect/effect2
        if order > $7f → order = 0
        calc_order_jump check
        current_pattern = pattern_order[current_order]
        current_line = 0 (or next_line if pattern_break)
   c. If rewind → Dec(current_line)
3. Clear ignore_note_once[], glfsld_table[] for all channels
4. If not play_single_patt and line==0 and order==0 and speed_update:
       restore tempo/speed
```

### C (`a2t.c:2800`)
```
1. If not pattern end and not pattern_break → current_line++
2. Else:
   a. If pattern_loop (ZCx/ZDx):
        next_line = loopbck_table[chan]
        loop_table[chan][line]--
   b. Else:
        Clear loopbck_table, loop_table
        If pattern_break (Bxx):
            set_current_order(val)        // from event_table
            pattern_break = false
        Else:
            set_current_order(order + 1)
   c. calc_order_jump inlined in set_current_order (128-iteration guard)
   d. current_pattern = pattern_order[current_order]
   e. current_line = 0 (or next_line if pattern_break)
3. Clear glfsld_table for all channels
4. If speed_update and line==0 and order==calc_following_order(0):
       restore tempo/speed
```

### Differences

| Missing in C | Location | Impact |
|-------------|----------|--------|
| `play_pos_buf` history | Step 1 | Editor-only, no playback impact |
| `repeat_pattern` | Step 2b | **Missing**: repeat mode wraps to line 0 at song end |
| `rewind` decrement line | Step 2c | Feature gap |
| `ignore_note_once` clear | Step 3 | **Bug**: note-ignore flag never cleared, may cause stale suppression |
| `play_single_patt` guard | Step 4 | Feature gap |
| `calc_order_jump` | Step 2c | Same result, different implementation (C inlines with 128-iteration guard) |

---

## `macro_poll_proc`

### Pascal (`adt2unit.pas:4138`) vs C (`a2t.c:2891`)

Already verified identical in earlier analysis. Same:
- FM register macro position/duration advancement with loop/keyoff handling
- Same IDLE ($0fff) / FINISHED ($0ffff) sentinels
- Same disabled_fmreg_col[] per-bit masking for each OPL register field
- Same force_macro_keyon logic for empty ADSR instruments
- Same MACRO_NOTE_RETRIG_FLAG / MACRO_ENVELOPE_RESTART_FLAG / MACRO_ZERO_FREQ_FLAG handling
- Same arpeggio macro table advancement with loop/keyoff
- Same vibrato macro table advancement with loop/keyoff/delay
- Same portamento_up/down for vibrato values

**Verdict:** Functionally identical.

---

## Summary of Playback-Relevant Discrepancies

| # | Missing/Bug in C | Location | Severity |
|---|-----------------|----------|----------|
| 1 | `IRQ_freq` / `irq_freq` dual variables — ticklooper wrap uses unaligned local | `a2t_update` | **Bug** |
| 2 | No `release_sustaining_sound` when loading empty instrument | `play_line` → `set_ins_data` | **Bug** |
| 3 | Vibrato/Tremolo/Retrig table not reset when effect removed | `play_line` | **Bug** |
| 4 | Tremor uses mute instead of saved volume | `play_line` | **Bug** (audible if instr vol != 63) |
| 5 | `ignore_note_once` never cleared | `update_song_position` | **Bug** |
| 6 | `repeat_pattern` mode missing | `update_song_position` | Feature gap |
| 7 | Frame size adaptive (Pascal is 50Hz-locked) | Entry point | C is actually **better** |
