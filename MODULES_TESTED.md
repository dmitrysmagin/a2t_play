# Tested Modules

Results of comparing C (a2m_dump) vs Pascal (adt2_dump) output.

## Status Legend

* PASS - 0 diff lines (identical output)
* INIT-ONLY - only pre-INIT register ordering diffs (benign)
* FRAME-DIFF - real algorithmic frame-level differences (needs investigation)

## Results

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| KULJE_V4 | 15000 | 44082 | FRAME-DIFF | INIT state divergence. |
| ALLOYRUN (VOID) | 19199 | 0 | PASS | |
| HANGOVER (VOID) | 20479 | 0 | PASS | |
| MINDFLUX (VOID) | 10891 | 0 | PASS | |
| RASTER (VOID) | 31999 | 0 | PASS | |
| TERRANIA (VOID) | 19199 | 0 | PASS | |
| andromeda | 58316 | 0 | PASS | |
| altair | 86399 | 0 | PASS | `run_one_test.sh modules/altair.a2m` with `MAX_FRAMES=100000` (2026-05-13): `diff -u` empty — **86399** frames (**172798** dump lines each side). No post–song-end trimming needed. Per **TESTING.md**, no diverging frame → no isolated `a2t.c` / `dump_context()` work. (`TIMEOUT_SEC=180` used for this run; ~68s wall time.) |
| AB_JULIA (tunes/ENCORE) | 34777 | 0 | PASS | `run_one_test.sh tunes/ENCORE/AB_JULIA.A2T` with `MAX_FRAMES=140000` (2026-05-13): `diff -u` empty — **34777** IRQ frames (**69554** dump lines each side). Song ends before frame cap. Per **TESTING.md**, no diverging frame → no isolated `a2t.c` / `dump_context()` work. |
| analogtr (tunes/MLF) | 24959 | 0 | PASS | `run_one_test.sh tunes/MLF/analogtr.a2m` with `MAX_FRAMES=140000` (2026-05-13): `diff -u` empty — **24959** IRQ frames (**49918** dump lines each side). Song ends before frame cap. Per **TESTING.md**, no diverging frame → no isolated `a2t.c` / `dump_context()` work. |
| adven | 38399 | 38 | FRAME-DIFF | Bug 3 fix resolved active playback (was 30 at 21k — 38 at 50k is end-of-song divergence). |
| 1942 | 35991 | 18 | FRAME-DIFF | 18 diff lines (last 3 frames, pre-existing end-of-song divergence). E-line dump added 2026-05-15 — event_table now matches Pascal for active playback. Bug 4 (unconditional eff write) and Bug 5 (init order) fixed. |
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
| os_intro | 15000 | 0 | PASS | `run_one_test.sh modules/os_intro.a2m` with `MAX_FRAMES=15000` (2026-05-13): `diff -u` empty — **15000** frames (**30000** dump lines each side). Per **TESTING.md**, no diverging frame → no isolated `a2t.c` / `dump_context()` work. |
| os_sblas | 15000 | 0 | PASS | 2026-05-13: previously **10499** FRAME-DIFF (`tone portamento target includes fine_tune in C but not in Pascal`). Fixed by keyoff+TonePortamento parity fix + ftune+output_note fix — both now match Pascal. |
| os_wins (madbrain) | 37626 | 5150 | FRAME-DIFF | Bug 3 fix resolved first 21k frames; remaining diffs from frame ~22040 are Bug 2 init artifact. |
| paradox3 | 3896 | 0 | PASS | `run_one_test.sh modules/paradox3.a2m` with `MAX_FRAMES=100000` (2026-05-13): `diff -u` empty — **3896** IRQ frames (**7792** dump lines each side). Song ends before frame cap. Per **TESTING.md**, no diverging frame → no isolated `a2t.c` / `dump_context()` work. |
| pink (PINK.A2T, tunes/MLF) | 15000 | 248 | FRAME-DIFF | `run_one_test.sh tunes/MLF/PINK.A2T` with default `MAX_FRAMES=15000` (2026-05-13): **248** diff lines starting at IRQ **6055** bank **0**. Single persistent -1 offset on `shadow_regs[0][0xA3]` (logical ch0 F-Number Low, `regoffs_n(0)=0x003`). C=`b0` (176), P=`af` (175). Diff repeats in blocks IRQ 6055–6064 (`b0b0`/`afb0`) and 12091–12100 (`b002`/`af02`). Root cause unclear — likely a fine_tune or ftune interaction difference smaller than 1 LSB of the F-Number. ~1.7% of frames affected. |
| PINK.A2M (tunes/MLF) | 50000 | 371 | FRAME-DIFF | Same pattern as A2T version: small repeating pitch nibbles on bank 0, ch0 F-Number Low (reg `0xA3`). |
| popular | 33965 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/popular.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **33965** IRQ frames (**67930** dump lines each side); last frame index **33964**. ~29s wall time. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. |
| remembrance | 38399 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/remembrance.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **38399** IRQ frames (**76798** dump lines each side); last frame index **38398**. ~46s wall time. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. |
| speed_reset_song103 | 2074 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/speed_reset_song103.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **2074** IRQ frames (**4148** dump lines each side). ~4s wall time. Covers **speed_reset** effect on **`song103`**-style module; song ends before frame cap. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. |
| square | 19588 | 48 | FRAME-DIFF | 48 diff lines (last 5 frames, pre-existing end-of-song divergence). E-line added 2026-05-15; event_table now matches Pascal for active playback. |
| sweetsin (modules/kvee) | 30000 | 218 | FRAME-DIFF | `run_one_test.sh modules/kvee/sweetsin.a2m`: **218** diff lines at IRQ **30–37** bank 0. KSL/TL init artifact (release_sustaining_sound vol=63 vs Pascal 0x00). |
| whereru | 30719 | 0 | PASS | Re-run 2026-05-13 (`run_one_test.sh modules/whereru.a2m`, `MAX_FRAMES=100000`, `TIMEOUT_SEC=300`): **`diff -u`** empty — **30719** IRQ frames (**61438** dump lines each side); last frame index **30718**. ~25s wall time. Per **TESTING.md**, no diverging frame → no isolated **`a2t.c`** / **`dump_context()`** work this pass. *(Earlier snapshot: FRAME-DIFF ~44k lines, **ch5** freq ~frame **4653**; current tree matches Pascal.)* |

