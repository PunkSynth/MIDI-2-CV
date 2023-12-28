/***********************************************
/ midi_uart.c : implementation file for the MIDI UART functions
/ Author: Patrik Källback - (c) 2023 PunkSynth
/ License: GPLv3
/***********************************************/

#include <stdio.h>
#include "main.h"
#include "midi_uart.h"
#include "error_list.h"
#include "midi.h"
#include "mcp4725.h"

// Global char initiation
bool gLEDPinValue = true; // On board LED
int gMidiChUart0 = MIDI_CH_1; // DIN MIDI
int gMidiChUart1 = MIDI_CH_1; // USB MIDI
int gMidiClk = MIDI_CLK_UART0; // MIDI clock source
int gHPWRange = 12; // Half Pitch Wheel range

// The blinking is done via timer interrupt with the timer_callback
// function below
bool midi_clock_check_callback( repeating_timer_t *rt ) {
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_put(LED_PIN, gLEDPinValue);

    gLEDPinValue = gLEDPinValue? false : true;

    return true;
}

// The function below converts bpm to ms
uint32_t bpm_to_ms(uint32_t bpm) {
    return 60000 / bpm;
}

// The function below converts bpm to us
uint32_t bpm_to_us(uint32_t bpm) {
    return 60000000 / bpm;
}

void init_midi_clock_check(int bpm) {
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    int tick_ms = bpm_to_ms(bpm);

    static repeating_timer_t timer;
    add_repeating_timer_us((tick_ms >> 1), 
        &midi_clock_check_callback, NULL, &timer );
} 


////////////////////////////////////////////////////////////////////////////////
// The code below belong to misc MIDI control
void SetMidiChannel(int uartNo, int midiCh) {
    if (uartNo < 0 || uartNo > 1) {
        return;
    }
    if (midiCh < MIDI_CH_1 || midiCh > MIDI_CH_ALL) {
        return;
    }
    if (uartNo == 0) {
        gMidiChUart0 = midiCh;
    }
    else { // uartNo == 1
        gMidiChUart1 = midiCh;
    }
}

void SetClockSource(int clockSource) {
    if (clockSource < MIDI_CLK_UART0 || clockSource > MIDI_CLK_INTERNAL) {
        return;
    }

    gMidiClk = clockSource;
}

void SetHalfPitchWheelRange(int noOfhalfNotes) {
    if (noOfhalfNotes < 0 || noOfhalfNotes > 24) {
        return;
    }

    gHPWRange = noOfhalfNotes;
}

////////////////////////////////////////////////////////////////////////////////
// The code below belong to UART X interrupt handling

// The UART0 is initiated in the function init_uart0()
// which is called in the main() function
int init_uart0_for_MIDI_and_interrupt() {
    return init_uartX_for_MIDI_and_interrupt(0);
}

// The UART1 is initiated in the function init_uart1()
// which is called in the main() function
int init_uart1_for_MIDI_and_interrupt() {
    return init_uartX_for_MIDI_and_interrupt(1);
}

