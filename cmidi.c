#include "cmidi.h"

// MIDI Masks
const uint8_t MIDI_StatusMask =              0b10000000;
const uint8_t MIDI_DataMask =                0b01111111;
const uint8_t MIDI_MessageMask =             0b11110000;
const uint8_t MIDI_CV_ChannelMask =          0b00001111;

// Channel Voice Messages
static void (*MIDI_NoteOffHandler)( MIDI_t* midi );
static void (*MIDI_NoteOnHandler)( MIDI_t* midi );
static void (*MIDI_KeyPressureHandler)( MIDI_t* midi );
static void (*MIDI_ControlChangeHandler)( MIDI_t* midi );
static void (*MIDI_ProgramChangeHandler)( MIDI_t* midi );
static void (*MIDI_ChannelPressureHandler)( MIDI_t* midi );
static void (*MIDI_PitchBendHandler)( MIDI_t* midi );

// System Common messages
static void (*MIDI_SysExHandler)( MIDI_t* midi );
static void (*MIDI_TC_QFrameHandler)( MIDI_t* midi );
static void (*MIDI_SongPositionHandler)( MIDI_t* midi );
static void (*MIDI_SongSelectHandler)( MIDI_t* midi );
static void (*MIDI_Undefined_0Handler)( MIDI_t* midi );
static void (*MIDI_Undefined_1Handler)( MIDI_t* midi );
static void (*MIDI_TuneRequestHandler)( MIDI_t* midi );
static void (*MIDI_EndSysExHandler)( MIDI_t* midi );

// System RT Messages
static void (*MIDI_TimingClockHandler)( MIDI_t* midi );
static void (*MIDI_Undefined_2Handler)( MIDI_t* midi );
static void (*MIDI_StartHandler)( MIDI_t* midi );
static void (*MIDI_ContinueHandler)( MIDI_t* midi );
static void (*MIDI_StopHandler)( MIDI_t* midi );
static void (*MIDI_Undefined_3Handler)( MIDI_t* midi );
static void (*MIDI_ActiveHandler)( MIDI_t* midi );
static void (*MIDI_ResetHandler)( MIDI_t* midi );

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

