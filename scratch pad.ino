#include <Adafruit_NeoPixel.h>

uint32_t lerpColor(
    uint32_t color1, uint32_t color2, float factor) {
    return strip.Color(
        lerpChannel(getR(color1), getR(color2), factor),
        lerpChannel(getG(color1), getG(color2), factor),
        lerpChannel(getB(color1), getB(color2), factor)
    );
}
void setColorGrad(uint32_t color1, uint32_t color2, uint8_t n1, uint8_t n2) {
    for (uint8_t i = 0; i <= (n2 - n1); i++) {
        float factor = (float)i / (n2 - n1);
        strip.setPixelColor( 
            n1 + i,
            lerpColor(color1, color2, factor) );
    }
}

//x1 is beginning pixel, x2 is end of fill
void colorFill(uint32_t color, uint8_t i, uint8_t x) {
    for(; i<=x; i++) {
        strip.setPixelColor(i, color);
    }
}

