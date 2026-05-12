# Tested Modules

Results of comparing C (a2m_dump) vs Pascal (adt2_dump) output.

## Status Legend

* PASS - 0 diff lines (identical output)
* INIT-ONLY - only pre-INIT register ordering diffs (benign)
* FRAME-DIFF - real algorithmic frame-level differences (needs investigation)

## Results

| Module | Frames | Diff Lines | Status | Notes |
|--------|--------|-----------|--------|-------|
| altair | 86399 | 0 | PASS | |
| adven | 38399 | 0 | PASS | |
| 1942 | 11997 | 0 | PASS | NOW PASSES (was INIT-ONLY) |
| 2_prydz | - | - | PASS | |
| 3812funk | - | - | PASS | |
| bxx_nowgone | - | - | PASS | |
| fank5 | 93738 | 15768 | FRAME-DIFF | constant frequency offset on ch0 from frame 4812; ch0 start freq differs by 4 steps |
| mmori | - | - | PASS | |
| badapple | - | - | FRAME-DIFF | 390 frame-level diffs |
| badseed | 38019 | 0 | PASS | |
| ca54 | - | - | FRAME-DIFF | |
| crisis | - | - | FRAME-DIFF | 112 frame-level diffs |
| fm-troni | - | - | FRAME-DIFF | 120 frame-level diffs |
| fm63b_rv | - | - | FRAME-DIFF | 321 frame-level diffs |
| os_sblas | 68181 | 10499 | FRAME-DIFF | tone portamento target includes fine_tune in C but not in Pascal |
| pink | - | 1503 | FRAME-DIFF | not yet investigated |
| whereru | 30719 | ~44555 | FRAME-DIFF | ch5 freq diverges at frame 4653; cause unclear |
