// Utility Functions
uint8_t getR(uint32_t color) { return (uint8_t)(color >> 16); }
uint8_t getG(uint32_t color) { return (uint8_t)(color >>  8); }
uint8_t getB(uint32_t color) { return (uint8_t)color; }


uint8_t lerpChannel(uint8_t value1, uint8_t value2, float scalar) {
    return value1 + (scalar * (value2 - value1));
}


uint32_t lerpColor(
    uint32_t color1, uint32_t color2, float scalar) {
    return strip.Color(
        lerpChannel(getR(color1), getR(color2), scalar),
        lerpChannel(getG(color1), getG(color2), scalar),
        lerpChannel(getB(color1), getB(color2), scalar)
    );
}

uint32_t lerpColorMirror(uint32_t color1, uint32_t color2, float scalar, bool reverse = false) {
    if (scalar < 0.5) {
        scalar = scalar * 2;
    } else {
        scalar = 1 - ((scalar - 0.5) * 2);
    }
    return strip.Color(
        lerpChannel(getR(color1), getR(color2), scalar),
        lerpChannel(getG(color1), getG(color2), scalar),
        lerpChannel(getB(color1), getB(color2), scalar)
    );
}

