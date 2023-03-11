/**
 * IRIS I PAYLOAD - DATALOGGING SOFTWARE
 * 
 * Microcontroller used: ESP32-WROVER
 * 
 * @author Kail Olson, Vardaan Malhotra
 */

// including Arduino libraries
#include "MS5611.h" //used for barometer
#include <Adafruit_LSM6DSO32.h> //used for IMU
#include "SPIMemory.h"

#include "FS.h"
#include "SD_MMC.h"

// defining LED pins
#define BLUE_LED_PIN 25
#define GREEN_LED_PIN 26

// defining SPI pins


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
/*
int accel_calib_factor_x;
int accel_calib_factor_y;
int accel_calib_factor_z;
int gyro_calib_factor_x;
int gyro_calib_factor_y;
int gyro_calib_factor_z;
*/
// setting up barometer library
MS5611 MS5611(0x77);
*/
// setting up IMU library
Adafruit_LSM6DSO32 dso32;

// setting up flash library
SPIFlash flash(27);

// Declaring the state variable
State_Machine state = INITIALIZATION;


void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char * path){
    Serial.printf("Creating Dir: %s\n", path);
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char * path){
    Serial.printf("Removing Dir: %s\n", path);
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
}

void appendFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
}

void renameFile(fs::FS &fs, const char * path1, const char * path2){
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\n", path);
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char * path){
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if(file){
        len = file.size();
        size_t flen = len;
        start = millis();
        while(len){
            size_t toRead = len;
            if(toRead > 512){
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %u ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }


    file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for(i=0; i<2048; i++){
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %u ms\n", 2048 * 512, end);
    file.close();
}

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
            /*while (!MS5611.begin()) {
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

  uint8_t cardType = SD_MMC.cardType();

    if(cardType == CARD_NONE){
        Serial.println("No SD_MMC card attached");
        return;
    }

    Serial.print("SD_MMC Card Type: ");
    if(cardType == CARD_MMC){
        Serial.println("MMC");
    } else if(cardType == CARD_SD){
        Serial.println("SDSC");
    } else if(cardType == CARD_SDHC){
        Serial.println("SDHC");
    } else {
        Serial.println("UNKNOWN");
    }

    uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);

    listDir(SD_MMC, "/", 0);
    createDir(SD_MMC, "/mydir");
    listDir(SD_MMC, "/", 0);
    removeDir(SD_MMC, "/mydir");
    listDir(SD_MMC, "/", 2);
    writeFile(SD_MMC, "/hello.txt", "Hello ");
    appendFile(SD_MMC, "/hello.txt", "World!\n");
    readFile(SD_MMC, "/hello.txt");
    deleteFile(SD_MMC, "/foo.txt");
    renameFile(SD_MMC, "/hello.txt", "/foo.txt");
    readFile(SD_MMC, "/foo.txt");
    testFileIO(SD_MMC, "/test.txt");
    Serial.printf("Total space: %lluMB\n", SD_MMC.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD_MMC.usedBytes() / (1024 * 1024));


            // move to CALIBRATION state for now
            state = LAUNCH;
            break;
        case READ_MEMORY:
        //GO INTO READ MEMORY ONCE ACCE
            break;
        case ERASE_MEMORY:
            break;
        case CALIBRATION:
            /*
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
            */
            break;
        case PRELAUNCH:
            break;
        case LAUNCH:
            // poll barometer
            /*MS5611.read();

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
            dso32.getEvent(&accel, &gyro, &temp);

            /* Display the sensor results (temperature is measured in deg. C) */
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
