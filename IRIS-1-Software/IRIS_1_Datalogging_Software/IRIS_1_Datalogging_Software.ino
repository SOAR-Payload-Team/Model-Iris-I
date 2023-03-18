/**
 * IRIS I PAYLOAD - DATALOGGING SOFTWARE
 * 
 * Microcontroller used: ESP32-WROVER
 * 
 * @author Kail Olson, Vardaan Malhotra
 */

// including Arduino libraries
#include "MS5611.h" //used for barometer
#include <Adafruit_LSM6DSOX.h>

// defining LED pins
#define BLUE_LED_PIN 25
#define GREEN_LED_PIN 26


// setting up the state machine enumerator
enum State_Machine {
    INITIALIZATION = 1,
    READ_MEMORY = 2,
    ERASE_MEMORY = 3,
    CALIBRATION = 4,
    PRELAUNCH = 5,
    LAUNCH = 6,
    KALMAN_FILTER = 7,
    POSTLAUNCH = 8
};

// setting up barometer library
MS5611 MS5611(0x77);
// setting up IMU library
Adafruit_LSM6DSOX sox;


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
            while(!sox.begin_I2C()){
                Serial.println("LSM6DS0X not found.");
                blinkLED(BLUE_LED_PIN, 500);
                delay(500);              
            }

            // IMU initialization successful
            Serial.println("LSM6DS0X found.");
            blinkLED(GREEN_LED_PIN, 500);


            // setting acceleromter range
            sox.setAccelRange(LSM6DS_ACCEL_RANGE_2_G);


            // setting gyroscope range
            sox.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS );


            // setting accelrometer data rate
            sox.setAccelDataRate(LSM6DS_RATE_12_5_HZ);


            // setting gyroscope data rate
            sox.setGyroDataRate(LSM6DS_RATE_12_5_HZ);

            // move to LAUNCH state for now
            state = LAUNCH;
            break;
        case READ_MEMORY:
        //GO INTO READ MEMORY ONCE ACCE
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

            // Get a new normalized sensor event for the IMU
            sensors_event_t accel;
            sensors_event_t gyro;
            sensors_event_t temp;
            sox.getEvent(&accel, &gyro, &temp);

            //Display the sensor results (temperature is measured in deg. C)
            Serial.print("\t\tTemperature ");
            Serial.print(temp.temperature);
            Serial.println(" deg C");

            /* Display the sensor results (acceleration is measured in m/s^2) */
            Serial.print("\t\tAccel X: ");
            //Serial.print(accel.acceleration.x + accel_calib_factor_x);
            Serial.print(accel.acceleration.x);
            Serial.print(" \tY: ");
            //Serial.print(accel.acceleration.y + accel_calib_factor_y);
            Serial.print(accel.acceleration.y);
            Serial.print(" \tZ: ");
            //Serial.print(accel.acceleration.z + accel_calib_factor_z);
            Serial.print(accel.acceleration.z);
            Serial.println(" m/s^2 ");

            /* Display the sensor results (rotation is measured in rad/s) */
            Serial.print("\t\tGyro X: ");
            //Serial.print(gyro.gyro.x + gyro_calib_factor_x);
            Serial.print(gyro.gyro.x);
            Serial.print(" \tY: ");
            //Serial.print(gyro.gyro.y + gyro_calib_factor_y);
            Serial.print(gyro.gyro.y);
            Serial.print(" \tZ: ");
            //Serial.print(gyro.gyro.z + gyro_calib_factor_z);
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
