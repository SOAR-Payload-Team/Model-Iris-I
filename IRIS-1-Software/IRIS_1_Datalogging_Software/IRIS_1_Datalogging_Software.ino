/**
 * IRIS I PAYLOAD - DATALOGGING SOFTWARE
 * 
 * Microcontroller used: ESP32-WROVER
 * 
 * @author Kail Olson, Vardaan Malhotra
 */

// including Arduino libraries
#include "MS5611.h"

// defining LED pins
#define LED_1_PIN 25
#define LED_2_PIN 26

// Setting up the state machine enumerator
enum State_Machine {
    INITIALIZATION,
    READ_MEMORY,
    ERASE_MEMORY,
    CALIBRATION,
    PRELAUNCH,
    LAUNCH,
    KALMAN_FILTER,
    POSTLAUNCH
};

// setting up barometer library
MS5611 MS5611(0x77);

// Declaring the state variable
State_Machine state = INITIALIZATION;

/**
 * @brief
*/
void setup() {
    // setting up LED pins
    pinMode(LED_1_PIN, OUTPUT);
    pinMode(LED_2_PIN, OUTPUT);
    digitalWrite(LED_1_PIN, LOW);
    digitalWrite(LED_2_PIN, LOW);

}

int counter = 0;

void loop() {
    
    switch (state) {
        case INITIALIZATION:
            // setting up serial
            Serial.begin(115200);
            delay(50); // delay to open without errors

            // initializing barometer
            while (!MS5611.begin()) {
                Serial.println("Barometer not found :(.");
                digitalWrite(LED_1_PIN, HIGH);
                delay(500);
                digitalWrite(LED_1_PIN, LOW);
                delay(500);
            }

            // barometer initialization successful
            digitalWrite(LED_2_PIN, HIGH);
            delay(500);
            digitalWrite(LED_2_PIN, LOW);

            // move to launch state for now
            state = LAUNCH;
            break;
        case READ_MEMORY:
            break;
        case ERASE_MEMORY:
            break;
        case CALIBRATION:
            break;
        case PRELAUNCH:
            break;
        case LAUNCH:
            // poll barometer
            MS5611.read();

            // print current barometer values
            Serial.print("T:\t");
            Serial.print(MS5611.getTemperature(), 2);
            Serial.print("\tP:\t");
            Serial.print(MS5611.getPressure(), 2);
            Serial.print("\n");
            delay(100);
            break;
        case KALMAN_FILTER:
            break;
        case POSTLAUNCH:
            break;
        default:
            break;
    }

    counter++;
}

State_Machine nextState(State_Machine current_state) {
    State_Machine next_state;
    switch (current_state) {
        case INITIALIZATION:
            next_state = READ_MEMORY;
            break;
        case READ_MEMORY:
            next_state = ERASE_MEMORY;
            break;
        case ERASE_MEMORY:
            next_state = CALIBRATION;
            break;
        case CALIBRATION:
            next_state = PRELAUNCH;
            break;
        case PRELAUNCH:
            next_state = LAUNCH;
            break;
        case LAUNCH:
            next_state = KALMAN_FILTER;
            break;
        case KALMAN_FILTER:
            next_state = POSTLAUNCH;
            break;
        case POSTLAUNCH:
            next_state = INITIALIZATION;
            break;
        default:
            break;
    }
    return next_state;
}