static void MIDI_Handler(MIDI_t* midi) {
    //Handler was called too early
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
            if(MIDI_NoteOffHandler) {
                MIDI_NoteOffHandler(midi);
            }
            break;
        case MIDI_CV_NOTE_ON:
            if(MIDI_NoteOnHandler) {
                MIDI_NoteOnHandler(midi);
            }
            break;
        case MIDI_CV_KEY_PRESSURE:
            if(MIDI_KeyPressureHandler) {
                MIDI_KeyPressureHandler(midi);
            }
            break;
        case MIDI_CV_CONTROL_CHANGE:
            if(MIDI_ControlChangeHandler) {
                MIDI_ControlChangeHandler(midi);
            }
            break;
        case MIDI_CV_PROGRAM_CHANGE:
            if(MIDI_ProgramChangeHandler) {
                MIDI_ProgramChangeHandler(midi);
            }
            break;
        case MIDI_CV_CHANNEL_PRESSURE:
            if(MIDI_ChannelPressureHandler) {
                MIDI_ChannelPressureHandler(midi);
            }
            break;
        case MIDI_CV_PITCH_BEND:
            if(MIDI_PitchBendHandler) {
                MIDI_PitchBendHandler(midi);
            }
            break;
        case MIDI_SC_SYSEX:
            if(MIDI_SysExHandler) {
                MIDI_SysExHandler(midi);
            }
            break;
        case MIDI_SC_TCQF:
            if(MIDI_TC_QFrameHandler) {
                MIDI_TC_QFrameHandler(midi);
            }
            break;
        case MIDI_SC_SONG_POSITION:
            if(MIDI_SongPositionHandler) {
                MIDI_SongPositionHandler(midi);
            }
            break;
        case MIDI_SC_SONG_SELECT:
            if(MIDI_SongSelectHandler) {
                MIDI_SongSelectHandler(midi);
            }
            break;
        case MIDI_SC_UNDEFINED_0:
            if(MIDI_Undefined_0Handler) {
                MIDI_Undefined_0Handler(midi);
            }
            break;
        case MIDI_SC_UNDEFINED_1:
            if(MIDI_Undefined_1Handler) {
                MIDI_Undefined_1Handler(midi);
            }
            break;
        case MIDI_SC_TUNE_REQUEST:
            if(MIDI_TuneRequestHandler) {
                MIDI_TuneRequestHandler(midi);
            }
            break;
        case MIDI_SC_SYSEX_END:
            if(MIDI_EndSysExHandler) {
                MIDI_EndSysExHandler(midi);
            }
            break;
        case MIDI_RT_TIMING_CLOCK:
            if(MIDI_TimingClockHandler) {
                MIDI_TimingClockHandler(midi);
            }
            break;
        case MIDI_RT_UNDEFINED_2:
            if(MIDI_Undefined_2Handler) {
                MIDI_Undefined_2Handler(midi);
            }
            break;
        case MIDI_RT_START:
            if(MIDI_StartHandler) {
                MIDI_StartHandler(midi);
            }
            break;
        case MIDI_RT_CONTINUE:
            if(MIDI_ContinueHandler) {
                MIDI_ContinueHandler(midi);
            }
            break;
        case MIDI_RT_STOP:
            if(MIDI_StopHandler) {
                MIDI_StopHandler(midi);
            }
            break;
        case MIDI_RT_UNDEFINED_3:
            if(MIDI_Undefined_3Handler) {
                MIDI_Undefined_3Handler(midi);
            }
            break;
        case MIDI_RT_ACTIVE:
            if(MIDI_ActiveHandler) {
                MIDI_ActiveHandler(midi);
            }
            break;
        case MIDI_RT_RESET:
            if(MIDI_ResetHandler) {
                MIDI_ResetHandler(midi);
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
            MIDI_NoteOffHandler = handler;
            break;
        case MIDI_CV_NOTE_ON:
            MIDI_NoteOnHandler = handler;
            break;
        case MIDI_CV_KEY_PRESSURE:
            MIDI_KeyPressureHandler = handler;
            break;
        case MIDI_CV_CONTROL_CHANGE:
            MIDI_ControlChangeHandler = handler;
            break;
        case MIDI_CV_PROGRAM_CHANGE:
            MIDI_ProgramChangeHandler = handler;
            break;
        case MIDI_CV_CHANNEL_PRESSURE:
            MIDI_ChannelPressureHandler = handler;
            break;
        case MIDI_CV_PITCH_BEND:
            MIDI_PitchBendHandler = handler;
            break;
        case MIDI_SC_SYSEX:
            MIDI_SysExHandler = handler;
            break;
        case MIDI_SC_TCQF:
            MIDI_TC_QFrameHandler = handler;
            break;
        case MIDI_SC_SONG_POSITION:
            MIDI_SongPositionHandler = handler;
            break;
        case MIDI_SC_SONG_SELECT:
            MIDI_SongSelectHandler = handler;
            break;
        case MIDI_SC_UNDEFINED_0:
            MIDI_Undefined_0Handler = handler;
            break;
        case MIDI_SC_UNDEFINED_1:
            MIDI_Undefined_1Handler = handler;
            break;
        case MIDI_SC_TUNE_REQUEST:
            MIDI_TuneRequestHandler = handler;
            break;
        case MIDI_SC_SYSEX_END:
            MIDI_EndSysExHandler = handler;
            break;
        case MIDI_RT_TIMING_CLOCK:
            MIDI_TimingClockHandler = handler;
            break;
        case MIDI_RT_UNDEFINED_2:
            MIDI_Undefined_2Handler = handler;
            break;
        case MIDI_RT_START:
            MIDI_StartHandler = handler;
            break;
        case MIDI_RT_CONTINUE:
            MIDI_ContinueHandler = handler;
            break;
        case MIDI_RT_STOP:
            MIDI_StopHandler = handler;
            break;
        case MIDI_RT_UNDEFINED_3:
            MIDI_Undefined_3Handler = handler;
            break;
        case MIDI_RT_ACTIVE:
            MIDI_ActiveHandler = handler;
            break;
        case MIDI_RT_RESET:
            MIDI_ResetHandler = handler;
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
        MIDI_Handler(midi);
    }
}