// The UART0 is initiated in the function init_uart0()
// which is called in the main() function
int init_uartX_for_MIDI_and_interrupt(int uartNo) {

    if (uartNo < 0 || uartNo > 1) {
        return MIDI_HOST_UART_ERR_UART_NO;
    }
    
    static bool isOnboardLEDInitiated = false;

    // Initiate MIDI Clock
    if (!isOnboardLEDInitiated) {
        // Initiate the onboard green led to follow the
        // quarter note beat
        const uint LED_PIN = PICO_DEFAULT_LED_PIN;
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
        isOnboardLEDInitiated = true;
    }

    uint baudrate = 0;
    // Set up our UART with a basic baud rate.
    if (uartNo == 0) {
        baudrate = uart_init(UART_0, BAUD_RATE_MIDI);
    }
    else {
        baudrate = uart_init(UART_1, BAUD_RATE_MIDI);
    }
    // if baudrate is not the same as desired, return with fail
    if (baudrate != BAUD_RATE_MIDI) {
        return MIDI_HOST_UART_ERR_BAUDRATE;
    }

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    // (void return)
    // gpio_set_function(uartNo == 0? UART0_TX_PIN : UART1_TX_PIN, 
    //    GPIO_FUNC_UART);
    if (uartNo == 0) {
        gpio_set_function(UART0_RX_PIN, GPIO_FUNC_UART);
    }
    else {
        gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);
    }

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as 
    // close as possible to that requested
    int __unused actual = 0;
    if (uartNo == 0) {
        actual = uart_set_baudrate(UART_0, BAUD_RATE_MIDI);
    }
    else {
        actual = uart_set_baudrate(UART_1, BAUD_RATE_MIDI);
    }
    // if actual baudrate is not the same as desired, return with fail
    if (actual != BAUD_RATE_MIDI) {
        return MIDI_HOST_UART_ERR_BAUDRATE;
    }

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    // (void return)
    if (uartNo == 0) {
        uart_set_hw_flow(UART_0, false, false);
    }
    else {
        uart_set_hw_flow(UART_1, false, false);
    }

    // Set our data format
    // (void return)
    if (uartNo == 0) {
        uart_set_format(UART_0, DATA_BITS_8, STOP_BITS_1, NO_PARITY);
    }
    else {
        uart_set_format(UART_1, DATA_BITS_8, STOP_BITS_1, NO_PARITY);
    }

    // Turn off FIFO's - we want to do this character by character
    // (void return)
    if (uartNo == 0) {
        uart_set_fifo_enabled(UART_0, false);
    }
    else {
        uart_set_fifo_enabled(UART_1, false);
    }
    // Set up a RX interrupt
    // We need to set up the handler first
    // And set up and enable the interrupt handlers
    // (void return)
    if (uartNo == 0) {
        irq_set_exclusive_handler(UART0_IRQ, on_uart0_rx_for_MIDI_intr_handler);
    }
    else {
        irq_set_exclusive_handler(UART1_IRQ, on_uart1_rx_for_MIDI_intr_handler);
    } 
    
    // (void return)
    if (uartNo == 0) {
        irq_set_enabled(UART0_IRQ, true);
    }
    else {
        irq_set_enabled(UART1_IRQ, true);
    }

    // Now enable the UART to send interrupts - RX only
    // (void return)
    if (uartNo == 0) {
        uart_set_irq_enables(UART_0, true, false);
    }
    else {
        uart_set_irq_enables(UART_1, true, false);
    }

    // No errors
    return MIDI_HOST_UART_ERR_SUCCESS;
}

// UART0 RX interrupt handler
static inline void on_uart0_rx_for_MIDI_intr_handler() {
    while (uart_is_readable(UART_0)) {
        uint8_t val = uart_getc(UART_0);
        uartX_rx_for_MIDI_intr_handler(0, gMidiChUart0, val);
    }
}

// UART1 RX interrupt handler
static inline void on_uart1_rx_for_MIDI_intr_handler() {
    while (uart_is_readable(UART_1)) {
        uint8_t val = uart_getc(UART_1);
        uartX_rx_for_MIDI_intr_handler(1, gMidiChUart1, val);
    }
}

