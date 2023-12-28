///////////////////////////////////////////////////
// mcp4725.h - header file of i2c MCP4725
///////////////////////////////////////////////////

#ifndef MCP4725_H
#define MCP4725_H

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"

//#define I2C0_SDA PICO_DEFAULT_I2C_SDA_PIN
//#define I2C0_SCL PICO_DEFAULT_I2C_SCL_PIN
#define I2C0_SDA 8
#define I2C0_SCL 9

#define MCP4725_ADDR 0x62
#define MCP4725_BAUDRATE 400000
#define MCP4725_CMD_WRITEDAC 0x40 // Writes data to the DAC
#define MCP4725_CMD_WRITEDACEEPROM 0x60 // Writes data to the DAC and the EEPROM (persisting the assigned value after reset)
#define MCP4725_MIN_VALUE 0
#define MCP4725_MAX_VALUE 4095
#define MCP4725_TIMER_UPDATE_250 250 // Update every 250 uS
#define GLIDE_TIMER_UPDATE 1000 // Update every 1000 uS

#define MIDI_C0_NOTE_VALUE 12 // The MIDI note for C0 note
#define MIDI_C8_NOTE_VALUE 108 // The MIDI note for C0 note
#define DAC_VALUE_C0_NOTE 30 // The 12 bit DAC value for C0 note
#define DAC_HALF_NOTE_VALUE 42 // The 12 bit DAC value from one half note to next

#define GLIDE_TYPE_PORTAMENTO 1
#define GLIDE_TYPE_GLISSANDO 2

// Global char extern declaration
extern int gDACVal;
extern int gDACValOld;
extern int16_t gPW_DACVal;

// Minimum glide speed at glide value of 127 is 1 half note / s
extern int gGlideVal; // Glide value can be 0 - 127
extern int gGlideType; // Glide type can be portamento or glissando
extern uint16_t gGlideTable[128]; // Glide table
extern uint64_t gCurrentTick;
extern uint64_t gBeginTick;
extern uint64_t gEndTick;
extern float gCurrentNoteFactor;
extern float gCurrentNote;
extern float gBeginNote;
extern float gEndNote;

bool init_i2c_mcp4725(uint8_t addr, uint baudrate);
static inline bool setOutput_i2c_mcp4725(uint8_t addr, uint16_t output);
bool setDefault_i2c_mcp4725(uint8_t addr, uint16_t output);

void init_mcp4725_us_timer_event(int us_timer_event); // Minimum 250 us
static inline bool mcp4725_us_timer_callback(repeating_timer_t *rt);

void init_glide_timer_event(); // The glide timer event is every 1000 us
static inline bool glide_timer_callback(repeating_timer_t *rt);

uint16_t set_get_mcp4725_dac_value(bool isSet, uint16_t dacValue);

// Based on midi note, pitch wheel, and portamento
static inline uint16_t calculate_dac_value(); 
void set_midiNote(uint8_t noteNo);
void set_pitch_wheel(uint8_t lsb, uint8_t msb, int hpwRange);

float dac_value_to_midi_note(uint16_t dacValue);
uint16_t midi_note_to_dac_value(float midiNote);
uint64_t calculate_glide_end_tick(uint64_t beginTick, float beginNote, float endNote);

#endif // MCP4725_H
