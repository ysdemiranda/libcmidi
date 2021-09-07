#include "cmidi.h"

// MIDI Masks
const uint8_t MIDI_StatusMask =              0b10000000;
const uint8_t MIDI_DataMask =                0b01111111;
const uint8_t MIDI_MessageMask =             0b11110000;
const uint8_t MIDI_CV_ChannelMask =          0b00001111;

// Channel Voice Messages
static void (*MIDI_NoteOffISR)( MIDI_t* midi );
static void (*MIDI_NoteOnISR)( MIDI_t* midi );
static void (*MIDI_KeyPressureISR)( MIDI_t* midi );
static void (*MIDI_ControlChangeISR)( MIDI_t* midi );
static void (*MIDI_ProgramChangeISR)( MIDI_t* midi );
static void (*MIDI_ChannelPressureISR)( MIDI_t* midi );
static void (*MIDI_PitchBendISR)( MIDI_t* midi );

// System Common messages
static void (*MIDI_SysExISR)( MIDI_t* midi );
static void (*MIDI_TC_QFrameISR)( MIDI_t* midi );
static void (*MIDI_SongPositionISR)( MIDI_t* midi );
static void (*MIDI_SongSelectISR)( MIDI_t* midi );
static void (*MIDI_Undefined_0ISR)( MIDI_t* midi );
static void (*MIDI_Undefined_1ISR)( MIDI_t* midi );
static void (*MIDI_TuneRequestISR)( MIDI_t* midi );
static void (*MIDI_EndSysExISR)( MIDI_t* midi );

// System RT Messages
static void (*MIDI_TimingClockISR)( MIDI_t* midi );
static void (*MIDI_Undefined_2ISR)( MIDI_t* midi );
static void (*MIDI_StartISR)( MIDI_t* midi );
static void (*MIDI_ContinueISR)( MIDI_t* midi );
static void (*MIDI_StopISR)( MIDI_t* midi );
static void (*MIDI_Undefined_3ISR)( MIDI_t* midi );
static void (*MIDI_ActiveISR)( MIDI_t* midi );
static void (*MIDI_ResetISR)( MIDI_t* midi );

static volatile uint8_t rxBuf = 0;
static volatile uint8_t expect = 0;
static volatile MIDI_t* midiPtr;

static uint8_t MIDI_GetMessageLength(uint8_t status) {
    status &= MIDI_MessageMask;
    if(status < MIDI_CV_NOTE_OFF) {
        return 0;
    }

    switch(status) {
        case MIDI_CV_NOTE_OFF:
        case MIDI_CV_NOTE_ON:
        case MIDI_CV_KEY_PRESSURE:
        case MIDI_CV_CONTROL_CHANGE:
            return 2;
        case MIDI_CV_PROGRAM_CHANGE:
        case MIDI_CV_CHANNEL_PRESSURE:
            return 1;
        case MIDI_CV_PITCH_BEND:
            return 2;
        case MIDI_SC_SYSEX:
            return 0; // TODO Should be handled somewhere else
        case MIDI_SC_TCQF:
            return 1;
        case MIDI_SC_SONG_POSITION:
            return 2;
        case MIDI_SC_SONG_SELECT:
            return 1;
        default:
            return 0;
    }
}

static uint8_t MIDI_IsStatus(uint8_t data) {
    return (data & MIDI_StatusMask) ? 1 : 0;
}

static MIDI_StatusType_t MIDI_GetStatusType(uint8_t status) {
    if((status & MIDI_MessageMask) < MIDI_SC_SYSEX)
        status &= MIDI_MessageMask;
    
    if(status < MIDI_CV_NOTE_OFF) {
        return MIDIStatusType_NoStatus;
    }
    else if(status < MIDI_SC_SYSEX) {
        return MIDIStatusType_ChannelVoice;
    }
    else if(status < MIDI_RT_TIMING_CLOCK) {
        return MIDIStatusType_SystemCommon;
    }
    else {
        return MIDIStatusType_SystemRealTime;
    }
}

static void MIDI_ClearObject(MIDI_t* midi) {
    // Do not clear the Status byte as it
    // might be required for a continuated event.
    midi->message.data0 = 0;
    midi->message.data1 = 0;
    midi->state = MIDIState_Status;
}

