#include <Arduino.h>
#include <string.h>
#include "SdFat.h"

#include <SPI.h>
#include <SD.h>


#define LEFT false
#define RIGHT true
#define DOWN false
#define UP true

#define IDLE 0
#define HOMING 1
#define RUNNING_JOB 2

// User constants
const int JoyBtn = 21;
const int xJoyPin = 22;
const int yJoyPin = 23;
// TODO: CHECK PINOUT AGAINST PCB!!!
const int xDirPin = 4;
const int xStepPin = 5;
const int yDirPin = 8;
const int yStepPin = 9;
const int ext_sd_cs = 10;
const int start_job_pin = 30;
const int home_pin = 31;
const int idle_pin = 32;
const int idle_state_led = 33;
const int homing_state_led = 34;
const int running_job_state_led = 35;
// These should probably be G-Code parameters at some point
const double thread_pitch = 2.14;  // mm
const int steps_per_rev = 800;


// System Constants
const int adc_bitwidth = 10;
const int noise_margin = 50;
const int max_delay = 2000;
const int min_delay = 100;
const int pulse_width = 1;  // usec
const double delay_coeff = 1;
const int half_adc_max = 1 << (adc_bitwidth - 1);
const double step_delta = thread_pitch / (double)steps_per_rev; // mm per step

// Speed Settings
double cut_speed = 2.5;    // mm per second    set by F in G1
unsigned long step_delay_us = 1000000 * (step_delta / cut_speed);


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
void g0MoveTo(double x2, double y2);
void g1MoveTo(double x2, double y2);



void setup() {
    pinMode(JoyBtn,  INPUT_PULLUP);
    pinMode(xJoyPin,  INPUT); // Analog In
    pinMode(yJoyPin,  INPUT); // Analog In
    pinMode(xDirPin,  OUTPUT);
    pinMode(yDirPin,  OUTPUT);
    pinMode(xStepPin, OUTPUT);
    pinMode(yStepPin, OUTPUT);
    pinMode(home_pin, INPUT_PULLUP);
    pinMode(idle_pin, INPUT_PULLUP);
    pinMode(start_job_pin, INPUT_PULLUP);
    pinMode(idle_state_led,  OUTPUT);
    pinMode(homing_state_led, OUTPUT);
    pinMode(running_job_state_led, OUTPUT);
    Serial.begin(115200);
//                while (!Serial);
    Serial.println("Starting with:");
    Serial.print("Cut speed:");
    Serial.println(cut_speed, 8);
    Serial.print("Step Delay:");
    Serial.println(step_delay_us, 8);
    Serial.print("Step Delta:");
    Serial.println(step_delta, 8);
}



