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
uint32_t lerpColor(
    uint32_t color1, uint32_t color2, float factor) {
    return strip.Color(
        lerpChannel(getR(color1), getR(color2), factor),
        lerpChannel(getG(color1), getG(color2), factor),
        lerpChannel(getB(color1), getB(color2), factor)
    );
}

void setup() {
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
}



class LEDSet {
    private:
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
    private:
    LEDSet ledSet1;
    LEDSet ledSet2;
    LEDSet ledSet3;
    LEDSet ledSet4;
    LEDSet ledSet5;

    static const byte ANIM_DEFAULT = 0; // Normal Beam
    static const byte ANIM_ICE     = 1; // IceBeam
    static const byte ANIM_PLAZMA  = 2;
    static const byte ANIM_WHITE   = 3;

    byte state = ANIM_DEFAULT;
    bool stateTrans = false;
    byte changeDuration = 127; // Max 255

    byte incrementor = 0; // Used for state changes
    int timer = 0; // Global timer

    // Default colors are set to the starter beam
    uint32_t Color1 = strip.Color(56, 48, 0);
    uint32_t Color2 = strip.Color(72, 8, 0);
    uint32_t Color3 = strip.Color(16, 8, 2);
    uint32_t lastColor1;
    uint32_t lastColor2;
    uint32_t lastColor3;
    uint32_t newColor1;
    uint32_t newColor2;
    uint32_t newColor3;

    public: ArmCannon () {
        ledSet1.init( 0, 11);
        ledSet2.init(12, 23, true); //true means "reverse" the LED order here
        ledSet3.init(24, 39);
        ledSet4.init(40, 55, true);
        ledSet5.init(56, 64);
        // ? Do I need to init them like this?
        ledSet1.setColor(Color1, Color2);
        ledSet2.setColor(Color1, Color2);
        ledSet3.setColor(Color1, Color2);
        ledSet4.setColor(Color1, Color2);
        ledSet5.setColor(Color3, Color3);
    }

    void changeState(byte newState) {
        if (newState == state) {return;} // Do nothing if in this state already
        incrementor = 0;
        stateTrans = true;
        state = newState;
        lastColor1 = Color1;
        lastColor2 = Color2;
        lastColor3 = Color3;
        if (state == ANIM_DEFAULT) {
            newColor1 = strip.Color(56, 48, 0);
            newColor2 = strip.Color(72, 8, 0);
            newColor3 = strip.Color(16, 8, 2);
        } else if (state == ANIM_ICE) { 
            newColor1 = strip.Color(8, 8, 64);
            newColor2 = strip.Color(32, 32, 32);
            newColor3 = strip.Color(2, 8, 16);
        } else if (state == ANIM_PLAZMA) { 
            newColor1 = strip.Color(0, 92, 0);
            newColor2 = strip.Color(24, 40, 24);
            newColor3 = strip.Color(2, 16, 8);
        } else if ( state == ANIM_WHITE) {
            newColor1 = strip.Color(64, 64, 64);
            newColor2 = strip.Color(64, 64, 64);
            newColor3 = strip.Color(64, 64, 64);
        } 
    }


    void Update() {

        //if (timer == 512) { changeState(ANIM_ICE);}
        if (timer == 512) { changeState(ANIM_PLAZMA);}
        if (timer == 1024) { changeState(ANIM_ICE);}
        if (timer == 1500) { changeState(ANIM_DEFAULT); timer=0;}


        if (stateTrans == true) {
            if (incrementor == changeDuration) {stateTrans = false;}
            Color1 = lerpColor(lastColor1, newColor1, (float)incrementor/changeDuration);
            Color2 = lerpColor(lastColor2, newColor2, (float)incrementor/changeDuration);
            Color3 = lerpColor(lastColor2, newColor2, (float)incrementor/changeDuration);
            ledSet1.setColor(Color1, Color2);
            ledSet2.setColor(Color1, Color2);
            ledSet3.setColor(Color1, Color2);
            ledSet4.setColor(Color1, Color2);
            ledSet5.setColor(Color3, Color3);
        }

        ledSet1.Update();
        ledSet2.Update();
        ledSet3.Update();
        ledSet4.Update();
        ledSet5.Update();
        incrementor++;
        timer++;
    }
};


ArmCannon ac;

void loop() {
    ac.Update();
    strip.show();
}
