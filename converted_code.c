if (event[chan].note == (event[chan].note | keyoff_flag)) {
    key_off(chan);
} else if (!is_tone_portamento_effect(LO(effect_table[chan])) &&
           !is_tone_portamento_effect(LO(effect_table2[chan]))) {
    if (!is_no_restart_case(event, chan)) {
        output_note(event[chan].note, voice_table[chan], chan, TRUE, TRUE);
    } else {
        output_note(event[chan].note, voice_table[chan], chan, TRUE, FALSE);
    }
} else if (event[chan].note != 0 && tporta_flag &&
           event_table[chan].note == (event_table[chan].note | keyoff_flag)) {
    output_note(event_table[chan].note & ~keyoff_flag, voice_table[chan], chan, FALSE, TRUE);
} else if (event[chan].note != 0) {
    if (portaFK_table[chan] && tporta_flag) {
        output_note(event[chan].note, event[chan].instr_def, chan, FALSE, TRUE);
    }
}

static bool is_tone_portamento_effect(uint8_t effect) {
    return effect == ef_TonePortamento ||
           effect == ef_TPortamVolSlide ||
           effect == ef_TPortamVSlideFine ||
           effect == (ef_extended2 + ef_fix2 + ef_ex2_NoteDelay);
}

static bool is_no_restart_case(Event* event, int chan) {
    return (((event[chan].effect_def2 == ef_SwapArpeggio ||
             event[chan].effect_def2 == ef_SwapVibrato) &&
            event[chan].effect_def == ef_Extended &&
            (event[chan].effect / 16) == ef_ex_ExtendedCmd2 &&
            (event[chan].effect % 16) == ef_ex_cmd2_NoRestart) ||
           ((event[chan].effect_def == ef_SwapArpeggio ||
             event[chan].effect_def == ef_SwapVibrato) &&
            event[chan].effect_def2 == ef_Extended &&
            (event[chan].effect2 / 16) == ef_ex_ExtendedCmd2 &&
            (event[chan].effect2 % 16) == ef_ex_cmd2_NoRestart));
}