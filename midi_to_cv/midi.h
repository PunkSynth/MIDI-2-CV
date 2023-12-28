/***********************************************
/ midi.h : header file for the MIDI functions
/ Author: Patrik Källback - (c) 2023 PunkSynth
/ License: GPLv3
/***********************************************/

#ifndef MIDI_H
#define MIDI_H

////////////////////////////////////////////////////////////////////////////////
// The midi.h is based on: 
// The Complete MIDI 1.0
// Detailed Specification
// Incorporating all Recommended Practices
// document version 96.1
// third edition
// https://www.midi.org/downloads?task=callelement&format=raw&item_id=92&element=f85c494b-2b32-4109-b8c1-083cca2b7db6&method=download
////////////////////////////////////////////////////////////////////////////////

// The n in the status byte is the MIDI Channel Number (0-F)
// 0 is equalt to MIDI Channel Number 1
// 1 is equalt to MIDI Channel Number 2
// ... is equalt to MIDI Channel Number ...
// F is equalt to MIDI Channel Number 16

enum midiStatus {
    noteOff = 0, // Status: 8n, Data1: Note Number, Data2: Velocity
    noteOn, // Status: 9n, Data1: Note Number, Data2: Velocity
    polyAfter, // Status: An, Data1: Note Number, Data2: Pressure
    controlChange, // Status: Bn, Data1: Control Number, Data2: Data
    programChange, // Status: Cn, Data1: Program Number, Data2: Unused
    chanAfter, // Status: Dn, Data1: Pressure, Data2: Unused
    pitchWheel, // Status: En, Data1: LSB, Data2: MSB
    sysExStart, // Status: F0, Data1: Manufacturers ID, Data2: Data, data, data
    sysExEnd, // Status: F7, no data, no data
    songPointer, // Status: F2, Data1: LSB, Data2: MSB
    songSelect, // Status: F3, Data1: Song Number, no data
    tuneRequest, // Status: F6, no data, no data
    quarterFrame, // Status: F1, Data1: data, no data
    timingClock, // Status: F8, no data, no data
    measureEnd, // Status: F9, Data1: unused, no data
    start, // Status: FA, no data, no data
    cont, // Status: FB, no data, no data
    stop, // Status: FC, no data, no data
    activeSensing, // Status: FE, no data, no data
    reset, // Status: FF, no data, no data
};



#endif // MIDI_H
