#include <Arduino.h>
//#include "main.hpp"
//#include <SPI.h>
//#include <SD.h>
#include "SdFat.h"

#define LEFT false
#define RIGHT true

#define IDLE 0
#define HOMING 1
#define RUNNING_JOB 2

const int xJoyPin = 22;
const int yJoyPin = 23;
const int xDirPin = 4;
const int yDirPin = 5;
const int xStepPin = 6;
const int yStepPin = 7;
const int home_pin = 0;
const int idle_pin = 1;
const int start_job_pin = 2;
// These should probably be G-Code parameters at some point
const double thread_pitch = 0.01;  // in Meters
const int steps_per_rev = 300;

void setup() {
    pinMode(xDirPin,  OUTPUT);
    pinMode(yDirPin,  OUTPUT);
    pinMode(xStepPin, OUTPUT);
    pinMode(yStepPin, OUTPUT);
    pinMode(home_pin, INPUT);
    pinMode(idle_pin, INPUT);
    pinMode(start_job_pin, INPUT);
    Serial.begin(9600);
//                while (!Serial);
    Serial.println("Starting...");
}

const int adc_bitwidth = 10;
const int noise_margin = 50;
const int max_delay = 2000;
const int min_delay = 100;
const int pulse_width = 1;  // usec
const double delay_coeff = 1;
const int half_adc_max = 1 << (adc_bitwidth - 1);
const double step_delta = thread_pitch / steps_per_rev;

// Loop variables
int x_joy;
int y_joy;
double var_udelay;

// State Variables
double current_x = 0.;
double current_y = 0.;
int state = IDLE;
int last_state = state;

SdFat sd;
SdFile file;

void stepX(bool dir);
void stepY(bool dir);


void loop() {
    switch (state) {
        case IDLE:
            // Set LEDs
            Serial.println("Idling");

            // Check buttons for state change
            last_state = state;
            if (digitalRead(home_pin))
                state = HOMING;
            else if (digitalRead(start_job_pin))
                state = RUNNING_JOB;
            delay(1000);
            break;
        case HOMING:
            x_joy = analogRead(xJoyPin);
            y_joy = analogRead(yJoyPin);
            if (x_joy > ((1 << (adc_bitwidth - 1)) + noise_margin)) {
                stepX(LEFT);
                var_udelay =
                        ((double) (min_delay - max_delay)) * (double) (x_joy - half_adc_max) / (double) half_adc_max +
                        (double) max_delay; // May have casting/truncation issues
                delayMicroseconds((int) (var_udelay * delay_coeff));
            } else if (x_joy < ((1 << (adc_bitwidth - 1)) - noise_margin)) {
                stepX(RIGHT);
                var_udelay = (double) (max_delay - min_delay) * (double) x_joy / (double) half_adc_max + min_delay;
                delayMicroseconds((int) (var_udelay * delay_coeff));
            }

            if (y_joy > ((1 << (adc_bitwidth - 1)) + noise_margin)) {
                stepY(LEFT);
                var_udelay =
                        (double) (min_delay - max_delay) * (double) (y_joy - half_adc_max) / (double) half_adc_max +
                        max_delay; // May have casting/truncation issues
                delayMicroseconds((int) (var_udelay * delay_coeff));
            } else if (y_joy < ((1 << (adc_bitwidth - 1)) - noise_margin)) {
                stepY(RIGHT);
                var_udelay = (double) (max_delay - min_delay) * (double) y_joy / (double) half_adc_max + min_delay;
                delayMicroseconds((int) (var_udelay * delay_coeff));
            }   

            // Check buttons for state change
            last_state = state;
            if (digitalRead(idle_pin))
                state = IDLE;
            else if (digitalRead(start_job_pin))
                state = RUNNING_JOB;
            break;
        case RUNNING_JOB:
            if (state != last_state) {
                Serial.println("Running Job");
                // Init SD Card and start job
                Serial.println("Initializing SD card...");
                if (!sd.begin(SdioConfig(FIFO_SDIO))) {
                    Serial.println("initialization failed!");
                }
                Serial.println("Initialized SD Card");

                if (!file.open("job.gcode", O_RDWR | O_CREAT)) {
                    Serial.println("Open failed!");
                }
                Serial.println("Opened File");
                Serial.println("\nDone Initializing");
            }
            int b = file.read();
            char line_buf[500];
            if (b >= 0) {
                int i = 0;
                while ((char)b != '\n' && b >= 0) {
                    line_buf[i++] = (char)b;
                    b = file.read();
                }
                line_buf[i] = '\0';
                if (line_buf[0] == ';') {
                    // Comment
                }
                else if (line_buf[0] == 'G' & line_buf[1] == '1') {
                    double x, y;
                    sscanf(line_buf, "G1 X%lf Y%lf", &x, &y);
                    Serial.print("X: ");
                    Serial.print(x);
                    Serial.print(" Y: ");
                    Serial.println(y);
                } else {
                    Serial.println("Unsupported G-Command:");
                    Serial.println(line_buf);
                }

                delay(500);
            } else {
                Serial.println("Done");
                file.close();
                while (1);
            }

            // Check buttons for state change
            last_state = state;
            if (digitalRead(idle_pin))
                state = IDLE;
            break;
//        default:
//            state = IDLE;
    }

}

void stepX(bool dir) {
    digitalWrite(xDirPin, dir);
    digitalWrite(xStepPin, 1);
    delayMicroseconds(pulse_width);
    digitalWrite(xStepPin, 0);
}

void stepY(bool dir) {
    digitalWrite(yDirPin, dir);
    digitalWrite(yStepPin, 1);
    delayMicroseconds(pulse_width);
    digitalWrite(yStepPin, 0);
}
