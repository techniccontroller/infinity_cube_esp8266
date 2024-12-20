#ifndef ledstrip_h
#define ledstrip_h

#include <Adafruit_NeoPixel.h>          // NeoPixel library used to run the NeoPixel LEDs:
#include "udplogger.h"
#include "constants.h"

#define DEFAULT_CURRENT_LIMIT 9999

class LEDStrip{
    public:
        LEDStrip(Adafruit_NeoPixel *neopixel_leds, UDPLogger *logger);
        static uint32_t Color24bit(uint8_t r, uint8_t g, uint8_t b);
        static uint32_t Wheel(uint8_t WheelPos);
        static uint32_t interpolateColor24bit(uint32_t color1, uint32_t color2, float factor);

        void initializeStrip();

        void setBrightness(uint8_t brightness);
        void setCurrentLimit(uint16_t currentLimit);

        uint8_t getBrightness();

        void flush();

        void setPixel(uint16_t pixel, uint32_t color);

        void drawOnLEDsInstant();
        void drawOnLEDsSmooth(float factor);

        void runLEDTest();

        void setDynamicColorShiftPhase(int16_t phase);

    private:
        Adafruit_NeoPixel *neopixel_leds;
        UDPLogger *logger;

        uint16_t currentLimit;
        uint8_t stripBrightness;
        int16_t dynamicColorShiftActivePhase = -1; // -1: not active, 0-255: active phase shift

        uint32_t target[LED_COUNT] = {0};
        uint32_t current[LED_COUNT] = {0};

        void drawOnLEDs(float factor);
        uint16_t calcEstimatedLEDCurrent(uint32_t color, uint8_t brightness);
        
};


#endif