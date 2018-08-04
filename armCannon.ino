#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// Create the strip object
#define PIN 6
Adafruit_NeoPixel strip = Adafruit_NeoPixel(65, PIN, NEO_GRB + NEO_KHZ800);


// Utility Functions
uint8_t getR(uint32_t color) { return (uint8_t)(color >> 16); }
uint8_t getG(uint32_t color) { return (uint8_t)(color >>  8); }
uint8_t getB(uint32_t color) { return (uint8_t)color; }
uint8_t lerpChannel(uint8_t value1, uint8_t value2, float factor) {
    return value1 + (factor * (value2 - value1));
}

void setup() {
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
}



class LEDSet {
    unsigned long previousMillis;
    uint8_t cycler = 0; // 0-255 incremented each update, useful for cyclers.
    long interval = 10; // in milliseconds
    // Coords for the subset of strip we're controlling:
    int x1;
    int x2;
    // Colors for fades
    uint32_t color1;
    uint32_t color2;
    bool reverse;
    float brightModAmp = 0.5;
    float brightModFreq = 0.5;
    float gradCycleFreq = 2;

    public: LEDSet () {
        x1 = 0; // First index in LED strip
        x2 = 1; // Second index
        previousMillis = 0;
        color1 = strip.Color(64, 0, 0);
        color2 = strip.Color(0, 64, 0);
    }

    public: LEDSet (int pos1, int pos2) {
        x1 = pos1; // First index in LED strip
        x2 = pos2; // Second index
        previousMillis = 0;
        color1 = strip.Color(64, 0, 0);
        color2 = strip.Color(0, 64, 0);
    }


    uint32_t lerpColorMirror(uint32_t color1, uint32_t color2, float factor, bool reverse = false) {
        if (factor < 0.5) {
            factor = factor * 2;
        } else {
            factor = 1 - ((factor - 0.5) * 2);
        }
        return strip.Color(
            lerpChannel(getR(color1), getR(color2), factor),
            lerpChannel(getG(color1), getG(color2), factor),
            lerpChannel(getB(color1), getB(color2), factor)
        );
    }

    void setColorGradCycle(
        uint32_t color1, uint32_t color2,
        uint8_t n1, uint8_t n2, uint8_t cycle,
        bool reverse = false
    ) {
        if (reverse) {
            cycle = 128 - cycle;
            uint32_t tColor = color1;
            color1 = color2;
            color2 = tColor;
        }
        for (uint8_t i = 0; i <= (n2 - n1); i++) {
            float factor = (float)i / (n2 - n1);
            // Now we cycle the factor...
            factor = modf(factor + ((float)cycle / 256), nullptr);
            strip.setPixelColor( 
                n1 + i,
                lerpColorMirror(color1, color2, factor, reverse) );
        }
    }

    // Input color, a sine vector, and a multiplier factor to modulate the
    // brightness. Returns a color. Vector in this should be an incrementor
    // to maintain a clean animation. Factor should be 0-1.
    uint32_t sineModBright(uint32_t color, byte vector, float factor ) {
        float value = ((strip.sine8(vector) * factor) / 255) - (0.5 * factor);
        int8_t r = getR(color);
        int8_t g = getG(color);
        int8_t b = getB(color);
        return strip.Color(
            r + (r * value),
            g + (g * value),
            b + (b * value));
    }

    void init (int pos1, int pos2, bool r = false) {
        x1 = pos1; // First index in LED strip
        x2 = pos2; // Second index
        previousMillis = 0;
        reverse = r;
    }

    void setColor(uint32_t c1, uint32_t c2) {
        color1 = c1;
        color2 = c2;
    }

    void Update() {
        unsigned long currentMillis = millis();
        if((currentMillis - previousMillis > interval)) {
            setColorGradCycle(
                sineModBright(color1, cycler * brightModFreq, brightModAmp),
                sineModBright(color2, cycler * brightModFreq, brightModAmp),
                x1, x2, cycler * gradCycleFreq, reverse);
            previousMillis = currentMillis;
            cycler = cycler + 2;
        }
    }
};


class ArmCannon {
    LEDSet ledSet1;
    LEDSet ledSet2;
    LEDSet ledSet3;
    LEDSet ledSet4;
    LEDSet ledSet5;
    const byte ANIM_DEFAULT = 0; // Normal Beam
    const byte ANIM_ICE     = 1; // IceBeam
    const byte ANIM_PLAZMA  = 2;
    byte state = ANIM_PLAZMA;

    // Default colors are set to the starter beam
    uint32_t boreColor1   = strip.Color(56, 48, 0);
    uint32_t boreColor2   = strip.Color(72, 8, 0);
    uint32_t radialColor1 = strip.Color(56, 48, 0);
    uint32_t radialColor2 = strip.Color(72, 8, 0);
    uint32_t muzzleColor  = strip.Color(16, 8, 2);


    public: ArmCannon () {
        ledSet1.init( 0, 11);
        ledSet2.init(12, 23, true);
        ledSet3.init(24, 39);
        ledSet4.init(40, 55, true);
        ledSet5.init(56, 64);
    }

    void Update() {
        if (state == ANIM_DEFAULT) {
            boreColor1   = strip.Color(56, 48, 0);
            boreColor2   = strip.Color(72, 8, 0);
            radialColor1 = strip.Color(56, 48, 0);
            radialColor2 = strip.Color(72, 8, 0);
            muzzleColor  = strip.Color(16, 8, 2);
        } else if (state == ANIM_ICE) { 
            boreColor1   = strip.Color(0, 0, 72);
            boreColor2   = strip.Color(32, 32, 32);
            radialColor1 = strip.Color(0, 0, 72);
            radialColor2 = strip.Color(32, 32, 32);
            muzzleColor  = strip.Color(2, 8, 16);
        } else if (state == ANIM_PLAZMA) { 
            boreColor1   = strip.Color(0, 72, 0);
            boreColor2   = strip.Color(24, 32, 24);
            radialColor1 = strip.Color(0, 72, 0);
            radialColor2 = strip.Color(16, 48, 16);
            muzzleColor  = strip.Color(2, 16, 8);
        }
        ledSet1.setColor(boreColor1, boreColor2);
        ledSet2.setColor(boreColor1, boreColor2);
        ledSet3.setColor(radialColor1, radialColor2);
        ledSet4.setColor(radialColor1, radialColor2);
        ledSet5.setColor(boreColor1, boreColor2);

        ledSet1.Update();
        ledSet2.Update();
        ledSet3.Update();
        ledSet4.Update();
        ledSet5.Update();
    }
};


ArmCannon ac;

void loop() {
    ac.Update();
    strip.show();
}
