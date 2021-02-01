#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
    #include <avr/power.h>
#endif
#include "ledSet.h"
#include "ledUtility.h"

int timer = 0; // Global timer

// Create the strip object
#define PIXELPIN 6
Adafruit_NeoPixel strip = Adafruit_NeoPixel(43, PIXELPIN, NEO_GRB + NEO_KHZ800);

// Color Constants
static const uint32_t cWhite = strip.Color(255,255,255);

class TwoMeters {
    private:
    // Light objects
    LEDSet ledSet1; // Target meter
    LEDSet ledSet2; // Actual meter

    static const byte changeDuration = 64; // Max 255
    
    // Default colors are set to the starter beam
    uint32_t Color1 = strip.Color(56, 40, 0);
    uint32_t Color2 = strip.Color(72, 8, 0);

    public: TwoMeters () {
        ledSet1.init( 0, 21);
        ledSet2.init(22, 43, true); //true means "reverse" the LED order here
        // ? Do I need to init them like this?
        ledSet1.setColor(Color1, Color2);
        ledSet2.setColor(Color1, Color2);
    }


    void update() {
        ledSet1.Update();
        ledSet2.Update();
    }
};

class SerialListenServer{
    public:
    SerialListenServer() {
        /* // Not necessary for integrated USB serial port
        Serial.begin(9600);
        while (!Serial) {
            ; // wait for serial port to connect. Needed for native USB
        } */
    }

    void process() {
        char ch = Serial.read();
        if (ch == 'x') {
            Serial.println("pressed: x");
        }
        if (ch == 'z') {
            Serial.println("pressed: z");
        }
    }
};

TwoMeters meters;

void loop() {
    meters.update();
    strip.show();
    timer++;

// This whole section is just debug.
/*
    if (timer % 32 == 0) {
        Serial.print("timer:");
        Serial.print(timer);
    }
*/
}