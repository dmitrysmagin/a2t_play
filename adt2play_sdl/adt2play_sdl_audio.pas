unit AdT2play_sdl_audio;
{$S-,Q-,R-,V-,B-,X+}
{$PACKRECORDS 1}
interface

procedure snd_init;
procedure snd_done;

implementation

uses
  SDL,SDL_Audio,
  A2player,
  OPL3EMU;

const
  sdl_sample_rate: Longint = 44100;
  sdl_sample_buffer: Longint = 2048;

var
  sample_frame_size: Longint;
  opl3_sample_buffer_ptr: Pointer;
  sdl_audio_spec: SDL_AudioSpec;

procedure play_callback_proc(var userdata; stream: pByte; len: Longint); cdecl;

const
  counter_idx: Longint = 0;

var
  counter: Longint;
  idx: Byte;
  dummy_chan_ptrs: array[1..18] of pDword;
  sample_offset: pDword;

begin
  For idx := 1 to 18 do
    dummy_chan_ptrs[idx] := opl3_sample_buffer_ptr;

  for counter := 0 to PRED(len DIV 4) do
    begin
      Inc(counter_idx);
      If (counter_idx >= sample_frame_size) then
        begin
          counter_idx := 0;
          timer_poll_proc;
        end;

      sample_offset := opl3_sample_buffer_ptr;
      Inc(sample_offset,counter);
      OPL3EMU_PollProc(sample_offset,dummy_chan_ptrs);
    end;

  Move(opl3_sample_buffer_ptr^,stream^,len);
end;

procedure snd_set_timer(freq: Longint);
begin
  sample_frame_size := sdl_sample_rate DIV freq;
end;

procedure snd_init;
begin
  GetMem(opl3_sample_buffer_ptr,sdl_sample_buffer*4);
  sample_frame_size := sdl_sample_rate DIV 50;
  snd_SetTimer := snd_set_timer;

  OPL3EMU_init;

  sdl_audio_spec.freq := sdl_sample_rate;
  sdl_audio_spec.format := AUDIO_S16;
  sdl_audio_spec.channels := 2;
  sdl_audio_spec.samples := sdl_sample_buffer;
  @sdl_audio_spec.callback := @play_callback_proc;
  sdl_audio_spec.userdata := NIL;

  If (SDL_OpenAudio(@sdl_audio_spec,NIL) < 0) then
    HALT(1);

  sdl_sample_rate := sdl_audio_spec.freq;
  sdl_sample_buffer := sdl_audio_spec.samples;

  SDL_PauseAudio(0);
end;

procedure snd_done;
begin
  SDL_PauseAudio(1);
  SDL_CloseAudio;
  FreeMem(opl3_sample_buffer_ptr);
  opl3_sample_buffer_ptr := NIL;
end;

end.
