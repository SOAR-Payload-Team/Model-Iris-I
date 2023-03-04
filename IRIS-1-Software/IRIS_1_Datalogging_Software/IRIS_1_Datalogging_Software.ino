/**
 * IRIS I PAYLOAD - DATALOGGING SOFTWARE
 * 
 * Microcontroller used: ESP32-WROVER
 * 
 * @author Kail Olson, Vardaan Malhotra
 */

// including Arduino libraries
#include "MS5611.h"
#include <Adafruit_LSM6DSO32.h>
#include "SPIMemory.h"

// defining LED pins
#define GREEN_LED_PIN 26
#define BLUE_LED_PIN 25

// setting up the state machine enumerator
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

// setting up IMU library
Adafruit_LSM6DSO32 dso32;

// setting up flash library
SPIFlash flash(27);

// Declaring the state variable
State_Machine state = INITIALIZATION;

/**
 * @brief
*/
void setup() {
    // setting up LED pins
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);

    // setting up serial
    Serial.begin(115200);
    delay(50); // delay to open without errors

}

void loop() {
    
    switch (state) {
        case INITIALIZATION:

            // initializing barometer
            while (!MS5611.begin()) {
                Serial.println("Barometer not found.");
                blinkLED(BLUE_LED_PIN, 500);
                delay(500);
            }

            // barometer initialization successful
            Serial.println("Barometer found.");
            blinkLED(GREEN_LED_PIN, 500);

            // initializing IMU
            while(!dso32.begin_I2C()){
                Serial.println("LSM6DS032 not found.");
                blinkLED(BLUE_LED_PIN, 500);
                delay(500);              
            }

            // IMU initialization successful
            Serial.println("LSM6DS032 found.");
            blinkLED(GREEN_LED_PIN, 500);


            // setting acceleromter range
            dso32.setAccelRange(LSM6DSO32_ACCEL_RANGE_4_G);

            // setting accelrometer data rate
            dso32.setAccelDataRate(LSM6DS_RATE_6_66K_HZ);

            // setting gyroscope range
            dso32.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);

             // setting gyroscope data rate
            dso32.setGyroDataRate(LSM6DS_RATE_6_66K_HZ);

            // move to CALIBRATION state for now
            state = CALIBRATION;
            break;
        case READ_MEMORY:
            break;
        case ERASE_MEMORY:
            break;
        case CALIBRATION:

            // move to LAUNCH state for now
            state = LAUNCH;
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

             // Get a new normalized sensor event
            sensors_event_t accel;
            sensors_event_t gyro;
            sensors_event_t temp;
            dso32.getEvent(&accel, &gyro, &temp);

            Serial.print("\t\tTemperature ");
            Serial.print(temp.temperature);
            Serial.println(" deg C");

          //TODO: PUT THE CALIBRATION VALUES IN THE APPROPRIATE REGISTERS ON THE LSM6DSO32 INSTEAD OF ADDING TO THE VALUES AS DONE IN THE CODE BELOW???

            /* Display the results (acceleration is measured in m/s^2) */
            Serial.print("\t\tAccel X: ");
            Serial.print(accel.acceleration.x);
            Serial.print(" \tY: ");
            Serial.print(accel.acceleration.y);
            Serial.print(" \tZ: ");
            Serial.print(accel.acceleration.z);
            Serial.println(" m/s^2 ");

            /* Display the results (rotation is measured in rad/s) */
            Serial.print("\t\tGyro X: ");
            Serial.print(gyro.gyro.x);
            Serial.print(" \tY: ");
            Serial.print(gyro.gyro.y);
            Serial.print(" \tZ: ");
            Serial.print(gyro.gyro.z);
            Serial.println(" radians/s ");
            Serial.println();
            delay(100);
            
            break;
        case KALMAN_FILTER:
            break;
        case POSTLAUNCH:
            break;
        default:
            break;
    }
}

void blinkLED(int led_pin, uint32_t time_ms) {
  digitalWrite(led_pin, HIGH);
  delay(time_ms);
  digitalWrite(led_pin, LOW);
}
