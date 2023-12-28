///////////////////////////////////////////////////
// mcp4725.c - implementation file of i2c MCP4725
///////////////////////////////////////////////////

#include <stdio.h>
#include "main.h"
#include "mcp4725.h"

int gDACVal = 0;
int gDACValOld = 0;
int16_t gPW_DACVal = 0;
int gMIDINote = 0;
int gGlideVal = 89; // DEBUG ONLY!!!
int gGlideType = GLIDE_TYPE_GLISSANDO; // DEBUG ONLY!!!
// The unit in the table is half notes per hecto seconds
// When index is 0 the glide is 655.35 half notes per second
// When index is 127 the glide is 1.00 half notes per second
uint16_t gGlideTable[128] = { 
    65535, 25600, 24498, 23443, 22434, 21468, 20544, 19659,
    18813, 18003, 17228, 16486, 15776, 15097, 14447, 13825,
    13230, 12660, 12115, 11593, 11094, 10616, 10159, 9722,
    9303, 8903, 8520, 8153, 7802, 7466, 7144, 6837,
    6542, 6261, 5991, 5733, 5486, 5250, 5024, 4808, 
    4601, 4403, 4213, 4032, 3858, 3692, 3533, 3381, 
    3235, 3096, 2963, 2835, 2713, 2596, 2485, 2378,
    2275, 2177, 2084, 1994, 1908, 1826, 1747, 1672, 
    1600, 1531, 1465, 1402, 1342, 1284, 1229, 1176, 
    1125, 1077, 1030, 986, 944, 903, 864, 827, 
    791, 757, 725, 693, 664, 635, 608, 581, 
    556, 532, 510, 488, 467, 447, 427, 409, 
    391, 374, 358, 343, 328, 314, 300, 288, 
    275, 263, 252, 241, 231, 221, 211, 202, 
    194, 185, 177, 170, 162, 155, 149, 142,
    136, 130, 125, 119, 114, 109, 104, 100 };

uint64_t gCurrentTick = 0;
uint64_t gBeginTick = 0;
uint64_t gEndTick = 0;
float gCurrentNoteFactor = 1.f;
float gCurrentNote = 0.f;
float gBeginNote = 0.f;
float gEndNote = 0.f;


bool init_i2c_mcp4725(uint8_t addr, uint baudrate) {
    // This example will use I2C0 on the default SDA and SCL pins (8, 9 on a Pico)
    i2c_init(i2c_default, baudrate);
    gpio_set_function(I2C0_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA);
    gpio_pull_up(I2C0_SCL);

    // Make the I2C pins available to picotool
    bi_decl(bi_2pins_with_func(I2C0_SDA, I2C0_SCL, GPIO_FUNC_I2C));
    uint8_t rxdata = 0x00;
    int ret = i2c_read_blocking(i2c_default, addr, &rxdata, 1, false);

    if (ret != PICO_ERROR_GENERIC) {
        setDefault_i2c_mcp4725(addr, 0);    
    }

    return ret != PICO_ERROR_GENERIC? true : false;
}

static inline bool setOutput_i2c_mcp4725(uint8_t addr, uint16_t output) {
    // Upper data bits (D11.D10.D9.D8.D7.D6.D5.D4)
    // Lower data bits (D3.D2.D1.D0.x.x.x.x)
    uint8_t packet[3] = { MCP4725_CMD_WRITEDAC, 
        (uint8_t)(output >> 4), 
        (uint8_t)((output & 0x000f) << 4) };

    int ret = i2c_write_blocking(i2c_default, addr, packet, 3, false);

    return ret != PICO_ERROR_GENERIC? true : false;
}

bool setDefault_i2c_mcp4725(uint8_t addr, uint16_t output) {
    uint8_t packet[3] = { MCP4725_CMD_WRITEDACEEPROM, 
        (uint8_t)(output >> 4), 
        (uint8_t)((output & 0x000f) << 4) };

    int ret = i2c_write_blocking(i2c_default, addr, packet, 3, false);

    return ret != PICO_ERROR_GENERIC? true : false;
}

void init_mcp4725_us_timer_event(int us_timer_event) { // Minimum 250 us
    static repeating_timer_t timer;
    add_repeating_timer_us(us_timer_event, 
        &mcp4725_us_timer_callback, NULL, &timer);
}

static inline bool mcp4725_us_timer_callback(repeating_timer_t *rt) {
    
    if (gDACValOld != gDACVal) {
        gDACValOld = gDACVal;
        setOutput_i2c_mcp4725(MCP4725_ADDR, gDACVal);
    }
    return true;
}

void init_glide_timer_event() { // Every 1000 us
    static repeating_timer_t timer;
    add_repeating_timer_us(GLIDE_TIMER_UPDATE, 
        &glide_timer_callback, NULL, &timer);
}

