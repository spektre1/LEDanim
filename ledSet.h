

class LEDSet {
    private:
    unsigned long previousMillis = 0;
    uint8_t cycler = 0; // 0-255 incremented each update, useful for cyclers.
    static const long interval = 10; // in milliseconds
    // Coords for the subset of strip we're controlling:
    int x1;
    int x2;
    // Colors for fades
    uint32_t color1;
    uint32_t color2;
    bool reverse;
    float brightModAmp = 0.5;
    static constexpr float brightModFreq = 0.5;
    static constexpr float gradCycleFreq = 2;

    public: LEDSet () {
        x1 = 0; // First index in LED strip
        x2 = 1; // Second index
        color1 = strip.Color(64, 0, 0);
        color2 = strip.Color(0, 64, 0);
    }

    public: LEDSet (int pos1, int pos2) {
        x1 = pos1; // First index in LED strip
        x2 = pos2; // Second index
        color1 = strip.Color(64, 0, 0);
        color2 = strip.Color(0, 64, 0);
    }

    void setBrightModAmp(float newAmp) {
        brightModAmp = newAmp;
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
            float scalar = (float)i / (n2 - n1);
            // Now we cycle the scalar...
            scalar = modf(scalar + ((float)cycle / 256), nullptr);
            strip.setPixelColor( 
                n1 + i,
                lerpColorMirror(color1, color2, scalar, reverse) );
        }
    }

    // Input color, a sine vector, and a multiplier scalar to modulate the
    // brightness. Returns a color. Vector in this should be an stateIncrementor
    // to maintain a clean animation. scalar should be 0-1.
    uint32_t sineModBright(uint32_t color, byte vector, float scalar ) {
        float value = ((strip.sine8(vector) * scalar) / 255) - (0.5 * scalar);
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
}

