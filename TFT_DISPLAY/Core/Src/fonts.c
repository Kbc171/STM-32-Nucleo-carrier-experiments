#include "fonts.h"

// Simple 7x10 font set (Example minimal font)
static const uint8_t Font7x10_Data[] = {
    // You can use a full font file later
    // This is just placeholder space character
    0x00,0x00,0x00,0x00,0x00,
};

FontDef Font_7x10 = {Font7x10_Data, 7, 10};
FontDef Font_11x18 = {Font7x10_Data, 11, 18};
FontDef Font_16x26 = {Font7x10_Data, 16, 26};