static void StartMidiMessage() {
    midiPtr->message.status = rxBuf;
    midiPtr->message.data0 = 0;
    midiPtr->message.data1 = 0;
    expect = MIDI_GetMessageLength(rxBuf);
    if(expect) midiPtr->state = MIDIState_Data0;
    else midiPtr->state = MIDIState_Full;
}

static void MIDI_ISR(MIDI_t* midi) {
    //ISR was called too early
    if(midi->state != MIDIState_Full)
        return;
    
    if(MIDI_GetStatusType(midi->message.status) == MIDIStatusType_ChannelVoice) {
        if((midi->message.status & MIDI_CV_ChannelMask) != midi->channel) {
            // This message is not for me
            MIDI_ClearObject(midi);
            midi->message.status = 0;
            return;
        }
        else {
            midi->message.status &= MIDI_MessageMask;
        }
    }
    
    switch(midi->message.status) {
        case MIDI_CV_NOTE_OFF:
            if(MIDI_NoteOffISR) {
                MIDI_NoteOffISR(midi);
            }
            break;
        case MIDI_CV_NOTE_ON:
            if(MIDI_NoteOnISR) {
                MIDI_NoteOnISR(midi);
            }
            break;
        case MIDI_CV_KEY_PRESSURE:
            if(MIDI_KeyPressureISR) {
                MIDI_KeyPressureISR(midi);
            }
            break;
        case MIDI_CV_CONTROL_CHANGE:
            if(MIDI_ControlChangeISR) {
                MIDI_ControlChangeISR(midi);
            }
            break;
        case MIDI_CV_PROGRAM_CHANGE:
            if(MIDI_ProgramChangeISR) {
                MIDI_ProgramChangeISR(midi);
            }
            break;
        case MIDI_CV_CHANNEL_PRESSURE:
            if(MIDI_ChannelPressureISR) {
                MIDI_ChannelPressureISR(midi);
            }
            break;
        case MIDI_CV_PITCH_BEND:
            if(MIDI_PitchBendISR) {
                MIDI_PitchBendISR(midi);
            }
            break;
        case MIDI_SC_SYSEX:
            if(MIDI_SysExISR) {
                MIDI_SysExISR(midi);
            }
            break;
        case MIDI_SC_TCQF:
            if(MIDI_TC_QFrameISR) {
                MIDI_TC_QFrameISR(midi);
            }
            break;
        case MIDI_SC_SONG_POSITION:
            if(MIDI_SongPositionISR) {
                MIDI_SongPositionISR(midi);
            }
            break;
        case MIDI_SC_SONG_SELECT:
            if(MIDI_SongSelectISR) {
                MIDI_SongSelectISR(midi);
            }
            break;
        case MIDI_SC_UNDEFINED_0:
            if(MIDI_Undefined_0ISR) {
                MIDI_Undefined_0ISR(midi);
            }
            break;
        case MIDI_SC_UNDEFINED_1:
            if(MIDI_Undefined_1ISR) {
                MIDI_Undefined_1ISR(midi);
            }
            break;
        case MIDI_SC_TUNE_REQUEST:
            if(MIDI_TuneRequestISR) {
                MIDI_TuneRequestISR(midi);
            }
            break;
        case MIDI_SC_SYSEX_END:
            if(MIDI_EndSysExISR) {
                MIDI_EndSysExISR(midi);
            }
            break;
        case MIDI_RT_TIMING_CLOCK:
            if(MIDI_TimingClockISR) {
                MIDI_TimingClockISR(midi);
            }
            break;
        case MIDI_RT_UNDEFINED_2:
            if(MIDI_Undefined_2ISR) {
                MIDI_Undefined_2ISR(midi);
            }
            break;
        case MIDI_RT_START:
            if(MIDI_StartISR) {
                MIDI_StartISR(midi);
            }
            break;
        case MIDI_RT_CONTINUE:
            if(MIDI_ContinueISR) {
                MIDI_ContinueISR(midi);
            }
            break;
        case MIDI_RT_STOP:
            if(MIDI_StopISR) {
                MIDI_StopISR(midi);
            }
            break;
        case MIDI_RT_UNDEFINED_3:
            if(MIDI_Undefined_3ISR) {
                MIDI_Undefined_3ISR(midi);
            }
            break;
        case MIDI_RT_ACTIVE:
            if(MIDI_ActiveISR) {
                MIDI_ActiveISR(midi);
            }
            break;
        case MIDI_RT_RESET:
            if(MIDI_ResetISR) {
                MIDI_ResetISR(midi);
            }
            break;
        default:
            break;
    }
    
    MIDI_ClearObject(midi);
}

