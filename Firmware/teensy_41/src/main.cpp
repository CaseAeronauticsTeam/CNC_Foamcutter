#include <Arduino.h>

const int xJoyPin = 23;
const int yJoyPin = 22;
const int xDirPin = 4;
const int yDirPin = 5;
const int xStepPin = 6;
const int yStepPin = 7;

void setup() {
    pinMode(xDirPin,  OUTPUT);
    pinMode(yDirPin,  OUTPUT);
    pinMode(xStepPin, OUTPUT);
    pinMode(yStepPin, OUTPUT);
}

const int adc_bitwidth = 10;
const int noise_margin = 50;
const int udelay = 30;
const int max_delay = 30;
const int min_delay = 30;
const int delay_coeff = 1;
const int adc_max = 1 << adc_bitwidth;
const int half_adc_max = 1 << (adc_bitwidth - 1);


int x_joy;
int y_joy;
int var_udelay;

void loop() {
    x_joy = analogRead(xJoyPin);
    y_joy = analogRead(yJoyPin);

    if (x_joy > ((1 << (adc_bitwidth - 1)) + noise_margin)) {
        digitalWrite(xDirPin, 1);
        digitalWrite(xStepPin, 1);
        digitalWrite(xStepPin, 0);
//        delayMicroseconds(udelay);
        var_udelay = (min_delay - max_delay) * (x_joy - half_adc_max) / half_adc_max + max_delay; // May have casting/truncation issues
//        delayMicroseconds(delay_coeff * ((1 << adc_bitwidth) - x_joy + base_delay));
        delayMicroseconds(var_udelay * delay_coeff);
    } else if (x_joy < ((1 << (adc_bitwidth - 1)) - noise_margin)) {
        digitalWrite(xDirPin, 0);
        digitalWrite(xStepPin, 1);
        digitalWrite(xStepPin, 0);
//        delayMicroseconds(udelay);
//        delayMicroseconds(delay_coeff * ((1 << adc_bitwidth) - x_joy + base_delay));
        var_udelay = (max_delay - min_delay) * x_joy / half_adc_max + min_delay;
        delayMicroseconds(var_udelay * delay_coeff);
    }

    if (y_joy > ((1 << (adc_bitwidth - 1)) + noise_margin)) {
        digitalWrite(yDirPin, 1);
        digitalWrite(yStepPin, 1);
        digitalWrite(yStepPin, 0);
//        delayMicroseconds(udelay);
//        delayMicroseconds(delay_coeff * ((1 << adc_bitwidth) - y_joy + base_delay));
        var_udelay = (min_delay - max_delay) * (x_joy - half_adc_max) / half_adc_max + max_delay; // May have casting/truncation issues
        delayMicroseconds(var_udelay * delay_coeff);
    } else if (y_joy < ((1 << (adc_bitwidth - 1)) - noise_margin)) {
        digitalWrite(yDirPin, 0);
        digitalWrite(yStepPin, 1);
        digitalWrite(yStepPin, 0);
//        delayMicroseconds(udelay);
        var_udelay = (max_delay - min_delay) * x_joy / half_adc_max + min_delay;
        delayMicroseconds(var_udelay * delay_coeff);
    }
}
