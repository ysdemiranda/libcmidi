#ifndef MIDI_H
#define	MIDI_H

// Channel Voice Messages
#define MIDI_CV_NOTE_OFF			0b10000000
#define MIDI_CV_NOTE_ON 			0b10010000
#define MIDI_CV_KEY_PRESSURE		0b10100000
#define MIDI_CV_CONTROL_CHANGE		0b10110000
#define MIDI_CV_PROGRAM_CHANGE		0b11000000
#define MIDI_CV_CHANNEL_PRESSURE	0b11010000
#define MIDI_CV_PITCH_BEND			0b11100000

// System Common Messages
#define MIDI_SC_SYSEX				0b11110000
#define MIDI_SC_TCQF				0b11110001
#define MIDI_SC_SONG_POSITION		0b11110010
#define MIDI_SC_SONG_SELECT			0b11110011
#define MIDI_SC_UNDEFINED_0			0b11110100
#define MIDI_SC_UNDEFINED_1			0b11110101
#define MIDI_SC_TUNE_REQUEST		0b11110110
#define MIDI_SC_SYSEX_END			0b11110111

// System Real-time Messages
#define MIDI_RT_TIMING_CLOCK		0b11111000
#define MIDI_RT_UNDEFINED_2			0b11111001
#define MIDI_RT_START				0b11111010
#define MIDI_RT_CONTINUE			0b11111011
#define MIDI_RT_STOP				0b11111100
#define MIDI_RT_UNDEFINED_3			0b11111101
#define MIDI_RT_ACTIVE				0b11111110
#define MIDI_RT_RESET				0b11111111

#include <stdint.h>
#include <stdbool.h>

#ifdef	__cplusplus
extern "C" {
#endif
    
    typedef struct {
        uint8_t status;
        uint8_t data0;
        uint8_t data1;
    } MIDI_Message_t;
    
    typedef enum {
        MIDIState_Status,
        MIDIState_Data0,
        MIDIState_Data1,
        MIDIState_Full
    } MIDI_State_t;
    
    typedef struct {
        MIDI_State_t state;
        MIDI_Message_t message;
        uint8_t channel;
    } MIDI_t;

    typedef enum {
        MIDIStatusType_NoStatus,
        MIDIStatusType_ChannelVoice,
        MIDIStatusType_SystemCommon,
        MIDIStatusType_SystemRealTime
    } MIDI_StatusType_t;
    
    // Macro's
    void MIDI_SetStatusHandler(uint8_t status, void (* handler)( MIDI_t* midi ));

    // ISRs and Tasks
    void MIDI_Task( uint8_t data, MIDI_t* midi );
    
#ifdef	__cplusplus
}
#endif

#endif	/* MIDI_H */

