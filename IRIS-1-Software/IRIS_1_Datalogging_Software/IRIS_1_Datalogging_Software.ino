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

// defining LED pins
#define LED_1_PIN 25
#define LED_2_PIN 26

// defining IMU SPI pins
#define LSM_CS 9
#define LSM_SCK 13
#define LSM_MISO 8
#define LSM_MOSI 16

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

// offsets generated from CALIBRATION
int accel_calib_factor_x;
int accel_calib_factor_y;
int accel_calib_factor_z;
int gyro_calib_factor_x;
int gyro_calib_factor_y;
int gyro_calib_factor_z;

// setting up barometer library
MS5611 MS5611(0x77);

// setting up IMU library
Adafruit_LSM6DSO32 dso32;

// Declaring the state variable
State_Machine state = CALIBRATION;

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

void loop() {
    
    switch (state) {
        case INITIALIZATION:
            // setting up serial
            Serial.begin(115200);
            delay(50); // delay to open without errors

            // initializing barometer
            while (!MS5611.begin()) {
                Serial.println("Barometer not found.");
                digitalWrite(LED_1_PIN, HIGH);
                delay(500);
                digitalWrite(LED_1_PIN, LOW);
                delay(500);
            }

            // barometer initialization successful
            Serial.println("Barometer found.");
            digitalWrite(LED_2_PIN, HIGH);
            delay(500);
            digitalWrite(LED_2_PIN, LOW);

            // initializing IMU
            while(!dso32.begin_SPI(LSM_CS, LSM_SCK, LSM_MISO, LSM_MOSI)){
                Serial.println("LSM6DS032 not found.");
                digitalWrite(LED_1_PIN, HIGH);
                delay(500);
                digitalWrite(LED_1_PIN, LOW);
                delay(500);              
            }

            // IMU initialization successful
            Serial.println("LSM6DS032 found.");
            digitalWrite(LED_2_PIN, HIGH);
            delay(500);
            digitalWrite(LED_2_PIN, LOW);

            // setting acceleromter range
            dso32.setAccelRange(LSM6DSO32_ACCEL_RANGE_8_G);
            Serial.println("Accelerometer range set to: +-8G");

            // setting accelrometer data rate
            dso32.setAccelDataRate(LSM6DS_RATE_12_5_HZ);
            Serial.println("Accelerometer data rate set to: 12.5 Hz");

            // setting gyroscope range
            dso32.setGyroRange(LSM6DS_GYRO_RANGE_250_DPS);
            Serial.println("Gyro range set to: +-250 degrees/s");

             // setting gyroscope data rate
            dso32.setGyroDataRate(LSM6DS_RATE_12_5_HZ);
            Serial.print("Gyro data rate set to: 12.5 Hz");

            // move to CALIBRATION state for now
            state = CALIBRATION;
            break;
        case READ_MEMORY:
            break;
        case ERASE_MEMORY:
            break;
        case CALIBRATION:
            int calib_data = 50;
            int i;
            int accel_x[calib_data];
            int accel_y[calib_data];
            int accel_z[calib_data];
            int gyro_x[calib_data];
            int gyro_y[calib_data];
            int gyro_z[calib_data];            
            int temp_sum = 0;


            // calibrating accelerometer
            Serial.println("Acceleration along x dimension:")
            for (i=0; i<calib_data; i++){
                accel_x[i] = accel.acceleration.x
                Serial.print(accel_x[i]);
                temp_sum = accel_x[i] + temp_sum;
            }
            accel_calib_factor_x = temp_sum/calib_data;
            temp_sum = 0;

            Serial.println("Acceleration along y dimension:")
            for (i=0; i<calib_data; i++){
                accel_y[i] = accel.acceleration.y
                Serial.print(accel_y[i]);
                temp_sum = accel_y[i] + temp_sum;
            }
            accel_calib_factor_y = temp_sum/calib_data;
            temp_sum = 0;

            Serial.println("Acceleration along z dimension:")
            for (i=0; i<calib_data; i++){
                accel_z[i] = accel.acceleration.z
                Serial.print(accel_z[i]);
                temp_sum = accel_z[i] + temp_sum;
            }
            accel_calib_factor_z = temp_sum/calib_data;
            temp_sum = 0;

            //calibrating gyroscope
            Serial.println("Angular velocity along x dimension:")
            for (i=0; i<calib_data; i++){
                gyro_x[i] = gyro.gyro.x
                Serial.print(gyro_x[i]);
                temp_sum = gyro_x[i] + temp_sum;
            }
            gyro_calib_factor_x = temp_sum/calib_data;
            temp_sum = 0;

            Serial.println("Angular velocity along y dimension:")
            for (i=0; i<calib_data; i++){
                gyro_y[i] = gyro.gyro.y
                Serial.print(gyro_y[i]);
                temp_sum =  gyro_y[i] + temp_sum;
            }
            gyro_calib_factor_y = temp_sum/calib_data;
            temp_sum = 0;

            Serial.println("Angular velocity along z dimension:")
            for (i=0; i<calib_data; i++){
                gyro_z[i] = gyro.gyro.z
                Serial.print(gyro_z[i]);
                temp_sum = gyro_z[i] + temp_sum;
            }
            gyro_calib_factor_z = temp_sum/calib_data;
            temp_sum = 0;

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
            delay(100);

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
            Serial.print(accel.acceleration.x + accel_calib_factor_x);
            Serial.print(" \tY: ");
            Serial.print(accel.acceleration.y + accel_calib_factor_y);
            Serial.print(" \tZ: ");
            Serial.print(accel.acceleration.z + accel_calib_factor_z);
            Serial.println(" m/s^2 ");

            /* Display the results (rotation is measured in rad/s) */
            Serial.print("\t\tGyro X: ");
            Serial.print(gyro.gyro.x + gyro_calib_factor_x);
            Serial.print(" \tY: ");
            Serial.print(gyro.gyro.y + gyro_calib_factor_y);
            Serial.print(" \tZ: ");
            Serial.print(gyro.gyro.z + gyro_calib_factor_z);
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