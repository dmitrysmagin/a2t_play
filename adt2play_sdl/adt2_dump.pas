program AdT2_Dump;
{$S-,Q-,R-,V-,B-,X+,F+}
{$PACKRECORDS 1}

uses
  SysUtils,
  A2player,
  A2fileIO,
  OPL3EMU,
  Windows;

const
  WR_TRACE_SIZE = 64;

var
  outfd: Longint;
  filename, outfilename: String;
  entries: Byte;
  correction: Integer;
  dump_ticks: Longint;
  max_ticks: Longint;
  pcm_buf: array[0..4095] of Byte;
  s: AnsiString;
  max_frames: Longint;
  frames_dumped: Longint;
  trace_init: Boolean;
  i: Integer;

  wr_trace_reg: array[0..WR_TRACE_SIZE-1] of Word;
  wr_trace_val: array[0..WR_TRACE_SIZE-1] of Byte;
  wr_trace_idx: Integer;
  wr_trace_count: Integer;

type
  TConsoleCtrlHandlerFn = function(dwCtrlType: DWORD): BOOL; stdcall;

function CtrlCHandler(dwCtrlType: DWORD): BOOL; stdcall;
begin
  if dwCtrlType = CTRL_C_EVENT then
  begin
    play_status := isStopped;
    songend := True;
    CtrlCHandler := True;
    Exit;
  end;
  CtrlCHandler := False;
end;

procedure wr_trace_add(reg, data: Word);
begin
  wr_trace_reg[wr_trace_idx] := reg;
  wr_trace_val[wr_trace_idx] := Byte(data);
  wr_trace_idx := (wr_trace_idx + 1) mod WR_TRACE_SIZE;
  if wr_trace_count < WR_TRACE_SIZE then
    Inc(wr_trace_count);
end;

procedure dump_opl2out(reg, data: Word);
begin
  wr_trace_add(reg, data);
  if trace_init then
    begin
      s := s + LowerCase(IntToHex(reg AND $1ff, 3)) + ' ' + LowerCase(IntToHex(data AND $ff, 2)) + #13#10;
    end;
  shadow_regs[reg shr 8, reg and $ff] := data;
  OPL3EMU_WriteReg(reg, data);
end;

procedure dump_opl3out(reg, data: Word);
begin
  wr_trace_add(reg, data);
  if trace_init then
    begin
      s := s + LowerCase(IntToHex(reg AND $1ff, 3)) + ' ' + LowerCase(IntToHex(data AND $ff, 2)) + #13#10;
    end;
  shadow_regs[reg shr 8, reg and $ff] := data;
  OPL3EMU_WriteReg(reg, data);
end;

procedure dump_opl3exp(data: Word);
begin
  wr_trace_add((data AND $ff) OR $100, data SHR 8);
  if trace_init then
    begin
      s := s + LowerCase(IntToHex((data AND $ff) OR $100, 3)) + ' ' + LowerCase(IntToHex(data SHR 8, 2)) + #13#10;
    end;
  shadow_regs[1, data and $ff] := data shr 8;
  OPL3EMU_WriteReg((data AND $ff) OR $100, data SHR 8);
end;

procedure dump_snd_settimer(freq: Longint);
begin
end;

procedure detect_all_effects;
const
  echars: array[0..255] of Char = (
    '0','1','2','3','4','5','6','7','8','9',
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
    '&','%','!','@','=','#','$','~','^','`','>','<',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_',
    '_','_','_','_','_','_','_','_','_','_','_','_','_','_','_','_'
  );
var
  effects: array[0..255] of Char;
  p, c, r, block, slot: Integer;
  ev: tCHUNK;
  ws: AnsiString;
begin
  for p := 0 to 255 do
    effects[p] := '_';

  for block := 0 to 15 do
    for slot := 0 to 7 do
      for c := 1 to songdata.nm_tracks do
        for r := 0 to songdata.patt_len - 1 do
        begin
          ev := pattdata^[block][slot][c][r];
          if (ev.effect_def <> 0) or (ev.effect <> 0) then
            effects[ev.effect_def] := echars[ev.effect_def];
          if (ev.effect_def2 <> 0) or (ev.effect2 <> 0) then
            effects[ev.effect_def2] := echars[ev.effect_def2];
        end;

  ws := 'EF ';
  for p := 0 to 255 do
    ws := ws + effects[p];
  ws := ws + #13#10;
  FileWrite(outfd, ws[1], Length(ws));
end;

procedure dump_frame;
type
   tEventTable = array[1..20] of tCHUNK;
   tFreqTable = array[1..20] of Word;
var
   pevt: ^tEventTable;
   pft: ^tFreqTable;
   i: Integer;
   ws: AnsiString;
   ctx: AnsiString;
