unit SDLaudio;
{$PACKRECORDS 4}

interface

const
  LibName = 'SDL.dll';
  SDL_INIT_AUDIO = $00000010;
  AUDIO_S16 = $8010; // Signed 16-bit samples

type
  UInt8 = Byte;
  PUInt8 = ^UInt8;
  UInt16 = word;
  UInt32 = Cardinal;
  PSDL_AudioSpec = ^TSDL_AudioSpec;
  TSDL_AudioSpec = record
    freq: Integer; // DSP frequency -- samples per second
    format: UInt16; // Audio data format
    channels: UInt8; // Number of channels: 1 mono, 2 stereo
    silence: UInt8; // Audio buffer silence value (calculated)
    samples: UInt16; // Audio buffer size in samples
    padding: UInt16; // Necessary for some compile environments
    size: UInt32; // Audio buffer size in bytes (calculated)
    { This function is called when the audio device needs more data.
      'stream' is a pointer to the audio data buffer
      'len' is the length of that buffer in bytes.
      Once the callback returns, the buffer will no longer be valid.
      Stereo samples are stored in a LRLRLR ordering.}
    callback: procedure(userdata: Pointer; stream: PUInt8; len: Integer);
    cdecl;
    userdata: Pointer;
  end;

function SDL_InitSubSystem(flags: UInt32): Integer; cdecl; external LibName;
function SDL_GetError: PChar; cdecl; external LibName;
procedure SDL_Quit; cdecl; external LibName;
procedure SDL_Delay(msec: UInt32); cdecl; external LibName;
function SDL_OpenAudio(desired, obtained: PSDL_AudioSpec): Integer; cdecl;external LibName;
procedure SDL_PauseAudio(pause_on: Integer); cdecl; external LibName;
procedure SDL_CloseAudio; cdecl; external LibName;

implementation

end.