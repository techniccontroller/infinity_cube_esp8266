#include "ledstrip.h"

LEDStrip::LEDStrip(Adafruit_NeoPixel *neopixel_leds, UDPLogger *logger)
{
    this->neopixel_leds = neopixel_leds;
    this->logger = logger;
    currentLimit = DEFAULT_CURRENT_LIMIT;
    stripBrightness = 255;
}

/**
 * @brief Convert RGB value to 24bit color value
 *
 * @param r red value (0-255)
 * @param g green value (0-255)
 * @param b blue value (0-255)
 * @return uint32_t 24bit color value
 */
uint32_t LEDStrip::Color24bit(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

/**
 * @brief Input a value 0 to 255 to get a color value. The colors are a transition r - g - b - back to r.
 *
 * @param WheelPos Value between 0 and 255
 * @return uint32_t return 24bit color of colorwheel
 */
uint32_t LEDStrip::Wheel(uint8_t WheelPos)
{
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85)
    {
        return Color24bit(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170)
    {
        WheelPos -= 85;
        return Color24bit(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return Color24bit(WheelPos * 3, 255 - WheelPos * 3, 0);
}

/**
 * @brief Interpolates two colors24bit and returns an color of the result
 *
 * @param color1 startcolor for interpolation
 * @param color2 endcolor for interpolatio
 * @param factor which color is wanted on the path from start to end color
 * @return uint32_t interpolated color
 */
uint32_t LEDStrip::interpolateColor24bit(uint32_t color1, uint32_t color2, float factor)
{
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;

    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;

    uint8_t r = r1 + (r2 - r1) * factor;
    uint8_t g = g1 + (g2 - g1) * factor;
    uint8_t b = b1 + (b2 - b1) * factor;

    return Color24bit(r, g, b);
}

/**
 * @brief Initialize the LED strip
 *
 */
void LEDStrip::initializeStrip()
{
    neopixel_leds->begin();
    neopixel_leds->setBrightness(stripBrightness);
}

/**
 * @brief Set the brightness of the overall LED strip
 *
 * @param brightness brightness value (0-255)
 */
void LEDStrip::setBrightness(uint8_t brightness)
{
    stripBrightness = brightness;
    neopixel_leds->setBrightness(brightness);
}

/**
 * @brief Set the current limit for the LED strip
 *
 * @param currentLimit current limit value (0-9999 mA)
 */
void LEDStrip::setCurrentLimit(uint16_t currentLimit)
{
    this->currentLimit = currentLimit;
}

/**
 * @brief Get the current set brightness
 *
 * @return uint8_t brightness value (0-255)
 */
uint8_t LEDStrip::getBrightness()
{
    return stripBrightness;
}

/**
 * @brief Set all pixels to black (off)
 *
 * Will be shown on the LED strip after calling drawOnLEDsInstant() or drawOnLEDsSmooth()
 */
void LEDStrip::flush()
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        target[i] = 0;
    }
}

/**
 * @brief Set the color of a pixel on the LED strip
 *
 * Will be shown on the LED strip after calling drawOnLEDsInstant() or drawOnLEDsSmooth()
 *
 * @param pixel pixel number (0-90)
 * @param color 24bit color value
 */
void LEDStrip::setPixel(uint16_t pixel, uint32_t color)
{
    // check if pixel is in range
    if (pixel >= LED_COUNT)
    {
        logger->logString("ERROR: pixel out of range");
        return;
    }

    if (dynamicColorShiftActivePhase >= 0)
    {
        color = Wheel((pixel * 256 * 3 / (LED_COUNT) + dynamicColorShiftActivePhase) % 256);
    }

    target[pixel] = color;
}

/**
 * @brief Draw the target color on the LED strip instantly
 *
 */
void LEDStrip::drawOnLEDsInstant()
{
    drawOnLEDs(1.0);
}

/**
 * @brief Draw the target color on the LED strip with a smooth transition
 *
 * @param factor transition factor (1.0 = instant, 0.1 = smooth)
 */
void LEDStrip::drawOnLEDsSmooth(float factor)
{
    if (factor < 0.0 || factor > 1.0)
    {
        logger->logString("ERROR: factor out of range");
        factor = 1.0;
        return;
    }
    drawOnLEDs(factor);
}

/**
 * @brief Run a quick LED test
 *
 * Test quickly each LED, leds should light up white one after another
 */
void LEDStrip::runLEDTest()
{
    for (int i = 0; i < neopixel_leds->numPixels(); i++)
    {
        neopixel_leds->setPixelColor(i, neopixel_leds->Color(0, 0, 0));
        neopixel_leds->setPixelColor((i + 1) % neopixel_leds->numPixels(), neopixel_leds->Color(128, 128, 128));
        neopixel_leds->show();
        delay(10);
    }

    neopixel_leds->clear();
    neopixel_leds->show();
    delay(200);
}

/**
 * @brief Set dynamic color shift phase (0-255)
 *
 * @param phase phase of the color shift
 */
void LEDStrip::setDynamicColorShiftPhase(int16_t phase)
{
    dynamicColorShiftActivePhase = phase;
}

/**
 * @brief Draw the target color on the LED strip with a smooth transition
 *
 * @param factor transition factor (1.0 = instant, 0.1 = smooth)
 */
void LEDStrip::drawOnLEDs(float factor)
{

    // set pixels on the led strip and calculate current
    uint16_t totalCurrent = 0;
    for (int i = 0; i < LED_COUNT; i++)
    {
        uint32_t currentColor = current[i];
        uint32_t targetColor = target[i];
        uint32_t newColor = interpolateColor24bit(currentColor, targetColor, factor);
        neopixel_leds->setPixelColor(i, newColor);
        current[i] = newColor;

        totalCurrent += calcEstimatedLEDCurrent(newColor, stripBrightness);
    }

    // Check if totalCurrent reaches CURRENTLIMIT -> if yes reduce brightness
    if (totalCurrent > currentLimit)
    {
        uint8_t newBrightness = stripBrightness * float(currentLimit) / float(totalCurrent);
        // logger->logString("CurrentLimit reached!!!: " + String(totalCurrent));
        neopixel_leds->setBrightness(newBrightness);
    }
    else
    {
        neopixel_leds->setBrightness(stripBrightness);
    }

    neopixel_leds->show();
}

/**
 * @brief Calc estimated current (mA) for one pixel with the given color and brightness
 *
 * @param color 24bit color value of the pixel for which the current should be calculated
 * @param brightness brightness value (0-255)
 * @return the current in mA
 */
uint16_t LEDStrip::calcEstimatedLEDCurrent(uint32_t color, uint8_t brightness)
{
    // extract rgb values
    uint8_t red = color >> 16 & 0xff;
    uint8_t green = color >> 8 & 0xff;
    uint8_t blue = color & 0xff;

    // Linear estimation: 20mA for full brightness per LED
    // (calculation avoids float numbers)
    uint32_t estimatedCurrent = (20 * red) + (20 * green) + (20 * blue);
    estimatedCurrent /= 255;
    estimatedCurrent = (estimatedCurrent * brightness) / 255;

    return estimatedCurrent;
}
