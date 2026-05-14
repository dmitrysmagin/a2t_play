# Tested Modules

Results of comparing C (a2m_dump) vs Pascal (adt2_dump) output.

## Status Legend

* PASS - 0 diff lines (identical output)
* INIT-ONLY - only pre-INIT register ordering diffs (benign)
* FRAME-DIFF - real algorithmic frame-level differences (needs investigation)

## Results

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 1856step | 73727 | 44 | FRAME-DIFF | `run_one_test.sh modules/1856step.a2m` with `MAX_FRAMES=140000` (2026-05-13): **44** diff lines at IRQ **5032–5043**, bank **1** registers `shadow_regs[1][0x52]` (ch17 mod TL) and `[0x55]` (ch17 car TL). C=0x3F (muted via `release_sustaining_sound` init for all 20 ch), Pascal=0x00. ch17 beyond `nm_tracks`, stays muted in C for 12 frames until frame 5044. Cosmetic init artifact. |
| adr1ft (modules/diodema) | 15000 | 0 | PASS | `run_one_test.sh modules/diodema/adr1ft.a2m` with default `MAX_FRAMES=30000` (2026-05-13): `diff -u` empty. Fixed by `fmreg_table_allocate` guard change: allocation uses `src[0]` (original length byte) instead of inferred `real_length`. Pascal's `macro_poll_proc` terminates FMREG when `length=0` via `(fmreg_pos < length) else finished_flag`, never processing cells. Previously **40172** diff lines — C was inferring length from non-zero cell data, allocating the table, and running the macro when Pascal would not. |
| KULJE_V4 | 15000 | 44082 | FRAME-DIFF | `run_one_test.sh modules/KULJE_V4.a2m` with default `MAX_FRAMES=15000` (2026-05-13): **44082** diff lines from frame **0** across all registers — INIT state divergence. Previously blocked by `assert(i)` crash at `src/a2t.c:2470` in `slide_volume_down` (voice_table[chan] out of instr range). Fix: replaced `assert(i)` with `if (!i) return;` in both `slide_volume_up` and `slide_volume_down`. The init diff is pre-existing (waveform registers, ADSR, operator params all differ from frame 0), causing frame-by-frame divergence throughout playback. C uses `regoffs` computed live via `!!percussion_mode` in init, Pascal uses `_chan_n/m/c` arrays set after `stop_playing` — stale during stop. |
| sweetsin (kvee) | 4307 | 218 | FRAME-DIFF | `run_one_test.sh modules/kvee/sweetsin.a2m` with default `MAX_FRAMES=30000` (2026-05-13): **218** diff lines at IRQ **30–37**, bank **0** only. KSL/TL volume registers differ during init frames — same `release_sustaining_sound` init artifact as `1856step` and `KULJE_V4`. Song has 4307 frames, diff resolves after frame 37. |
| aquarius (modules/diodema) | 15000 | 122 | FRAME-DIFF | `run_one_test.sh modules/diodema/aquarius.a2m` with default `MAX_FRAMES=15000` (2026-05-13): **122** diff lines at IRQ **10467–10475** and **10476–10478**, bank **0** only. Diff at register `shadow_regs[0][0xC1]` (ch1 FEEDBACK/CONNECTION). C toggles `connect` and `feedb` between cell values via FMREG macro; Pascal stays constant. Same root cause as `adr1ft`: C applies macro cell columns (`dis_fmreg_cols=0` = all enabled) while Pascal doesn't — dis_fmregs logic inversion hypothesis. |
| ALLOYRUN (VOID) | 19199 | 0 | PASS | |
| HANGOVER (VOID) | 20479 | 0 | PASS | |
| MINDFLUX (VOID) | 10891 | 0 | PASS | |
| RASTER (VOID) | 31999 | 0 | PASS | |
| TERRANIA (VOID) | 19199 | 0 | PASS | |
| andromeda | 58316 | 0 | PASS | |
| altair | 86399 | 0 | PASS | `run_one_test.sh modules/altair.a2m` with `MAX_FRAMES=100000` (2026-05-13): `diff -u` empty — **86399** frames (**172798** dump lines each side). No post–song-end trimming needed. Per **TESTING.md**, no diverging frame → no isolated `a2t.c` / `dump_context()` work. (`TIMEOUT_SEC=180` used for this run; ~68s wall time.) |
| AB_JULIA (tunes/ENCORE) | 34777 | 0 | PASS | `run_one_test.sh tunes/ENCORE/AB_JULIA.A2T` with `MAX_FRAMES=140000` (2026-05-13): `diff -u` empty — **34777** IRQ frames (**69554** dump lines each side). Song ends before frame cap. Per **TESTING.md**, no diverging frame → no isolated `a2t.c` / `dump_context()` work. |
| analogtr (tunes/MLF) | 24959 | 0 | PASS | `run_one_test.sh tunes/MLF/analogtr.a2m` with `MAX_FRAMES=140000` (2026-05-13): `diff -u` empty — **24959** IRQ frames (**49918** dump lines each side). Song ends before frame cap. Per **TESTING.md**, no diverging frame → no isolated `a2t.c` / `dump_context()` work. |
| adven | 38399 | 0 | PASS | |
| 1942 | 100000 | 0 | PASS | Re-checked 2026-05-13 (`MAX_FRAMES=100000`); 0 diff. (Earlier note: was INIT-ONLY once; now identical to Pascal for full dump.) |
| top-2act | 43802 | 0 | PASS | Re-checked 2026-05-13 (`MAX_FRAMES=100000`, `TIMEOUT_SEC=360`): **`diff -u`** empty. Root cause: instrument **19** had FMREG **`length==0`** and empty cells but **`src[5]`** selected vibrato table **1**; **`fmreg`** was not allocated so **`instrument->vibrato`** never propagated (**`src/a2t.c`** **`fmreg_table_allocate`** now allocates when **`src[1]|…|src[5]`** is non-zero). Cell inference when **`length==0`** retained. Optional **`tools/fmreg_peek.c`** can inspect FMREG blobs. |
| 3812funk | - | - | PASS | |
| bxx_nowgone | - | - | PASS | |
| fank5 | 93738 | 0 | PASS | Re-run 2026-05-13 (`MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty. Fixed by keyoff+TonePortamento parity fix: `porta.freq` is no longer overwritten during keyoff (matching Pascal's `event[chan].note in [1..97]` guard), enabling the portamento slide to proceed. Previously **3002** diff lines. |
| mmori | 100000 | 3722 | FRAME-DIFF | `MAX_FRAMES=100000` (2026-05-13): **3722** diff lines (was **33770** before fix). Main bank 1 divergence at IRQ **28804** resolved. Root cause: Pascal's `ef_TonePortamento` handler checks `event[chan].note in [1..12*8+1]` — keyoff notes fixed to `0x80|old_note` FAIL this check (value > 97), so `porta.freq` is NOT updated. C stripped the keyoff bit for the range check (finding `51 in [1..97]` → TRUE), then overwrote `porta.freq` with `freq_table & 0x1fff` — making it equal to the keyoff frequency and suppressing the slide. Fix: removed the keyoff `porta.freq` assignment, matching Pascal's behavior. Also fixed **fank5**, **ca54**, and improved **badapple** (93% reduction). Minor regression on **fm-troni** (50→170 lines). |
| farhome | 21119 | 0 | PASS | |
| badapple | 100000 | 0 | PASS | 2026-05-13 (`MAX_FRAMES=140000`): full pass (was **5090** before fix). Root cause: row **112** ch4 has `ef_Extended2` (0x24, **not** ArpggVSlide as originally misidentified) with val `0x41` → FineTuneUp +1. Pascal's `output_note` allows `note=0` when `ftune_table!=0` — it applies ftune to `freq_table[chan]` even on empty-note rows. C's `new_process_note` had `if (event->note == 0) return;` which returned **before** `output_note` had a chance to apply the ftune. Fix: call `output_note(0, ...)` when `event->note==0` and `ftune_table[chan]!=0`. Also fixed by same change: **fank5**, **ca54** (both now PASS). |
| badseed | 38019 | 0 | PASS | |
| corridor | 49139 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/corridor.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **49139** IRQ frames (**98278** dump lines each side). ~173s wall time. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. *(Earlier snapshot listed FRAME-DIFF / ~36k-line diff; current tree matches Pascal.)* |
| ca54 | 72583 | 0 | PASS | 2026-05-13: reduced from **129429→0** diff lines by keyoff+TonePortamento parity fix. Both bank 0 and bank 1 now match Pascal throughout. The earlier first divergence at IRQ **2560** bank 1 (`shadow_regs[1][0xA2]` ch14 fnum) was caused by the same keyoff+portamento target mismatch. |
| class05 | 24959 | 0 | PASS | `run_one_test.sh modules/class05.a2m` with `MAX_FRAMES=100000` (2026-05-13): `diff -u` empty — **24959** IRQ frames (**49918** dump lines each side). Song ends before frame cap. Per **TESTING.md**, no diverging frame → no isolated `a2t.c` / `dump_context()` work. |
| crisis | 55870 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/crisis.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **55870** IRQ frames (**111740** dump lines each side); last frame index **55869**. ~33s wall time. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. *(Earlier snapshot: FRAME-DIFF ~28k lines @ frame **19580**, **`shadow_regs[0][0xA1]`** C=`57` vs Pascal=`58`; current tree matches Pascal — likely helped by **`macro_poll_proc`** looping **`nm_tracks`** vs fixed **20**, plus other parity work.)* |
| fm-troni | 100000 | 170 | FRAME-DIFF | 2026-05-13: regressed from **50→170** (keyoff+TonePortamento parity fix removed the `freq_table & 0x1fff` guard for FSlide+portamento interaction). The fix aligns with Pascal for the general keyoff+portamento case, but fm-troni has a **FSlideUp** followed by keyoff+portamento where the old `freq_table`-based target prevented an octave-mismatch slide. Future work: re-add the `freq_table` guard when FSlide is active on the same channel. First diff ~**34557** bank 1. |
| fm63b_rv | 17759 | 0 | PASS | Re-run 2026-05-13 (`MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty vs Pascal (**song ends earlier than cap**). **`a2t.c` `new_process_note`**: aligned with **`a2player.pas` ~2648–2674** — **`LO(effect_table)` / `LO(effect_table2)`** block same-row immediate **`output_note`** for **TonePortamento family** *and* **Extended2+NoteDelay** (`effect_slot_blocks_same_row_immediate_note`); **removed** the **`notedelay_flag` early return** so **`~2668`** / **`~2672`** still run when deferral is due to carried effects (matches Pascal’s single **`else If`** chain). Earlier divergence at IRQ **~5305** (**`shadow_regs[1][0xB1]`** key-on bit) cleared by this change. |
| no72 | 15000 | 0 | PASS | `run_one_test.sh modules/no72.a2m` with default `MAX_FRAMES=15000` (2026-05-13): `diff -u` empty. Fix: `fmreg_table_allocate` allocation guard changed from `real_length` (inferred from cell data) to `src[0]` (original length byte). Pascal's `macro_poll_proc` terminates FMREG when `length=0` — `If (fmreg_pos < length) else fmreg_pos := finished_flag` — never processing cell data. C was inferring `length` from non-zero cell data, allocating the table, and running the macro when Pascal would not. Also `instrument->fmreg->length` now uses `src[0]` (original) not `real_length` (inferred) to match Pascal's bounds behavior. |
| os_intro | 15000 | 0 | PASS | `run_one_test.sh modules/os_intro.a2m` with `MAX_FRAMES=15000` (2026-05-13): `diff -u` empty — **15000** frames (**30000** dump lines each side). Per **TESTING.md**, no diverging frame → no isolated `a2t.c` / `dump_context()` work. |
| os_sblas | 15000 | 0 | PASS | 2026-05-13: previously **10499** FRAME-DIFF (`tone portamento target includes fine_tune in C but not in Pascal`). Fixed by keyoff+TonePortamento parity fix + ftune+output_note fix — both now match Pascal. |
| paradox3 | 3896 | 0 | PASS | `run_one_test.sh modules/paradox3.a2m` with `MAX_FRAMES=100000` (2026-05-13): `diff -u` empty — **3896** IRQ frames (**7792** dump lines each side). Song ends before frame cap. Per **TESTING.md**, no diverging frame → no isolated `a2t.c` / `dump_context()` work. |
| pink (PINK.A2T, tunes/MLF) | 15000 | 248 | FRAME-DIFF | `run_one_test.sh tunes/MLF/PINK.A2T` with default `MAX_FRAMES=15000` (2026-05-13): **248** diff lines starting at IRQ **6055** bank **0**. Single persistent -1 offset on `shadow_regs[0][0xA3]` (logical ch0 F-Number Low, `regoffs_n(0)=0x003`). C=`b0` (176), P=`af` (175). Diff repeats in blocks IRQ 6055–6064 (`b0b0`/`afb0`) and 12091–12100 (`b002`/`af02`). Root cause unclear — likely a fine_tune or ftune interaction difference smaller than 1 LSB of the F-Number. ~1.7% of frames affected. |
| PINK.A2M (tunes/MLF) | 50000 | 371 | FRAME-DIFF | Same pattern as A2T version: small repeating pitch nibbles on bank 0, ch0 F-Number Low (reg `0xA3`). |
| popular | 33965 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/popular.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **33965** IRQ frames (**67930** dump lines each side); last frame index **33964**. ~29s wall time. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. |
| remembrance | 38399 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/remembrance.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **38399** IRQ frames (**76798** dump lines each side); last frame index **38398**. ~46s wall time. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. |
| speed_reset_song103 | 2074 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/speed_reset_song103.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **2074** IRQ frames (**4148** dump lines each side). ~4s wall time. Covers **speed_reset** effect on **`song103`**-style module; song ends before frame cap. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. |
| square | 19588 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/square.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **19588** IRQ frames (**39176** dump lines each side). ~19s wall time. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. |
| whereru | 30719 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/whereru.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **30719** IRQ frames (**61438** dump lines each side); last frame index **30718**. ~25s wall time. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. *(Earlier snapshot: FRAME-DIFF ~44k lines, **ch5** freq ~frame **4653**; current tree matches Pascal.)* |

---

## Init Sequence Comparison

### Pascal (adt2_dump.pas → a2player.pas)

```
OPL3EMU_init                  ← chip reset, all regs = 0
init_songdata                 ← clear song info
init_timer_proc
load file (a2m_loader etc.)   ← sets songdata.common_flag etc.
─── start_playing ───
  stop_playing:
    release_sustaining_sound(1..20)  ← vol=63, key_on/off for ALL 20
    opl2out($bd, 0)
    opl3exp($0004), opl3exp($0005)
    init_buffers
  init_player:
    opl2out($01, 0)
    keyoff 18 channels via _chan_n
    clear ADSR $080..$08d, $090..$095
    ← sets percussion_mode, flag_4op from songdata.common_flag
    ← **sets _chan_n/_chan_m/_chan_c** based on percussion_mode (stale during stop!)
    opl2out($01, $20), opl2out($08, $40)
    opl3exp($0105), opl3exp($04 + flag_4op<<8)
    key_off(17), key_off(18)
    opl2out($bd, misc_register)
    global_volume = 63
    voice_table[i] = i, arpgg[i].state = 1
  set_current_order(0)
  play_status := isPlaying
set_overall_volume(63)        ← after frame_hook
```

### C (a2t.c)

```
a2t_init(freq)                ← OPL3_Reset → chip reset, all regs = 0
a2t_load(name)                ← load file into memory
─── a2t_play ───
  a2t_stop:
    release_sustaining_sound(0..19)  ← vol=63, key_on/off for ALL 20
    opl2out($bd, 0)
    opl3exp($0004), opl3exp($0005)
    init_buffers
  a2_import                         ← sets songdata, common_flag etc.
  init_player:
    opl2out($01, 0)
    keyoff 18 channels via regoffs_n
    clear ADSR $080..$08d, $090..$095
    ← percussion_mode, flag_4op etc. already set by a2_import
    opl2out($01, $20), opl2out($08, $40)
    opl3exp($0105), opl3exp($04 + flag_4op<<8)
    key_off(16), key_off(17)
    opl2out($bd, misc_register)
    **init_buffers**                  ← SECOND call (Pascal doesn't)
    global_volume = 63
    voice_table[i] = i+1, arpgg[i].state = 1
  set_current_order(0)
  play_status := isPlaying
set_overall_volume(63)        ← from a2m_dump.c main()
```

### Differences Found

| Aspect | Pascal | C | Impact |
|--------|--------|---|-------|
| `init_buffers` count | Once in `stop_playing` | **Twice** (stop + init_player) | Re-clears channel state after import — redundant, harmless |
| `_chan_n/m/c` / `regoffs` timing | Set in `init_player` AFTER `stop_playing` — **stale during stop** | Computed live via `!!percussion_mode` — **always current** | **Potential bug**: Pascal's `stop_playing` → `release_sustaining_sound` writes vol=63 using old `_chan_m` from PREVIOUS song. If prev song had different `percussion_mode`, it writes to wrong regs |
| `common_flag` parsing | In `init_player` | In `a2_import` (before `init_player`) | Same result, different location |
| `key_off` indices | 17,18 (1-based → C 16,17) | 16,17 (0-based) | **Same** (just 1-based vs 0-based) |
| `speed` / `update_timer` in stop | Not done | `speed=4; update_timer(50)` | C resets tempo/speed, Pascal doesn't |
