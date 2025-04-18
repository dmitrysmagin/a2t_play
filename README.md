# A2M/A2T player with Nuked OPL3 emulator

This is a player of Adlib Tracker 2 music formats A2M/A2T versions 1 to 14 which is written in plain C and uses [Nuked OPL3 Emulator](https://github.com/nukeykt/Nuked-OPL3) for maximum authentic sound.

Original [Adlib Tracker 2](https://web.archive.org/web/20241208054547/http://adlibtracker.net/) was written in TMT Pascal and later rewritten in [FreePascal](https://github.com/ijsf/at2). Porting Pascal to C took some considerable time and effort, so bugs are still possible.

## What about Adplug?

[Adplug](https://adplug.github.io/) is not forgotten. The code was [committed](https://github.com/adplug/adplug/blob/master/src/a2m-v2.cpp) into Adplug and included into the latest Adplug [release 2.4](https://github.com/adplug/adplug/releases/tag/adplug-2.4). Applications that use Adplug 2.4 and later should already have support for all versions of A2M/A2T modules.

[DeadBeef v1.9.6](https://deadbeef.sourceforge.io/) is a good example of a player which includes the latest git adplug, so it does support A2M/A2T v9-14.

Foobar's [AdPlug 1.56](https://www.foobar2000.org/components/view/foo_input_adplug) still has no support for A2M v9-14 (somebody, poke the devs to update it).
