/***********************************************
/ main.h : header file for the main functions
/ Author: Patrik Källback - (c) 2023 PunkSynth
/ License: GPLv3
/***********************************************/

#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "hardware/irq.h"

extern bool gPM; // Print MIDI Messages

int main();

#endif // MAIN_H