#include <Arduino.h>

const int xJoyPin = 22;
const int yJoyPin = 23;
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
const int max_delay = 2000;
const int min_delay = 100;
const int pulse_width = 1;  // usec
const double delay_coeff = 1;
const int half_adc_max = 1 << (adc_bitwidth - 1);


int x_joy;
int y_joy;
double var_udelay;

void loop() {
    x_joy = analogRead(xJoyPin);
    y_joy = analogRead(yJoyPin);

    if (x_joy > ((1 << (adc_bitwidth - 1)) + noise_margin)) {
        digitalWrite(xDirPin, 1);
        digitalWrite(xStepPin, 1);
        delayMicroseconds(pulse_width);
        digitalWrite(xStepPin, 0);
        var_udelay = ((double)(min_delay - max_delay)) * (double)(x_joy - half_adc_max) / (double)half_adc_max + (double)max_delay; // May have casting/truncation issues
        delayMicroseconds((int)(var_udelay * delay_coeff));
    } else if (x_joy < ((1 << (adc_bitwidth - 1)) - noise_margin)) {
        digitalWrite(xDirPin, 0);
        digitalWrite(xStepPin, 1);
        delayMicroseconds(pulse_width);
        digitalWrite(xStepPin, 0);
        var_udelay = (double)(max_delay - min_delay) * (double)x_joy / (double)half_adc_max + min_delay;
        delayMicroseconds((int)(var_udelay * delay_coeff));
    }

    if (y_joy > ((1 << (adc_bitwidth - 1)) + noise_margin)) {
        digitalWrite(yDirPin, 0);
        digitalWrite(yStepPin, 1);
        delayMicroseconds(pulse_width);
        digitalWrite(yStepPin, 0);
        var_udelay = (double)(min_delay - max_delay) * (double)(y_joy - half_adc_max) / (double)half_adc_max + max_delay; // May have casting/truncation issues
        delayMicroseconds((int)(var_udelay * delay_coeff));
    } else if (y_joy < ((1 << (adc_bitwidth - 1)) - noise_margin)) {
        digitalWrite(yDirPin, 1);
        digitalWrite(yStepPin, 1);
        delayMicroseconds(pulse_width);
        digitalWrite(yStepPin, 0);
        var_udelay = (double)(max_delay - min_delay) * (double)y_joy / (double)half_adc_max + min_delay;
        delayMicroseconds((int)(var_udelay * delay_coeff));
    }
}