### modules/diodema

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 1856step | 30000 | 60016 | FRAME-DIFF | MB + 24 pre-existing non-MB (init volume artifact). |
| acidtrac | 30000 | 0 | PASS | |
| adr1ft | 30000 | 2952 | FRAME-DIFF | MB-only: `FINISHED` vs `IDLE` (keyoff_loop flag mismatch). |
| altair | 30000 | 0 | PASS | |
| aquarius | 30000 | 62934 | FRAME-DIFF | MB + 3024 pre-existing non-MB (±1 nibble offsets + volume artifact). |
| ca54 | 30000 | 0 | PASS | |
| catpeopl | 30000 | 0 | PASS | |
| milinda | 30000 | 0 | PASS | |
| mmori | 30000 | 0 | PASS | |
| mp77 | 30000 | 0 | PASS | |
| no72 | 30000 | 0 | PASS | |
| null | 30000 | 26992 | FRAME-DIFF | MB + 1680 pre-existing non-MB (±1 nibble offsets). |
| oddtime | 30000 | 0 | PASS | |
| phone | 30000 | 0 | PASS | |
| psg | 30000 | 0 | PASS | |
| rf62 | 30000 | 0 | PASS | |
| ru41 | 30000 | 0 | PASS | |
| samsara | 30000 | 0 | PASS | |
| signs | 30000 | 60540 | FRAME-DIFF | MB + 600 pre-existing non-MB (±1 nibble offsets). |
| sv73 | 30000 | 0 | PASS | |
| ty58 | 30000 | 0 | PASS | |
| xmission | 30000 | 0 | PASS | |
| zaxxon | 30000 | 0 | PASS | |