// UART X RX interrupt handler
// Called by on_uart0_rx_intr_handler(), and on_uart1_rx_intr_handler()
static inline void uartX_rx_for_MIDI_intr_handler(int uartNo, int midiCh, uint8_t val) {
    // Since this function is listening on both MIDI messages from
    // both UART0 and UART1, all static variables must be
    // tagged with uart number!
    static enum midiStatus midiStat_u0 = reset;
    static enum midiStatus midiStat_u1 = reset;
    enum midiStatus *pmidiStat = NULL;
    pmidiStat = (uartNo == 0)? &midiStat_u0 : &midiStat_u1;

    static uint8_t status_u0 = 0; // MIDI Status value u0
    static uint8_t status_u1 = 0; // MIDI Status value u1
    uint8_t *pstatus = NULL;
    pstatus = (uartNo == 0)? &status_u0 : &status_u1;

    static uint8_t data1_u0 = 0; // MIDI data1 value u0
    static uint8_t data1_u1 = 0; // MIDI data1 value u1
    uint8_t *pdata1 = NULL;
    pdata1 = (uartNo == 0)? &data1_u0 : &data1_u1;

    static uint8_t data2_u0 = 0; // MIDI data2 value u0
    static uint8_t data2_u1 = 0; // MIDI data2 value u1
    uint8_t *pdata2 = NULL;
    pdata2 = (uartNo == 0)? &data2_u0 : &data2_u1;

    static int byteCount_u0 = 0; // Count the number of bytes since MIDI Status
    static int byteCount_u1 = 0; // Count the number of bytes since MIDI Status
    int *pbyteCount = NULL;
    pbyteCount = (uartNo == 0)? &byteCount_u0 : &byteCount_u1;

    static int expectedByteCount_u0 = 0; // Number of expected bytes dependent
    // on the MIDI message
    static int expectedByteCount_u1 = 0; // Number of expected bytes dependent
    // on the MIDI message
    int *pexpectedByteCount = NULL;
    pexpectedByteCount = (uartNo == 0)? &expectedByteCount_u0 : 
        &expectedByteCount_u1;

    // Test if the it is status or data
    bool isStatus = (val & 0x80) == 0x80? true : false;
    uint8_t sys = 0; // MIDI System Status value
      
    if (isStatus) {
        if ((val & 0xF0) < 0xF0) {
            *pstatus = val;
            *pbyteCount = 1;
            *pexpectedByteCount = 3;
        }

        switch (val & 0xF0) {
        case 0x80:
            *pmidiStat = noteOff;
            break;
        case 0x90:
            *pmidiStat = noteOn;
            break;
        case 0xA0:
            *pmidiStat = polyAfter;
            break;
        case 0xB0:
            *pmidiStat = controlChange;
            break;
        case 0xC0:
            *pmidiStat = programChange;
            break;
        case 0xD0:
            *pmidiStat = chanAfter;
            break;
        case 0xE0:
            *pmidiStat = pitchWheel;
            break;
        case 0xF0:
            {
                switch (val) {
                case 0xF0:
                    *pstatus = val;
                    *pbyteCount = 1;
                    *pexpectedByteCount = 3;
                    *pmidiStat = sysExStart;
                    break;   
                case 0xF1:
                    *pstatus = val;
                    *pbyteCount = 1;
                    *pexpectedByteCount = 2;
                    *pmidiStat = quarterFrame;
                    break;   
                case 0xF2:
                    *pstatus = val;
                    *pbyteCount = 1;
                    *pexpectedByteCount = 3;
                    *pmidiStat = songPointer;
                    break;   
                case 0xF3:
                    *pstatus = val;
                    *pbyteCount = 1;
                    *pexpectedByteCount = 2;
                    *pmidiStat = songSelect;
                    break;   
                case 0xF4: // Undefined
                    break;   
                case 0xF5: // Undefined
                    break;   
                case 0xF6:
                    sys = val;
                    *pmidiStat = tuneRequest;
                    break;   
                case 0xF7:
                    sys = val;
                    *pmidiStat = sysExEnd;
                    break;   
                case 0xF8:
                    sys = val;
                    *pmidiStat = timingClock;
                    break;   
                case 0xF9:
                    *pstatus = val;
                    *pbyteCount = 1;
                    *pexpectedByteCount = 2;
                    *pmidiStat = measureEnd;
                    break;   
                case 0xFA:
                    sys = val;
                    *pmidiStat = start;
                    break;   
                case 0xFB:
                    sys = val;
                    *pmidiStat = cont;
                    break;   
                case 0xFC:
                    sys = val;
                    *pmidiStat = stop;
                    break;   
                case 0xFD: // Undefined
                    break;   
                case 0xFE:
                    sys = val;
                    *pmidiStat = activeSensing;
                    break;   
                case 0xFF:
                    sys = val;
                    *pmidiStat = reset;
                    break;   
                }
            }
            break;
        }

        if (sys) {
            if (!sys_msg_callback(sys)) {
                // Do something with the error
            }
        }
    }
    else {
        if (!*pbyteCount) {
            // If byteCount is 0, no MIDI status has been sent
            return;
        }
        else {
            // Increment byte count
            *pbyteCount = *pbyteCount + 1;
        }

        if (*pbyteCount == 2) {
            *pdata1 = val;
        }
        else if (*pbyteCount == 3) {
            *pdata2 = val;
        }

        if (*pbyteCount >= *pexpectedByteCount) {

            if ((*pstatus & 0xF0) < 0xF0) {
                uint8_t midiCh = *pstatus & 0x0F;

                switch(*pstatus & 0xF0) {
                case 0x80:
                    if (!midi_note_off_callback(midiCh, *pdata1, *pdata2)) {
                        // Do something when error
                    }
                    break;
                case 0x90:
                    if (!midi_note_on_callback(midiCh, *pdata1, *pdata2)) {
                        // Do something when error
                    }
                    break;
                case 0xA0:
                    if (!polyphonic_aftertouch_callback(midiCh, *pdata1, *pdata2)) {
                        // Do something when error
                    }
                    break;
                case 0xB0:
                    if (!control_change_callback(midiCh, *pdata1, *pdata2)) {
                        // Do something when error
                    }
                    break;
                case 0xC0:
                    if (!program_change_callback(midiCh, *pdata1, *pdata2)) {
                        // Do something when error
                    }
                    break;
                case 0xD0:
                    if (!channel_aftertouch_callback(midiCh, *pdata1, *pdata2)) {
                        // Do something when error
                    }
                    break;
                case 0xE0:
                    if (!pitch_wheel_callback(midiCh, *pdata1, *pdata2)) {
                        // Do something when error
                    }
                    break;
                }
            }
            else { // status >= 0xF0
                switch (*pstatus) {
                case 0xF0:
                    if (!sysExStart_callback(*pdata1, *pdata2)) {
                        // Do something when error
                    }
                    break;   
                case 0xF1:
                    if (!quarterFrame_callback(*pdata1)) {
                        // Do something when error
                    }
                    break;   
                case 0xF2:
                    if (!songPointer_callback(*pdata1, *pdata2)) {
                        // Do something when error
                    }
                    break;   
                case 0xF3:
                    if (!songSelect_callback(*pdata1)) {
                        // Do something when error
                    }
                    break;   
                case 0xF9:
                    if (!measureEnd_callback(*pdata1)) {
                        // Do something when error
                    }
                    break;   
                }
            }

            // Time to reset variables
            *pmidiStat = reset;
            *pstatus = 0; // MIDI Status value
            *pdata1 = 0; // MIDI data1 value
            *pdata2 = 0; // MIDI data2 value
            *pbyteCount = 0; // Count the number of bytes since MIDI Status
            *pexpectedByteCount = 0; // Number of expected bytes dependent on
        }
    }
}

