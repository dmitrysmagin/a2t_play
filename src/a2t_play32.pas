uses ymf262,sdlaudio,a2replay,crt;

const
  freqhz=44100;
  framesmpl:word=freqhz div 50;
  buffsmpl=4096;

var
  f:file;
  thesong:array[0..1048575] of byte;
  irq_freq:word;
  factor:word;
  datapos:integer;
  maxdata:integer;
  cnt,cntr:integer;
  ym,temp:integer;
  auddesired:tsdl_audiospec;
  ticklooper,
  macro_ticklooper: Longint;
  buf:array[0..buffsmpl*2] of dword;
  wavwriter:boolean=false;

procedure playcallback( userdata : Pointer; stream : PUint8; len : integer);cdecl;
begin
  for cntr:=0 to (buffsmpl-1) do
  begin
    inc(cnt);
    if (cnt>=framesmpl) then
    begin
      cnt:=0;
      If (ticklooper = 0) then
        begin
          poll_proc;
          if irq_freq<>tempo*macro_speedup then
          begin
            irq_freq:=tempo*macro_speedup;
            if tempo=18 then irq_freq:=0;
            if irq_freq=0 {and timer_fix} then irq_freq:= TRUNC((18.2)*20);
            framesmpl:=freqhz div irq_freq;
          end;       
        end;  

      If (macro_ticklooper = 0) then
        macro_poll_proc;

      Inc(ticklooper);
      If (ticklooper >= IRQ_freq DIV tempo) then ticklooper := 0;

      Inc(macro_ticklooper);
      If (macro_ticklooper >= IRQ_freq DIV (tempo*macro_speedup)) then macro_ticklooper := 0;
    end;
    ymf262updateone(ym,@buf[cntr],1);
  end;
  move(buf[0],stream^,len);
  if wavwriter then blockwrite(f,buf[0],len);
end;

procedure myoplout(port,val:word);
begin
  ymf262write(ym,port,val);
end;

procedure fwrited(d:dword);inline;
begin
  blockwrite(f,d,4);
end;

procedure fwritew(w:word);inline;
begin
  blockwrite(f,w,2);
end;

procedure nullout(port,val:word);begin;end;

begin
{$IFDEF WINDOWS}
  writeLn('Little A2Tiny Player b20061014');
  writeLn('Code: Player by subz3ro; Windows Port by Da!NyL');
  writeLN('      OPL3-Emulator by Jarek Burczynski');
{$ELSE}
  writeLn('Little A2Tiny Player');
  writeLn('Code: Player by subz3ro; Linux Port by Da!NyL');
  writeLN('      OPL3-Emulator by Jarek Burczynski');
{$ENDIF}
  if not(paramcount in [1..2]) then
  begin
    writeLn('Syntax: a2t_play myfile.a2t            : plays myfile.a2t');
    writeLn('        a2t_play myfile.a2t output.wav : record myfile.a2t to output.wav');
    exit;
  end;
  assign(f,paramstr(1));reset(f,1);blockread(f,thesong,1048576,temp);close(f);
  if paramcount=2 then
  begin
    assign(f,paramstr(2));rewrite(f,1);
    wavwriter:=true;
    fwrited($46464952); // "RIFF"
    fwrited($00000000); // file length - 8 | POS=$04
    fwrited($45564157); // "WAVE"
    fwrited($20746D66); // "fmt "
    fwrited($00000010); // 16 bytes fmt data
    fwritew(    $0001); // PCM
    fwritew(    $0002); // Stereo
    fwrited(   freqhz); // Samples per second
    fwrited(freqhz* 4); // Bytes/Second= Sample rate * block align
    fwritew(    $0004); // block align = channels * bits/sample / 8
    fwritew(    $0010); // 16 bit
    fwrited($61746164); // "data"
    fwrited($00000000); // file length - 44 | POS=$28
  end;
  opl_out:=myoplout;
  ym:=ymf262init(1,OPL3_INTERNAL_FREQ,freqhz);
  ymf262resetchip(ym);
  start_playing(thesong,temp);
  If (error_code <> 0) then
    begin
      Case error_code of
        -1: WriteLn('Incorrect module data!');
        else
            WriteLn('Insufficient memory!');
      end;
      EXIT;
    end;

  auddesired.freq:=freqhz;
  auddesired.format:=audio_s16;
  auddesired.channels:=2;
  auddesired.samples:=buffsmpl;
  auddesired.callback:=@playcallback;
  auddesired.userdata:=nil;
  if (SDL_Openaudio(@auddesired,nil)<0) then
  begin
    writeLn('?Audio: '+ SDL_GetError+'  ERROR');
    halt(254);
  end;
  datapos:=0;cnt:=1;factor:=1;irq_freq:=50;
  if wavwriter then writeLn('Recording to ',paramstr(2));
  sdl_pauseaudio(0);
  writeLN('Playing - press anything to exit');
  while not keypressed do
  begin
    Write('Order ',current_order:3,', ','Pattern ',current_pattern:3,', ','Row ',current_line:3,#13);
    sdl_delay(20);
  end;
  readkey;writeLn;
  if wavwriter then
  begin
    wavwriter:=false;
    seek(f,$04);fwrited(filesize(f)-8);
    seek(f,$28);fwrited(filesize(f)-44);
    close(f);
    writeLN('Recording finished');
  end;
  stop_playing;opl_out:=nullout;
  sdl_pauseaudio(1);
  sdl_closeaudio;
  ymf262shutdown;
end.
