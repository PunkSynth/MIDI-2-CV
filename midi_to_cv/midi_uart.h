/***********************************************
/ midi_uart.h : header file for the MIDI UART functions
/ Author: Patrik Källback - (c) 2023 PunkSynth
/ License: GPLv3
/***********************************************/

#ifndef MIDI_UART_H
#define MIDI_UART_H

////////////////////////////////////////////////////////////////////////////////
// MIDI DIN is connected to UART0
// MIDI USB is connected to UART1
////////////////////////////////////////////////////////////////////////////////

// The code below is to set up UART0 for receiving MIDI BYTES
#define UART_0 uart0
#define UART_1 uart1
#define BAUD_RATE_MIDI 31250
#define DATA_BITS_8 8
#define STOP_BITS_1 1
#define NO_PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART0_TX_PIN 0
#define UART0_RX_PIN 1

#define UART1_TX_PIN 4
#define UART1_RX_PIN 5

#define MIDI_CH_1 0x00
#define MIDI_CH_2 0x01
#define MIDI_CH_3 0x02
#define MIDI_CH_4 0x03
#define MIDI_CH_5 0x04
#define MIDI_CH_6 0x05
#define MIDI_CH_7 0x06
#define MIDI_CH_8 0x07
#define MIDI_CH_9 0x08
#define MIDI_CH_10 0x09
#define MIDI_CH_11 0x0A
#define MIDI_CH_12 0x0B
#define MIDI_CH_13 0x0C
#define MIDI_CH_14 0x0D
#define MIDI_CH_15 0x0E
#define MIDI_CH_16 0x0F
#define MIDI_CH_ALL 0x10

#define MIDI_CLK_UART0 0x100
#define MIDI_CLK_UART1 0x101
#define MIDI_CLK_INTERNAL 0x102

// Global char extern declaration
extern bool gLEDPinValue; // On board LED
extern int gMidiChUart0; // DIN MIDI
extern int gMidiChUart1; // USB MIDI
extern int gMidiClk; // MIDI clock source
extern int gHPWRange; // Half Pitch Wheel range

// (A callback is what an interrupt handler calls)
bool midi_clock_check_callback(repeating_timer_t *rt);
uint32_t bpm_to_ms(uint32_t bpm);
uint32_t bpm_to_us(uint32_t bpm);
void init_midi_clock_check(int bpm);

// Midi misc functions
void SetMidiChannel(int uartNo, int midiCh);
void SetClockSource(int clockSource);
void SetHalfPitchWheelRange(int noOfhalfNotes);

// Initiate MIDI interrupt
int init_uart0_for_MIDI_and_interrupt();
int init_uart1_for_MIDI_and_interrupt();

// This function is called by init_uart0_for_MIDI_and_interrupt()
// and init_uart1_for_MIDI_and_interrupt(). Both init interrupt use
// the same code so it is important to have it in one function.
int init_uartX_for_MIDI_and_interrupt(int uartNo);

// Interrupt handler for MIDI UART
static inline void on_uart0_rx_for_MIDI_intr_handler();
static inline void on_uart1_rx_for_MIDI_intr_handler();

// This function is called by on_uart0_rx_intr_handler()
// and on_uart1_rx_intr_handler(). Both handler use the
// same code so it is important to have it in one function.
static inline void uartX_rx_for_MIDI_intr_handler(int uartNo, int midiCh, uint8_t val);

static inline bool midi_note_off_callback(uint8_t midiCh, uint8_t noteNo, uint8_t velocity);
static inline bool midi_note_on_callback(uint8_t midiCh, uint8_t noteNo, uint8_t velocity);
static inline bool polyphonic_aftertouch_callback(uint8_t midiCh, uint8_t noteNo, uint8_t pressure);
static inline bool control_change_callback(uint8_t midiCh, uint8_t controlNo, uint8_t data);
static inline bool program_change_callback(uint8_t midiCh, uint8_t programNo, uint8_t unused);
static inline bool channel_aftertouch_callback(uint8_t midiCh, uint8_t pressure, uint8_t unused);
static inline bool pitch_wheel_callback(uint8_t midiCh, uint8_t lsb, uint8_t msb);
static inline bool sysExStart_callback(uint8_t manufID, uint8_t data);
static inline bool quarterFrame_callback(uint8_t data);
static inline bool songPointer_callback(uint8_t lsb, uint8_t msb);
static inline bool songSelect_callback(uint8_t songNo);
static inline bool measureEnd_callback(uint8_t unused);
static inline bool sys_msg_callback(uint8_t sys);

// Old stuff !!!
void old_on_uart0_rx_intr_handler();
void old_on_uart1_rx_intr_handler();
static inline uint8_t hex_from_nibble(uint8_t nb);

#endif // MIDI_UART_H