static inline bool midi_note_off_callback(uint8_t midiCh, uint8_t noteNo, uint8_t velocity) {    
    if (gPM) {
        printf("NoteOff ");
    }
    return true;
}

static inline bool midi_note_on_callback(uint8_t midiCh, uint8_t noteNo, uint8_t velocity) {
    set_midiNote(noteNo);
    uint16_t dacValue = set_get_mcp4725_dac_value(false, 0);
    if (gPM) {
        printf("NoteOn(%d, %d) ", noteNo, dacValue);    
    }
    return true;
}

static inline bool polyphonic_aftertouch_callback(uint8_t midiCh, uint8_t noteNo, uint8_t pressure) {
    if (gPM) {
        printf("PolyAfter ");
    }
    return true;
}

static inline bool control_change_callback(uint8_t midiCh, uint8_t controlNo, uint8_t data) {
    if (gPM) {
        printf("CtrlChang ");
        switch (controlNo) {
        case 1:
            break;

        default:
            break;
        }
    }
    return true;
}

static inline bool program_change_callback(uint8_t midiCh, uint8_t programNo, uint8_t unused) {
    if (gPM) {
        printf("PrgChang ");
    }
    return true;
}

static inline bool channel_aftertouch_callback(uint8_t midiCh, uint8_t pressure, uint8_t unused) {
    if (gPM) {
        printf("ChanAfter ");
    }
    return true;
}