void loop() {
    switch (state) {
        case IDLE:
            // Set LEDs
            digitalWrite(idle_state_led, 1);
            digitalWrite(homing_state_led, 0);
            digitalWrite(running_job_state_led, 0);
            Serial.println("Idling");

            // Check buttons for state change
            last_state = state;
            if (!digitalRead(home_pin))
                state = HOMING;
            else if (!digitalRead(start_job_pin))
                state = RUNNING_JOB;
            delay(1000);
            break;
        case HOMING:
            digitalWrite(idle_state_led, 0);
            digitalWrite(homing_state_led, 1);
            digitalWrite(running_job_state_led, 0);
            Serial.println("Homing");


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
            if (!digitalRead(idle_pin))
                state = IDLE;
            else if (!digitalRead(start_job_pin))
                state = RUNNING_JOB;
            break;
        case RUNNING_JOB:
            if (state != last_state) {
                digitalWrite(idle_state_led, 0);
                digitalWrite(homing_state_led, 0);
                digitalWrite(running_job_state_led, 1);
                Serial.println("Running Job");
                current_x = 0;
                current_y = 0;
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
                // Read line into line_buf
                int i = 0;
                while ((char)b != '\n' && b >= 0) {
                    line_buf[i++] = (char)b;
                    b = file.read();
                }
                line_buf[i] = '\0'; // Null terminate the string
                int line_length = i;
                // Remove Comments
                for (int j = 0; j < line_length; j++) {
                    if (line_buf[j] == ';') {
                        line_buf[j] = '\0';
                        line_length = j;
                    }
                }

                if (line_buf[0] == 'G' && line_buf[1] == '1') {
                    double f, x, y;
                    x = 0;
                    y = 0;
                    const char delim[2] = " ";
                    char* ptr = strtok(line_buf, delim); // Tokenize by spaces
                    while (ptr != NULL) {
                        Serial.print("Token: '");
                        Serial.print(ptr);
                        Serial.print("'");
                        switch (ptr[0]) {
                            case 'X':
                                sscanf(ptr, "X%lf", &x);
                                break;
                            case 'Y':
                                sscanf(ptr, "Y%lf", &y);
                                break;
                            case 'F':
                                int ret = sscanf(ptr, "+F%lf", &f);
                                if (ret == 1) {
                                    cut_speed = f;
                                    step_delay_us = 1000000 * (step_delta / cut_speed);
                                }
                                break;
                        }
                        ptr = strtok(NULL, delim);
                    }
//                    sscanf(line_buf, "G1 X%lf Y%lf", &x, &y);
                    Serial.print("X: ");
                    Serial.print(x);
                    Serial.print(" Y: ");
                    Serial.println(y);

                    g1MoveTo(x, y);
                } else {
                    Serial.println("Unsupported G-Command:");
                    Serial.println(line_buf);
                }

            } else {
                Serial.println("Done");
                file.close();
                state = IDLE;
            }

            // Check buttons for state change
            last_state = state; // ???
            if (!digitalRead(idle_pin))
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

void stepXY(bool xDir, bool yDir) {
    digitalWrite(xDirPin, xDir);
    digitalWrite(yDirPin, yDir);
    digitalWrite(xStepPin, 1);
    digitalWrite(yStepPin, 1);
    delayMicroseconds(pulse_width);
    digitalWrite(xStepPin, 0);
    digitalWrite(yStepPin, 0);
}

void moveTo(double x2, double y2, unsigned long step_delay) {
    // Loosely based on Bresenham's Algorithm:
    // First, quantize mm to steps and determine direction
    int xSteps = abs(x2 - current_x) / step_delta;
    int ySteps = abs(y2 - current_y) / step_delta;
    bool xDir = x2 >= current_x ? RIGHT : LEFT;
    bool yDir = y2 >= current_y ? UP : DOWN;
    // Then interpolate
    if (xSteps >= ySteps) { // if slope <= 1
        int m = 2 * ySteps;
        int slope_error = m - xSteps;
        for (int x = 0, y = 0; x <= xSteps; x++) {
            // Add slope to increment angle formed
            slope_error += m;

            Serial.print('(');
            Serial.print(current_x + x * step_delta, 6);
            Serial.print(", ");
            Serial.print(current_y + y * step_delta, 6);
            Serial.println(')');
            delayMicroseconds(step_delay);

            // if slope error reached its limit, increment y and update slope error
            if (slope_error >= 0) {
                stepXY(xDir, yDir);
                y++;
                slope_error -= 2 * xSteps;
            } else {
                stepX(xDir);
            }
        }
    } else {
        int m = 2 * xSteps;
        int slope_error = m - ySteps;
        for (int x = 0, y = 0; y <= ySteps; y++) {
            slope_error += m;

            Serial.print('(');
            Serial.print(current_x + x * step_delta, 6);
            Serial.print(", ");
            Serial.print(current_y + y * step_delta, 6);
            Serial.println(')');
            delayMicroseconds(step_delay);

            if (slope_error > 0) {  // Must be > (not >=) for y-based variation
                stepXY(xDir, yDir);
                x++;
                slope_error -= 2 * ySteps;
            } else
                stepY(yDir);
        }
    }
    if (x2 >= current_x)
        current_x += xSteps * step_delta;
    else
        current_x -= xSteps * step_delta;
    if (y2 >= current_y)
        current_y += ySteps * step_delta;
    else
        current_y -= ySteps * step_delta;

}


void g0MoveTo(double x2, double y2) {
    moveTo(x2, y2, min_delay);
}


void g1MoveTo(double x2, double y2) {
    moveTo(x2, y2, step_delay_us);
}
