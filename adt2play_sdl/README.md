# AdLib Tracker II Player — SDL Port

SDL 1.2 port of the DOS-only `adt2play` (AdLib Tracker II module player), following the pattern
established by the `at2/` SDL tracker port.

## Build

```sh
make
```

Requires FreePascal 2.6.0 and SDL 1.2 (`sdl.dll` included).

## Usage

```
adt2play files|wildcards [files|wildcards{...}] [options]
```

**Options:**
| Flag | Effect |
|---|---|
| `/jukebox` | Play modules with no repeat |
| `/gfx` | Graphical (320×200 picture) interface |
| `/latency` | OPL3 latency compatibility mode |

**Controls (while playing):**
| Key | Action |
|---|---|
| `Space` hold | Fast-forward |
| `Backspace` | Restart current module |
| `Enter` | Skip to next module |
| `Escape` | Quit |

## Supported module formats

A2M, A2T, AMD, CFF, DFM, MTK, RAD, S3M, FMK, SAT, SA2, HSC.

## Porting summary

| Phase | File(s) | Work |
|---|---|---|
| 1 | Build scaffolding | `adt2play_sdl/` created with SDL bindings, `opl3emu.pas`, shared sources, `Makefile` |
| 2 | `a2player.pas` | Removed DOS/GO32/ISS_TIM deps; OPL3 output redirected to `OPL3EMU_WriteReg`; timer stubbed to no-ops |
| 2e | `adt2play_sdl_audio.pas` | SDL audio callback driving `poll_proc`/`macro_poll_proc` |
| 3 | `txtscrio.pas`, `a2scrio.pas` | Removed DOS/GO32/BIOS deps; replaced `dosmemget`/`dosmemput` with software buffers; stubbed VGA hardware (palette, retrace, cursor) |
| 4 | `a2fileio.pas` | Removed `uses DOS`; replaced `GetFAttr`/`ReadOnly` with `FileMode := 0` |
| 5 | `adt2play.pas` | SDL event loop replacing `int 16h` keyboard; `SDL_PollEvent` instead of `inportb($60)`; `SysUtils.FindFirst`/`FindNext` for directory scanning; software text rendering via `font8x16` bitmaps; SDL video/audio init/teardown |
| 6 | *(testing)* | Fixed `FindFirst` skipping first match, initialization order bug, missing `replay_forbidden` guard in audio callback, type compatibility issues |

## Notes

- Inline asm blocks for pure math/copy operations are kept (i386 Win32 target).
- Text mode uses a software `text_screen_shadow` buffer + font8x8/font8x16 bitmaps.
- GFX 320×200 mode uses `vmem_320x200` software buffer blitted to the SDL surface.
- Audio is driven by the SDL callback (no separate timer thread), matching `at2/adt2opl3.pas`.