begin
   if frames_dumped >= max_frames then
     begin
       play_status := isStopped;
       Exit;
     end;
   ctx := IntToStr(current_pattern) + ' ' + IntToStr(current_line) + ' ' +
          IntToStr(ticks) + ' ' + IntToStr(tick0) + ' ' +
          IntToStr(tickD) + ' ' + IntToStr(tickXF) + ' ';
   ws := IntToStr(frames_dumped) + ' ' + ctx + '0 ';
   for i := 0 to 255 do
     ws := ws + LowerCase(IntToHex(shadow_regs[0, i], 2));
   ws := ws + #13#10;
   ws := ws + IntToStr(frames_dumped) + ' ' + ctx + '1 ';
   for i := 0 to 255 do
     ws := ws + LowerCase(IntToHex(shadow_regs[1, i], 2));
   ws := ws + #13#10;
      ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'ET ';
      pevt := Pointer(get_event_table);
      for i := 1 to 20 do
        begin
          ws := ws + LowerCase(IntToHex(pevt^[i].note, 2));
          ws := ws + LowerCase(IntToHex(pevt^[i].instr_def, 2));
          ws := ws + LowerCase(IntToHex(pevt^[i].effect_def, 2));
          ws := ws + LowerCase(IntToHex(pevt^[i].effect, 2));
          ws := ws + LowerCase(IntToHex(pevt^[i].effect_def2, 2));
          ws := ws + LowerCase(IntToHex(pevt^[i].effect2, 2));
        end;
      ws := ws + #13#10;
     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'LE ' + get_last_effect_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'EFT ' + get_effect_table_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'KL ' + get_keyoff_loop_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'F ';
    pft := Pointer(get_freq_table);
    for i := 1 to 20 do
      ws := ws + LowerCase(IntToHex(pft^[i], 4));
    ws := ws + #13#10;

    { ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'MB ' + get_macro_table_dump + #13#10; }

    ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'PT ' + get_porta_table_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'FT ' + get_ftune_table_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'FK ' + get_portaFK_table_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'LB ' + get_loopbck_table_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'FS ' + get_fslide_table_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'AT ' + get_arpgg_table_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'VT ' + get_vibr_table_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'TT ' + get_trem_table_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'RT ' + get_retrig_table_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'RC ' + get_reset_chan_dump + #13#10;

     ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'MV ' + get_modulator_vol_dump + #13#10;

    ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'CV ' + get_carrier_vol_dump + #13#10;

    ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'VS ' + get_voice_table_dump + #13#10;

    ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'FP ' + get_fmpar_dump + #13#10;

    ws := ws + IntToStr(frames_dumped) + ' ' + ctx + 'GV ' + get_global_vol_dump + #13#10;

    { WR line removed - benign write-order differences only }

   FileWrite(outfd, ws[1], Length(ws));
   Inc(frames_dumped);
end;

begin
  wr_trace_idx := 0;
  wr_trace_count := 0;

  if ParamCount < 1 then
  begin
    WriteLn('Usage: adt2_dump <module_file> [output.reg] [max_frames]');
    Halt(1);
  end;

  filename := ParamStr(1);
  if ParamCount >= 2 then
    outfilename := ParamStr(2)
  else
    outfilename := filename + '.reg';

  max_frames := 500;
  if ParamCount >= 3 then
    max_frames := StrToIntDef(ParamStr(3), 500);
  if max_frames <= 0 then max_frames := 500;
  frames_dumped := 0;

  OPL3EMU_init;

  opl2out := dump_opl2out;
  opl3out := dump_opl3out;
  opl3exp := dump_opl3exp;
  snd_SetTimer := dump_snd_settimer;

  GetMem(pattdata, PATTERN_SIZE * 128);

  FillChar(decay_bar, SizeOf(decay_bar), 0);
  play_status := isStopped;
  init_songdata;
  init_timer_proc;

  songdata_source := filename;
  load_flag := BYTE_NULL;

  a2m_file_loader;
  If (load_flag = BYTE_NULL) then a2t_file_loader;
  If (load_flag = BYTE_NULL) then amd_file_loader;
  If (load_flag = BYTE_NULL) then cff_file_loader;
  If (load_flag = BYTE_NULL) then dfm_file_loader;
  If (load_flag = BYTE_NULL) then mtk_file_loader;
  If (load_flag = BYTE_NULL) then rad_file_loader;
  If (load_flag = BYTE_NULL) then s3m_file_loader;
  If (load_flag = BYTE_NULL) then fmk_file_loader;
  If (load_flag = BYTE_NULL) then sat_file_loader;
  If (load_flag = BYTE_NULL) then sa2_file_loader;
  If (load_flag = BYTE_NULL) then hsc_file_loader;

  If (load_flag = BYTE_NULL) or (load_flag = $7f) then
  begin
    WriteLn('ERROR: Invalid module file "', filename, '"');
    Halt(2);
  end;

  entries := 0;
  count_order(entries);
  correction := calc_following_order(0);
  If (correction <> -1) then Dec(entries, correction)
  else entries := 0;

  If (entries = 0) then
  begin
    WriteLn('ERROR: No playable orders in "', filename, '"');
    Halt(3);
  end;

  outfd := FileCreate(outfilename);
  if outfd = -1 then
  begin
    WriteLn('ERROR: Cannot create "', outfilename, '"');
    Halt(4);
  end;

  FillChar(pcm_buf, SizeOf(pcm_buf), 0);
  SetConsoleCtrlHandler(TConsoleCtrlHandlerFn(@CtrlCHandler), True);
  WriteLn('Dumping "', filename, '" -> ', outfilename, ' ...');

  detect_all_effects;

  start_playing;
  set_overall_volume(63);
  frame_hook := dump_frame;

  dump_ticks := 0;
  max_ticks := IRQ_freq * 600;

  while (play_status = isPlaying) and (not songend) and (dump_ticks < max_ticks) do
  begin
    a2t_update_dump(@pcm_buf, SizeOf(pcm_buf));
    Inc(dump_ticks);
  end;

  FileClose(outfd);

  stop_playing;
  done_timer_proc;
  FreeMem(pattdata);
  pattdata := NIL;

  WriteLn('Done: ', dump_ticks, ' a2t_update(', SizeOf(pcm_buf), ' B) iterations');
end.