static inline bool pitch_wheel_callback(uint8_t midiCh, uint8_t lsb, uint8_t msb) {
    if (gPM) {
        printf("PW(%d %d) ", msb, lsb);
    }

    set_pitch_wheel(lsb, msb, gHPWRange);

    //const int32_t pwMidValue = 64 * 256 + 0;
    //int32_t pwAbsValue = msb * 256 + lsb;
    //int32_t pwValue = pwAbsValue - pwMidValue;
    //int32_t pwDacValue = (pwValue * gHPWRange * DAC_HALF_NOTE_VALUE) /
    //    pwMidValue;

    //set_pitch_wheel_to_mcp4725_dac((int16_t)pwDacValue);

    return true;
}

static inline bool sysExStart_callback(uint8_t anufID, uint8_t data) {
    if (gPM) {
        printf("SysExStart ");
    }
    return true;
}

static inline bool quarterFrame_callback(uint8_t data) {
    if (gPM) {
        printf("QuartFrame ");
    }
    return true;
}

static inline bool songPointer_callback(uint8_t lsb, uint8_t msb) {
    if (gPM) {
        printf("SongPtr ");
    }
    return true;
}

static inline bool songSelect_callback(uint8_t songNo) {
    if (gPM) {
        printf("SongSel ");
    }
    return true;
}

static inline bool measureEnd_callback(uint8_t unused) {
    if (gPM) {
        printf("MeasEnd ");
    }
    return true;
}

static inline bool sys_msg_callback(uint8_t sys) {
    static int timerCount = 0;
    
    if (sys == 0xF8) { // midi.timingClock
        //printf("f8 ");

        if (timerCount == 0) {
            gpio_put(PICO_DEFAULT_LED_PIN, 1);
        }
        else if(timerCount == 5) {
            gpio_put(PICO_DEFAULT_LED_PIN, 0);
        }

        timerCount++;

        if (timerCount >= 24) {
            timerCount = 0;
        }
    }

    if (sys == 0xFA) { // midi.start
        if (gPM) {
            printf("timingSync ");
        }
        timerCount = 0;
    }

    return true;
}





// OLD! UART0 RX interrupt handler
void old_on_uart0_rx_intr_handler() {
    while (uart_is_readable(UART_0)) {
        uint8_t ch = uart_getc(UART_0);
        
        if (ch == 0xf8) {
            return;
        }

        // Can we send it back?
        ///////////////////////////////////////////
        // No! we are sending ch as hex to the USB!
        ///////////////////////////////////////////
        //if (uart_is_writable(UART_0)) {
        //    // Change it slightly first!
        //    ch++;
        //    uart_putc(UART_0, ch);
        //}

        uint8_t msn = ch >> 4; // most significant nibble
        uint8_t lsn = ch & 0x0f; // least significant nibble

        uint8_t chHexM = hex_from_nibble(msn);
        uint8_t chHexL = hex_from_nibble(lsn);
        const char str_usb[4] = { chHexM, chHexL, 0x20, 0x00 };

        if (gPM) {
            printf(str_usb);
        }

        //gChars_rxed++;
    }
}

// OLD! UART1 RX interrupt handler
void old_on_uart1_rx_intr_handler() {

    while (uart_is_readable(UART_1)) {
        uint8_t ch = uart_getc(UART_1);

        uint8_t msn = ch >> 4; // most significant nibble
        uint8_t lsn = ch & 0x0f; // least significant nibble

        char chHexM = (char)hex_from_nibble(msn);
        char chHexL = (char)hex_from_nibble(lsn);
        const char str_usb[4] = { chHexM, chHexL, 0x20, 0x00 };

        if (gPM) {
            printf(str_usb);
        }
    }
}

// Function hex_from_nibble
static inline uint8_t hex_from_nibble(uint8_t nb) {
    // Just make sure it is the lowest nibble
    uint8_t ch = nb & 0x0f;
    
    if (ch <= 9) {
        // If nb has the value 0 to 9,
        // just add the value for '0' char
        ch += (uint8_t)('0');
    }
    else {
        // If nb has the value 10 to 15,
        // subtract by 10 and then
        // just add the value for 'a' char
        ch += ((uint8_t)('a') - 0x0a);
    }

    // Return the character
    return ch;
}



////////////////////////////////////////////////////////////////////////////////
