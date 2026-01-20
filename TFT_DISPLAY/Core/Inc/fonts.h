#ifndef FONTS_H
#define FONTS_H

#include "stdint.h"

typedef struct {
    const uint8_t *data;
    uint8_t width;
    uint8_t height;
} FontDef;

extern FontDef Font_7x10;
extern FontDef Font_11x18;
extern FontDef Font_16x26;

#endif
