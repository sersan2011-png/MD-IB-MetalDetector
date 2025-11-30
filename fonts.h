#ifndef __FONTS_H
#define __FONTS_H

#include <stdint.h>

typedef struct {
    const uint8_t *data;
    uint16_t width;
    uint16_t height;
} FontDef;

extern FontDef Font12x12;
extern FontDef Font16x26;

#endif
