/***********************************************
/ error_list.h : error list for the MIDI UART functions
/ Author: Patrik Källback - (c) 2023 PunkSynth
/ License: GPLv3
/***********************************************/

#ifndef ERROR_LIST_H
#define ERROR_LIST_H

#define MIDI_HOST_UART_ERR
#define MIDI_HOST_UART_ERR_SUCCESS 0 // There is no error
#define MIDI_HOST_UART_ERR_BAUDRATE -1 // Desired and set baud rate differs
#define MIDI_HOST_UART_ERR_UART_NO -2 // Desired and set baud rate differs

#endif // ERROR_LIST_H