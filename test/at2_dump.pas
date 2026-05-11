program at2_dump;
{$MODE TP}
{$PACKRECORDS 1}
{$H+}

{$IFDEF GO32V2}
{$FATAL This program requires non-DOS target}
{$ENDIF}

{$DEFINE AT2_DUMP}

uses
{$IFDEF AT2_DUMP}
  SysUtils,
{$ENDIF}
{$IFDEF GO32V2}
  CRT, GO32,
{$ELSE}
  SDL_Types, SDL_Timer, SDL_Audio,
{$ENDIF}
  AdT2sys, AdT2keyb, AdT2opl3, AdT2unit, AdT2extn, AdT2ext2, AdT2ext3, AdT2ext4,
  AdT2ext5, AdT2text, AdT2data, AdT2pack,
  StringIO, DialogIO, ParserIO, TxtScrIO, MenuLib1, MenuLib2,
{$IFNDEF GO32V2}
  OPL3EMU,
{$ENDIF}
  DepackIO;

{$IFDEF AT2_DUMP}

var
  songend: Boolean;

{$IFNDEF GO32V2}
procedure OPL3EMU_DumpInit; forward;
procedure OPL3EMU_DumpDone; forward;
procedure OPL3EMU_DumpWriteReg(reg: Word; data: Byte); forward;
procedure OPL3EMU_DumpPollProc(p_data: pDword; var ch_table); forward;

var
  dump_opl3out_proc: tOPL3OUT_proc;
{$ENDIF}

procedure at2t_update(buf: Pointer; len: Longint);
var
  counter: Longint;
  counter_idx: Longint;
  IRQ_freq_val: Longint;
begin
  if not rewind then
    IRQ_freq_val := IRQ_freq
  else
    IRQ_freq_val := IRQ_freq * 20;

  for counter := 0 to pred(len div 4) do
  begin
    Inc(counter_idx);
    if counter_idx >= (49716 div 50) then
    begin
      counter_idx := 0;
      if (ticklooper > 0) then
        if fast_forward or rewind then
          if not replay_forbidden then
            poll_proc
          else
        else if not replay_forbidden then
          poll_proc;

      if macro_ticklooper = 0 then
        macro_poll_proc;

      Inc(ticklooper);
      if ticklooper >= IRQ_freq_val div tempo then
        ticklooper := 0;

      Inc(macro_ticklooper);
      if macro_ticklooper >= IRQ_freq_val div (tempo * macro_speedup) then
        macro_ticklooper := 0;
    end;

    if play_status = isStopped then
    begin
      songend := TRUE;
      EXIT;
    end;
  end;
end;

function basename_no_ext(const path: string): string;
var
  p, q: integer;
begin
  p := Length(path);
  while (p > 0) and not (path[p] in ['/', '\']) do
    Dec(p);
  if p > 0 then
    Inc(p)
  else
    p := 1;
  q := p;
  while (q <= Length(path)) and (path[q] <> '.') do
    Inc(q);
  basename_no_ext := Copy(path, p, q - p);
end;

function LoadA2M(const filename: string): Boolean;
var
  f: file;
  fsize: Longint;
  buf: pByte;
  magic: array[0..15] of char;
  i: integer;
begin
  LoadA2M := FALSE;

  Assign(f, filename);
  {$i-}
  Reset(f, 1);
  {$i-}
  if IOResult <> 0 then
    Exit;

  fsize := FileSize(f);
  GetMem(buf, fsize);
  BlockRead(f, buf^, fsize);
  Close(f);
  {$i+}

  magic := '';
  for i := 0 to 9 do
    magic[i] := chr(buf[i]);
  magic[10] := #0;

  if StrLComp(@magic, '_A2module_', 10) = 0 then
  begin
    ProcessA2MBuffer(buf, fsize);
    LoadA2M := TRUE;
  end
  else if StrLComp(@magic, '_a2module_', 10) = 0 then
  begin
    ProcessA2MBuffer(buf, fsize);
    LoadA2M := TRUE;
  end
  else if (fsize > 15) and
     ((StrLComp(@buf[0], '_A2tiny_module_', 15) = 0) or
      (StrLComp(@buf[0], '_a2tiny_module_', 15) = 0)) then
  begin
    ProcessA2MBuffer(buf, fsize);
    LoadA2M := TRUE;
  end
  else
    FreeMem(buf);

  tempo := init_tempo;
  speed := init_speed;
end;

procedure InitPlay;
begin
  play_status := isStopped;
  replay_forbidden := TRUE;
  init_player;
  init_songdata;

  FillChar(channel_flag, SizeOf(channel_flag), TRUE);
  current_octave := default_octave;
end;

procedure StartPlay;
begin
  stop_playing;
  init_player;
  current_order := 0;
  current_pattern := songdata.pattern_order[0];
  current_line := 0;
  pattern_break := FALSE;
  pattern_delay := FALSE;
  tickXF := 0;
  ticks := 0;
  next_line := 0;
  play_status := isPlaying;
  replay_forbidden := FALSE;
  ticklooper := 0;
  macro_ticklooper := 0;
  songend := FALSE;
  tempo := songdata.tempo;
  speed := songdata.speed;
  macro_speedup := songdata.macro_speedup;
  if macro_speedup = 0 then
    macro_speedup := 1;
  update_timer(tempo);
end;

procedure StopPlay;
begin
  stop_playing;
  done_timer_proc;
end;

procedure ShowUsage;
begin
  WriteLn('Usage: at2_dump <file.a2m>');
  WriteLn('Dumps OPL3 register writes from an A2M file');
  WriteLn('Output format: "REG DATA" per line (e.g., "004 0f")');
end;

var
  input_file: string;
  outname: string;
  buf: array[0..4095] of byte;
  res: boolean;
{$ENDIF}
begin
{$IFDEF AT2_DUMP}
  if ParamCount < 1 then
  begin
    ShowUsage;
    Halt(1);
  end;

  input_file := ParamStr(1);

  if not FileExists(input_file) then
  begin
    WriteLn('Error: File not found: ', input_file);
    Halt(1);
  end;

  outname := basename_no_ext(input_file) + '.reg';

  if not freopen(outname, 'w', stdout) then
  begin
    WriteLn('Error: Failed to open output file: ', outname);
    Halt(1);
  end;

  InitPlay;

  {$IFNDEF GO32V2}
  OPL3EMU_DumpInit;
  opl3out := OPL3EMU_DumpWriteReg;
  {$ENDIF}

  res := LoadA2M(input_file);
  if not res then
  begin
    {$IFNDEF GO32V2}
    OPL3EMU_DumpDone;
    {$ENDIF}
    WriteLn('Error: Failed to load A2M file: ', input_file);
    Halt(1);
  end;

  StartPlay;

  FillChar(buf, SizeOf(buf), 0);
  while (play_status = isPlaying) and not songend do
    at2t_update(@buf, SizeOf(buf));

  StopPlay;

  {$IFNDEF GO32V2}
  OPL3EMU_DumpDone;
  {$ENDIF}

{$ELSE}
  WriteLn('AT2 player unit - define AT2_DUMP to build dump utility');
{$ENDIF}
end.