static inline bool glide_timer_callback(repeating_timer_t *rt) {
    gCurrentTick  = time_us_64();

    if (gCurrentTick > gEndTick) {
        gCurrentNote = gEndNote;
    }
    else {
        // The calculation without gCurrentNoteFactor
        //gCurrentNote = (float)(gCurrentTick - gBeginTick) / 
        //    (float)(gEndTick - gBeginTick) * (gEndNote - gBeginNote) + gBeginNote;

        // The calculation with gCurrentNoteFactor
        gCurrentNote = (float)(gCurrentTick - gBeginTick) * gCurrentNoteFactor + 
            gBeginNote;
    }

    set_get_mcp4725_dac_value(true, calculate_dac_value());

    return true;
}

uint16_t set_get_mcp4725_dac_value(bool isSet, uint16_t dacValue) {
    if (isSet) {
        if (dacValue > MCP4725_MAX_VALUE) {
            gDACVal = MCP4725_MAX_VALUE;
        }
        else {
            gDACVal = dacValue;
        }
    }
    return gDACVal;
}

// Based on midi note (gMIDINote) and pitch wheel (gPW_DACVal)
static inline uint16_t calculate_dac_value() {
    int16_t dacValue = 0; 
    
    if (gGlideType == GLIDE_TYPE_PORTAMENTO) {
        dacValue = (int16_t)((gCurrentNote - MIDI_C0_NOTE_VALUE) * 
            DAC_HALF_NOTE_VALUE + DAC_VALUE_C0_NOTE + (float)gPW_DACVal + 0.5f);
    }
    else if (gGlideType == GLIDE_TYPE_GLISSANDO) {
        float intCurrentNote = (float)((int)(gCurrentNote));
        dacValue = (int16_t)((intCurrentNote - MIDI_C0_NOTE_VALUE) * 
            DAC_HALF_NOTE_VALUE + DAC_VALUE_C0_NOTE + (float)gPW_DACVal + 0.5f);
    }

    if (dacValue < MCP4725_MIN_VALUE) {
        return 0;
    }
    else if (dacValue > MCP4725_MAX_VALUE) {
        return MCP4725_MAX_VALUE;
    }
    else {
        return (uint16_t)dacValue;
    }
}

void set_midiNote(uint8_t noteNo) {
    // Initiate things with glide in mind
    gBeginNote = gCurrentNote;
    gEndNote = (int)noteNo;
    gBeginTick = time_us_64();
    gEndTick = calculate_glide_end_tick(gBeginTick, gBeginNote, gEndNote);
    gCurrentNoteFactor = 1.f / (float)(gEndTick - gBeginTick) * 
        (gEndNote - gBeginNote);

    printf("%.1f %.1f %d %llu %llu | ", gBeginNote, gEndNote, 
        gGlideTable[gGlideVal], gBeginTick, gEndTick);

    /*****************************************************/
    /* Since we intruduce the glide the dac value should */
    /* be calculated in the glide algorithm              */
    /*****************************************************/

    //uint16_t dacVal = 0;
    //if (noteNo < MIDI_C0_NOTE_VALUE) {
    //    dacVal = set_get_mcp4725_dac_value(true, MCP4725_MIN_VALUE);
    //}
    //else if (noteNo > MIDI_C8_NOTE_VALUE) {
    //    dacVal = set_get_mcp4725_dac_value(true, MCP4725_MAX_VALUE);
    //}
    //else {
    //    dacVal = set_get_mcp4725_dac_value(true, calculate_dac_value());
    //}
    
    //if (gPM) {
    //    printf("%d ", dacVal);
    //}
}

void set_pitch_wheel(uint8_t lsb, uint8_t msb, int hpwRange) {
    const int32_t pwMidValue = 64 * 256 + 0;
    int32_t pwAbsValue = msb * 256 + lsb;
    int32_t pwValue = pwAbsValue - pwMidValue;
    gPW_DACVal = (pwValue * hpwRange * DAC_HALF_NOTE_VALUE) /
        pwMidValue;

    uint16_t dacVal = set_get_mcp4725_dac_value(true, calculate_dac_value());

    if (gPM) {
        printf("%d ", gPW_DACVal);
    }
}

// Returns the midi_note as a float value
float dac_value_to_midi_note(uint16_t dacValue) {
    return (float)(dacValue-30) / 42.f + 12.f;
}


uint64_t calculate_glide_end_tick(uint64_t beginTick, float beginNote, float endNote) {
    // Sanity check gGlideVal
    if (gGlideVal < 0) {
        gGlideVal = 0;
    }
    else if (gGlideVal > 127) {
        gGlideVal = 127;
    }
    else {
        // gGlideVal is ok!
    }

    // half notes per hecto seconds
    uint16_t glideRate = gGlideTable[gGlideVal]; 
        
    float deltaNote = 0;
    if (endNote > beginNote) {
        deltaNote = endNote - beginNote;
    }
    else {
        deltaNote = beginNote - endNote;       
    }

    // Unit is [s]
    float deltaTime = deltaNote / ((float)glideRate) * 100.f;

    if (deltaTime < 0.f) {
        deltaTime = 0.f;
    }
    // Unit is [us]
    uint64_t endTick = beginTick + (uint64_t)(deltaTime * 1000000.f);

    return endTick;
}