### modules/mlf

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| analogtr | 30000 | 0 | PASS | |
| deorbit | 50000 | 0 | PASS | Bug 3 fix resolved. |
| fm-troni | 30000 | 122 | FRAME-DIFF | TonePortamento keyoff regression (was 50→170, now 122 with 30k default). |
| glass | 50000 | 0 | PASS | Bug 3 fix resolved. |
| ishtar | 30000 | 0 | PASS | |
| khaos | 30000 | 0 | PASS | |
| khaos2 | 30000 | 0 | PASS | |
| lbtrance | 30000 | 0 | PASS | |
| old_001 | 30000 | 0 | PASS | |
| old_002 | 30000 | 38589 | FRAME-DIFF | ±1 nibble offset most frames. ftune/fine_tune interaction. |
| opl303 | 30000 | 212 | FRAME-DIFF | Small ±1 nibble diffs. ftune/fine_tune interaction. |
| pink | 30000 | 371 | FRAME-DIFF | ±1 pitch nibbles bank 0 ch0 F-Number Low. ftune/fine_tune interaction. |
| spacediv | 30000 | 10873 | FRAME-DIFF | ±1 nibble offset many frames. ftune/fine_tune interaction. |

### modules/nula

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| cracker | 30000 | 0 | PASS | |
| fresh | 30000 | 68 | FRAME-DIFF | Small ±1 nibble diffs bank 0. |
| gummi | 30000 | 0 | PASS | |
| imadick | 30000 | 0 | PASS | |
| kulje | 30000 | 0 | PASS | |
| loader | 30000 | 0 | PASS | |
| mario | 30000 | 0 | PASS | |
| menuload | 30000 | 284 | FRAME-DIFF | Small ±1 nibble diffs bank 0. |
| mtkamies | 30000 | 0 | PASS | |
| onward | 30000 | 0 | PASS | |
| pre | 30000 | 0 | PASS | |
| psycho3x | 30000 | 23858 | FRAME-DIFF | ±1 nibble offset across most frames. |
| psycho5 | 30000 | 34088 | FRAME-DIFF | ±1 nibble offset from frame 24+, likely INIT state divergence. |
| spa | 30000 | 0 | PASS | |
| unreal | 30000 | 0 | PASS | |
| unreal2 | 30000 | 0 | PASS | |
| worms | 30000 | 0 | PASS | |
| zalza | 30000 | 0 | PASS | |
| zandax | 30000 | 0 | PASS | |
| zenbowl | 30000 | 0 | PASS | |

### modules/ben

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| ballad | 30000 | 0 | PASS | |
| boss8 | 30000 | 0 | PASS | |
| farhome | 30000 | 0 | PASS | |
| fdance06 | 30000 | 0 | PASS | |
| fdance27 | 30000 | 0 | PASS | |
| fm63b_rv | 30000 | 0 | PASS | |
| fmaven94 | 30000 | 0 | PASS | |
| fmaven95 | 30000 | 0 | PASS | |
| fmaven96 | 30000 | 0 | PASS | |
| fmaven97 | 30000 | 0 | PASS | |
| fmaven98 | 30000 | 0 | PASS | |
| fmavn63b | 30000 | 0 | PASS | |
| gates | 30000 | 0 | PASS | |
| hitech2 | 30000 | 0 | PASS | |
| hitech3 | 30000 | 0 | PASS | |
| hitech3f | 30000 | 0 | PASS | |

