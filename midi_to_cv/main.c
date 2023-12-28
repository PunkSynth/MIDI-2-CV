/***********************************************
/ main.c : implementation file for the main functions
/ Author: Patrik Källback - (c) 2023 PunkSynth
/ License: GPLv3
/***********************************************/

#include "main.h"
#include "midi_uart.h"
#include "error_list.h"
#include "mcp4725.h"

bool gPM = false; // Print debug messages if true

int main() {
    // Initialize chosen serial port
    stdio_init_all();

    int errNo = 0;
    
    // Initiate uart0 and its interrupt
    errNo = init_uart0_for_MIDI_and_interrupt();
    if (errNo != MIDI_HOST_UART_ERR_SUCCESS) {
        sleep_ms(10000);
        printf("Error while initiating\n");
        printf("init_uart0_for_MIDI_and_iterrupt()\n");
        return 1;
    }

    // Initiate uart1 and its interrupt
    errNo = init_uart1_for_MIDI_and_interrupt();
    if (errNo != MIDI_HOST_UART_ERR_SUCCESS) {
        sleep_ms(10000);
        printf("Error while initiating\n");
        printf("init_uart1_for_MIDI_and_iterrupt()\n");
        return 1;
    }

    // Initiate DAC MCP4725 via i2c
    errNo = (int)init_i2c_mcp4725(MCP4725_ADDR, MCP4725_BAUDRATE);
    if (errNo != (int)true) {
        sleep_ms(10000);
        printf("Error while initiating\n");
        printf("init_i2c_mcp4725()\n");
        return 1;
    }
    
    // Init automatic DAC, update every 250 us
    init_mcp4725_us_timer_event(MCP4725_TIMER_UPDATE_250);
    // Init gliede event, update every 1000 us
    init_glide_timer_event();

    sleep_ms(10000);
    printf("MIDI_TO_CV 0.00.01.008\n");

    while (1) {
        tight_loop_contents();
    }
}