void MIDI_SetStatusHandler(uint8_t status, void (* handler)( MIDI_t* midi )) {
    switch(status) {
        case MIDI_CV_NOTE_OFF:
            MIDI_NoteOffISR = handler;
            break;
        case MIDI_CV_NOTE_ON:
            MIDI_NoteOnISR = handler;
            break;
        case MIDI_CV_KEY_PRESSURE:
            MIDI_KeyPressureISR = handler;
            break;
        case MIDI_CV_CONTROL_CHANGE:
            MIDI_ControlChangeISR = handler;
            break;
        case MIDI_CV_PROGRAM_CHANGE:
            MIDI_ProgramChangeISR = handler;
            break;
        case MIDI_CV_CHANNEL_PRESSURE:
            MIDI_ChannelPressureISR = handler;
            break;
        case MIDI_CV_PITCH_BEND:
            MIDI_PitchBendISR = handler;
            break;
        case MIDI_SC_SYSEX:
            MIDI_SysExISR = handler;
            break;
        case MIDI_SC_TCQF:
            MIDI_TC_QFrameISR = handler;
            break;
        case MIDI_SC_SONG_POSITION:
            MIDI_SongPositionISR = handler;
            break;
        case MIDI_SC_SONG_SELECT:
            MIDI_SongSelectISR = handler;
            break;
        case MIDI_SC_UNDEFINED_0:
            MIDI_Undefined_0ISR = handler;
            break;
        case MIDI_SC_UNDEFINED_1:
            MIDI_Undefined_1ISR = handler;
            break;
        case MIDI_SC_TUNE_REQUEST:
            MIDI_TuneRequestISR = handler;
            break;
        case MIDI_SC_SYSEX_END:
            MIDI_EndSysExISR = handler;
            break;
        case MIDI_RT_TIMING_CLOCK:
            MIDI_TimingClockISR = handler;
            break;
        case MIDI_RT_UNDEFINED_2:
            MIDI_Undefined_2ISR = handler;
            break;
        case MIDI_RT_START:
            MIDI_StartISR = handler;
            break;
        case MIDI_RT_CONTINUE:
            MIDI_ContinueISR = handler;
            break;
        case MIDI_RT_STOP:
            MIDI_StopISR = handler;
            break;
        case MIDI_RT_UNDEFINED_3:
            MIDI_Undefined_3ISR = handler;
            break;
        case MIDI_RT_ACTIVE:
            MIDI_ActiveISR = handler;
            break;
        case MIDI_RT_RESET:
            MIDI_ResetISR = handler;
            break;
        default:
            break;
    }
}

void MIDI_Task( uint8_t data, MIDI_t* midi) {
    rxBuf = data;
    midiPtr = midi;
    switch(midi->state) {
        case MIDIState_Status:
            if(MIDI_IsStatus(rxBuf)) {
                StartMidiMessage();
            }
            else {
                // Continuation
                midi->message.data0 = rxBuf;
                expect = MIDI_GetMessageLength(midi->message.status) - 1;
                if(expect) midi->state = MIDIState_Data1;
                else midi->state = MIDIState_Full;
            }
            break;
        case MIDIState_Data0:
            if(MIDI_IsStatus(rxBuf)) {
                // Missed bytes, oh well
                StartMidiMessage();
            }
            else {
                midi->message.data0 = rxBuf;
                expect--;
                if(expect) midi->state = MIDIState_Data1;
                else midi->state = MIDIState_Full;
            }
            break;
        case MIDIState_Data1:
            if(MIDI_IsStatus(rxBuf)) {
                // Missed bytes, oh well
                StartMidiMessage();
            }
            else {
                midi->message.data1 = rxBuf;
                expect = 0;
                midi->state = MIDIState_Full;
            }
            break;
        default:
            // This is loss
            break;
    }

    if(midi->state == MIDIState_Full) {
        MIDI_ISR(midi);
    }
}