### modules/brendan

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| advnture | 30000 | 0 | PASS | |
| amangels | 30000 | 0 | PASS | |
| apoclyps | 30000 | 0 | PASS | |
| banzai | 30000 | 0 | PASS | |
| battleth | 30000 | 0 | PASS | |
| bigguns | 30000 | 0 | PASS | |
| birdston | 30000 | 0 | PASS | |
| bladeraz | 30000 | 0 | PASS | |
| blades | 30000 | 0 | PASS | |
| blades2 | 30000 | 0 | PASS | |
| blizzard | 30000 | 0 | PASS | |
| bonusthm | 30000 | 0 | PASS | |
| callduty | 30000 | 0 | PASS | |
| chiptune | 30000 | 0 | PASS | |
| chivalry | 38187 | 12184 | FRAME-DIFF | Bug 3 fix resolved first 21k frames; remaining difts beyond are ftune interaction. |
| cloudfrv | 30000 | 0 | PASS | |
| conspria | 30000 | 0 | PASS | |
| contrast | 30000 | 0 | PASS | |
| coolcat | 30000 | 0 | PASS | |
| cooldown | 30000 | 0 | PASS | |
| dayhcome | 30000 | 0 | PASS | |
| dayslive | 30000 | 0 | PASS | |
| deceptin | 30000 | 0 | PASS | |
| deepdown | 30000 | 0 | PASS | |
| depressn | 30000 | 0 | PASS | |
| difkhero | 30000 | 0 | PASS | |
| dream7m | 30000 | 0 | PASS | |
| dream7mx | 30000 | 158 | FRAME-DIFF | Small ±1 nibble offsets. |
| echorock | 30000 | 0 | PASS | |
| elskiing | 30000 | 0 | PASS | |
| enchloop | 30000 | 0 | PASS | |
| eviloop | 30000 | 0 | PASS | |
| fiftrial | 30000 | 0 | PASS | |
| frenzy | 30000 | 0 | PASS | |
| friendsh | 30000 | 0 | PASS | |
| frozen | 30000 | 2258 | FRAME-DIFF | Small ±1 nibble offsets. |
| fullthr | 30000 | 0 | PASS | |
| funland | 30000 | 83 | FRAME-DIFF | Small ±1 nibble offsets. |
| goldebel | 30000 | 83 | FRAME-DIFF | Small ±1 nibble offsets. |
| horizon | 30000 | 0 | PASS | |
| hovlane | 30000 | 0 | PASS | |
| hunter | 30000 | 0 | PASS | |
| inevitbl | 30000 | 128 | FRAME-DIFF | Small ±1 nibble offsets. |
| invasion | 30000 | - | untested | timed out |
| kelsey | 30000 | - | untested | timed out |
| lemmings | 30000 | - | untested | timed out |
| lostcaus | 30000 | - | untested | timed out |
| lucky7s | 30000 | - | untested | timed out |
| manifest | 30000 | - | untested | timed out |
| marblect | 30000 | - | untested | timed out |
| mariothm | 30000 | - | untested | timed out |
| mechage | 30000 | - | untested | timed out |
| mechwar | 30000 | - | untested | timed out |
| metro | 30000 | - | untested | timed out |
| modratly | 30000 | - | untested | timed out |
| neversay | 30000 | - | untested | timed out |
| nochoice | 30000 | - | untested | timed out |
| nogofers | 30000 | - | untested | timed out |
| oblivion | 30000 | - | untested | timed out |
| obstacle | 30000 | - | untested | timed out |
| ohirony | 30000 | - | untested | timed out |
| omegavir | 30000 | - | untested | timed out |
| paparazi | 30000 | - | untested | timed out |
| pfectwld | 30000 | - | untested | timed out |
| popcorn | 30000 | - | untested | timed out |
| racecyb | 30000 | - | untested | timed out |
| racecyb2 | 30000 | - | untested | timed out |
| robinthm | 30000 | - | untested | timed out |
| sinister | 30000 | - | untested | timed out |
| sitcom | 30000 | - | untested | timed out |
| skylight | 30000 | - | untested | timed out |
| skysharp | 30000 | - | untested | timed out |
| sleepwrk | 30000 | - | untested | timed out |
| spaceple | 30000 | - | untested | timed out |
| sparkplg | 30000 | - | untested | timed out |
| stormywe | 30000 | - | untested | timed out |
| suspicis | 30000 | - | untested | timed out |
| teamster | 30000 | - | untested | timed out |
| thinkfst | 30000 | - | untested | timed out |
| timewtel | 30000 | - | untested | timed out |
| torpdall | 30000 | - | untested | timed out |
| treacher | 30000 | - | untested | timed out |
| tutheme | 30000 | - | untested | timed out |
| twistdpa | 30000 | - | untested | timed out |
| ultravio | 30000 | - | untested | timed out |
| verynice | 30000 | - | untested | timed out |
| warhouse | 30000 | - | untested | timed out |
| wavesmar | 30000 | - | untested | timed out |
| westbeng | 30000 | - | untested | timed out |
| jdaniels | 30000 | 0 | PASS | |
| laboite | 30000 | 0 | PASS | |
| neurophb | 30000 | 0 | PASS | |
| nowgone | 30000 | 0 | PASS | |
| recherch | 30000 | 0 | PASS | |
| running | 30000 | 0 | PASS | |
| song100 | 30000 | 302 | FRAME-DIFF | ±1 nibble diffs bank 0, ftune/fine_tune interaction. |
| song102 | 30000 | 0 | PASS | |
| song103 | 30000 | 0 | PASS | |
| song105 | 30000 | 0 | PASS | |
| song108 | 30000 | 0 | PASS | |
| sonic | 30000 | 0 | PASS | |
| stormrid | 30000 | 0 | PASS | |
| tanmusik | 30000 | 0 | PASS | |
| trance | 30000 | 0 | PASS | |
| trance2 | 30000 | 5768 | FRAME-DIFF | ±1 nibble offset across many frames, ftune/fine_tune interaction. |
| trouble | 30000 | 0 | PASS | |
| ultra | 30000 | 0 | PASS | |
| village | 30000 | 0 | PASS | |
| waterfls | 30000 | 0 | PASS | |
| worldfal | 30000 | 0 | PASS | |

