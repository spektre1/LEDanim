#include <Adafruit_NeoPixel.h>
#include <EnableInterrupt.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif


// Animation names
static const byte ANIM_DEFAULT = 0; // Normal Beam
static const byte ANIM_ICE     = 1; // IceBeam
static const byte ANIM_PLAZMA  = 2;
static const byte ANIM_WHITE   = 3;



int timer = 0; // Global timer
int button1State = 0;
volatile unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;


// Create the strip object
#define SWITCH1PIN 2
#define SWITCH2PIN 3
#define PIXELPIN 6
Adafruit_NeoPixel strip = Adafruit_NeoPixel(65, PIXELPIN, NEO_GRB + NEO_KHZ800);

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

void setup() {
    //Serial.begin(9600);
    //Serial.print("Freshly flashed! Running...");
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    pinMode(SWITCH1PIN, INPUT_PULLUP);
    pinMode(SWITCH2PIN, INPUT_PULLUP);
    enableInterrupt(SWITCH1PIN, changeMode, LOW);
    enableInterrupt(SWITCH2PIN, fire, LOW);
    enableInterrupt(SWITCH2PIN, fire2, HIGH);
}

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
};

class ArmCannon {
    private:
    // Light objects
    LEDSet ledSet1; // Barrel line L
    LEDSet ledSet2; // Barrel line R
    LEDSet ledSet3; // Barrel Ring 1
    LEDSet ledSet4; // Barrel Ring 2
    LEDSet ledSet5; // Muzzle

    bool charging;  // Fire button was recently held down
    bool firing;    // Fire button was recently released
    byte chargingIncrementor; // How far through charge loop
    byte firingIncrementor;   // same for firing anim TODO: Could probably put reuse
    static const byte chargeDuration = 128; // Max 255
    static const byte firingDuration = 64; // Max 255


    static const byte ANIM_COUNT   = 4; // Need this to track mode switch

    volatile byte state = ANIM_DEFAULT;
    bool stateTrans = false;
    static const byte changeDuration = 64; // Max 255

    byte stateIncrementor = 0; // Used for state changes

    // Default colors are set to the starter beam
    uint32_t Color1 = strip.Color(10, 10, 10);
    uint32_t Color2 = strip.Color(10, 10, 10);
    uint32_t Color3 = strip.Color(10, 10, 10);
    uint32_t lastColor1;
    uint32_t lastColor2;
    uint32_t lastColor3;
    uint32_t newColor1;
    uint32_t newColor2;
    uint32_t newColor3;
    uint32_t fireTempColor1;
    uint32_t fireTempColor2;
    uint32_t fireTempColor3;


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
    byte getState(){
        return state;
    }

    //
    void changeState(byte newState) {
        if (newState == state) {return;} // Do nothing if in this state already
        stateIncrementor = 0; // Start new change counter
        stateTrans = true; // 
        state = newState; 
        // Record where we were
        lastColor1 = Color1;
        lastColor2 = Color2;
        lastColor3 = Color3;
        // Set where we're going
        if (state == ANIM_DEFAULT) {
            newColor1 = strip.Color(56, 40, 0);
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
            newColor1 = strip.Color(255, 255, 255);
            newColor2 = strip.Color(255, 255, 255);
            newColor3 = strip.Color(255, 255, 255);
        } 
    }

    void setState(byte newState) {
        state = newState;
    }
    void pressFire(bool fireButtonState) {
        //False is button down, true is button up
        if (fireButtonState == true) {
            charging = true;
            firing = false;
        } else {
            charging = false;
            firing = true;
        }
    }

    // Cycle to next mode over time, depends on ANIM_COUNT
    void nextState() {
        if (state >= (ANIM_COUNT - 1)) {
            changeState(0);
        } else {
            changeState(state + 1);
        }
    }

    void Update() {
        //if (timer == 512) { changeState(ANIM_PLAZMA);}
        //if (timer == 1024) { changeState(ANIM_ICE);}
        //if (timer == 1500) { changeState(ANIM_DEFAULT); timer=0;}
        if (stateTrans == true) {
            if (stateIncrementor == changeDuration) {stateTrans = false;}
            Color1 = lerpColor(lastColor1, newColor1, (float)stateIncrementor/changeDuration);
            Color2 = lerpColor(lastColor2, newColor2, (float)stateIncrementor/changeDuration);
            Color3 = lerpColor(lastColor2, newColor2, (float)stateIncrementor/changeDuration);
            ledSet1.setColor(Color1, Color2);
            ledSet2.setColor(Color1, Color2);
            ledSet3.setColor(Color1, Color2);
            ledSet4.setColor(Color1, Color2);
            ledSet5.setColor(Color3, Color3);
        }
        
        if (charging == true) {


            // How do I normalize this?
            // ledSet1.setBrightModAmp(1.0); // This has to be necessary here and scaled to incrementor?
            // ledSet1.setBrightModAmp(0.0);


            if (chargingIncrementor > 128) {
                // charged = true;
            } // Just thinking...
            fireTempColor1 = lerpColorMirror(
                Color1, strip.Color(255,255,255),
                (float)chargingIncrementor/chargingDuration);
            fireTempColor2 = lerpColorMirror(
                Color1, strip.Color(255,255,255),
                (float)chargingIncrementor/chargingDuration);
            fireTempColor3 = lerpColorMirror(
                Color1, strip.Color(255,255,255),
                (float)chargingIncrementor/chargingDuration);
            ledSet1.setColor(fireTempColor1, fireTempColor2);
            ledSet2.setColor(fireTempColor1, fireTempColor2);
            ledSet3.setColor(fireTempColor1, fireTempColor2);
            ledSet4.setColor(fireTempColor1, fireTempColor2);
            ledSet5.setColor(Color3, Color3);
            chargingIncrementor++;
        } else if (firing == true) {
            if (chargingIncrementor > 64) {} // It's fully charged! Do something diff
            firingIncrementor++;
        }

        ledSet1.Update();
        ledSet2.Update();
        ledSet3.Update();
        ledSet4.Update();
        ledSet5.Update();
        stateIncrementor++;
    }
};

ArmCannon ac;

void loop() {

    ac.Update();
    strip.show();
    timer++;
    // This whole section is just debug.
    /*
    if (timer % 64 == 0) {
        Serial.print("timer:");
        Serial.print(timer);
        Serial.print(" - state:");
        Serial.println(ac.getState());
    }
    */
}

void changeMode() {
    if ((millis() - lastDebounceTime) > debounceDelay) {
        //ac.changeState(ANIM_ICE);
        ac.nextState();
        //Serial.print("SWITCH! State change: ");
        //Serial.println(ac.getState());
        lastDebounceTime = millis();
    }
}

void fire()  { ac.pressFire(true); } // Fire button pressed
void fire2() { ac.pressFire(false); }  // Fire button depressed