### modules/hydra

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| crackit | 3300 | 2128 | FRAME-DIFF | Init volume artifact (`release_sustaining_sound`, 0x3f to vol regs) + volume scaling diffs in playback body. |
| intrcoop | 5157 | 13802 | FRAME-DIFF | Init volume artifact (`release_sustaining_sound`, 0x3f to vol regs) + extensive volume scaling divergence across song. |

### modules/encore

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 1_cworld | 30000 | 18 | FRAME-DIFF | Init volume artifact at IRQ 11513–11517, bank 1. |
| 2_prydz | 30000 | 22 | FRAME-DIFF | Init volume artifact at IRQ 1913–1918, bank 1. |
| ab_julia | 34777 | 0 | PASS | (Already tested in main table; included here for section completeness.) |
| amegas | 30000 | 0 | PASS | |
| crazbeat | 30000 | 0 | PASS | |
| fm_house | 30000 | 0 | PASS | |
| fm_tekno | 30000 | 10 | FRAME-DIFF | Late song divergence at IRQ 26479–26481, bank 1. |
| fmhouse2 | 30000 | 0 | PASS | |
| forest | 30000 | 0 | PASS | |
| whatslov | 30000 | 0 | PASS | |

### modules/televics

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| 4thcoast | 50000 | 15556 | FRAME-DIFF | Frame-timing drift: +23 FNUM offset in both channels from frame 29100+. Not Bug 3. |
| 4xmisste | 50000 | 160 | FRAME-DIFF | |
| 5ontelev | 50000 | 14 | FRAME-DIFF | Small end-of-song diff. |
| allfull | 50000 | 6470 | FRAME-DIFF | |
| antilato | 50000 | 0 | PASS | |
| asynth | 50000 | 0 | PASS | |
| atrick | 50000 | 30 | FRAME-DIFF | |
| badlib | 50000 | 62 | FRAME-DIFF | |
| badtraf | 50000 | 996 | FRAME-DIFF | |
| boppinb | 50000 | 54 | FRAME-DIFF | |
| bopptoda | 50000 | 38 | FRAME-DIFF | |
| build | 50000 | 2588 | FRAME-DIFF | |
| butterfl | 50000 | 0 | PASS | |
| composur | 50000 | 0 | PASS | |
| cwack | 50000 | 1742 | FRAME-DIFF | |
| d-rumsb | 50000 | 38 | FRAME-DIFF | |
| damn | 50000 | 14 | FRAME-DIFF | Small end-of-song diff. |
| discwrld | 50000 | 1958 | FRAME-DIFF | |
| drumbnba | 50000 | 46 | FRAME-DIFF | |
| fast | 50000 | 38 | FRAME-DIFF | |
| fuckyrev | 50000 | 270 | FRAME-DIFF | |
| funny | 50000 | 0 | PASS | |
| libretto | 50000 | 30 | FRAME-DIFF | |
| macroron | 50000 | 38 | FRAME-DIFF | |
| phaat | 50000 | 38 | FRAME-DIFF | |
| scary | 50000 | 0 | PASS | |
| shit | 50000 | 62 | FRAME-DIFF | |
| slap | 50000 | 30 | FRAME-DIFF | |
| stuffage | 50000 | 62 | FRAME-DIFF | |
| sunny | 50000 | 38 | FRAME-DIFF | |
| teknoh | 50000 | 0 | PASS | |
| topgear | 50000 | 3998 | FRAME-DIFF | |
| wip | 50000 | 0 | PASS | |
| woods | 50000 | 38 | FRAME-DIFF | |
| yellatfl | 50000 | 38 | FRAME-DIFF | |
| yellowwe | 50000 | 38 | FRAME-DIFF | |

