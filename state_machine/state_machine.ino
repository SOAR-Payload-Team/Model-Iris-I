/**
 * IRIS I PAYLOAD SOFTWARE
 * 
 * Microcontroller used: ESP32-WROVER
 * 
 * @author Kail Olson, Vardaan Malhotra
 */

 /* For some reason the read from flash is not working properly. I think the writing is working properly, but read from flash is not reading the first four addresses properly
 nor the rest of the data properly
 */

#include "flash_library.hh"
#include "LSM6DSO_library.hh"
#include "MS5611_library.hh"
#include "TMP177_library.hh"
#include "SD.h"
#include <string.h>
#include <stdlib.h>
#include <TimeLib.h>



// defining LED pins
#define BLUE_LED_PIN 25  //used to determine failed states
#define GREEN_LED_PIN 26 //used to determine successful states

//#define LAUNCH_DURATION 2400000 //40 minutes of launch time
#define LAUNCH_DURATION 25200000  //1 minute for testing
#define IDLE_WAIT 1000 //wait time while within idle loop in ms
#define LAUNCH_ACCEL_THRESHOLD 1 //acceleration in g's required to be seen on the accel.z value prior to entering launch state

// define VSPI pins
#define V_CLK 18
#define V_MISO 19
#define V_MOSI 23
#define CS_FLASH 5
#define CS_DATA_LOGGING 27  //the flash board flash chip's CS that is also using the VSPI SPI bus is initialized using SPI_peripheral.begin()
#define H_MISO 32
#define H_MOSI 13
#define H_CLK 14
#define CS_SD 33

// define VSPI pins

// setting up the state machine enumerator
enum State_Machine {
    INITIALIZATION = 1,
    IDLE = 2,
    LAUNCH = 3,
    POST_LAUNCH = 4,
    BACKUP = 5,
    READ_MEMORY_DATA_LOGGING = 6,
    READ_MEMORY_FLASH = 7,
    ERASE_MEMORY = 8,
};


// Declaring the state variable
State_Machine state = INITIALIZATION;

CHIP_TYPE CHIP_DATA_LOGGING = W25Q128;
CHIP_TYPE CHIP_FLASH = W25Q512;

/*typedef struct{
  IMU_data_t IMU_data;
  MS5611_int_digital_t MS5611_Data;
}sensor_combined_t;*/
 
uint16_t C[6]; //array to store the calibration values for the MS5611

//define offset through averaging a sample of IMU data on payload board
float ACCEL_X_OFFSET = 0;
float ACCEL_Y_OFFSET = 0;
float ACCEL_Z_OFFSET = 0;
float GYRO_X_OFFSET = 0;
float GYRO_Y_OFFSET = 0;
float GYRO_Z_OFFSET = 0;

File file; //file object used for SD card

String user_state; //user input for control over the state machine

unsigned long launch_start;
unsigned long launch_end;

typedef union{ //union used to recieve floats as bytes (chars) and then read as float
  uint32_t integer_32; //4 bytes
  uint8_t c[4]; // 1 byte * 4
}__attribute__((packed)) int32_and_char;

typedef union{ //union used to recieve floats as bytes (chars) and then read as float
  uint16_t integer_16; //2 bytes
  uint8_t c[2]; // 1 byte * 2
}__attribute__((packed)) uint16_and_char;

typedef struct{
  unsigned long current_time;
  IMU_data_t LSM6DS0_values;
  MS5611_int_digital_t MS5611_values;
  int16_t TMP177_values;
} __attribute__((packed)) data_packet;

uint32_t CURRENT_ADDRESS_DATA_LOGGING;
uint32_t MAX_ADDRESS_DATA_LOGGING = 0xFFFFFF;

uint32_t CURRENT_ADDRESS_FLASH;
uint32_t MAX_ADDRESS_FLASH = 0x3FFFFFF;


void LED_success(){
  digitalWrite(GREEN_LED_PIN, HIGH);
  delay(1000);
  digitalWrite(GREEN_LED_PIN, LOW);  //visual indication of success
}

void LED_fail(){
  digitalWrite(BLUE_LED_PIN, HIGH);
  delay(1000);
  digitalWrite(BLUE_LED_PIN, LOW);  //visual indication of fail
}




void setup(){
  //setting LED GPIO pins as output.
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  //LED sequence that the payload has been turned on
  for(uint8_t i=0; i<10; i++){
    digitalWrite(BLUE_LED_PIN, HIGH);
    delay(100);
    digitalWrite(BLUE_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    delay(100);
    digitalWrite(GREEN_LED_PIN, LOW);
  }

  Serial.begin(115200); 
  delay(2000);

  // begin Wire library if not done already
  Wire.begin();

  //begin SPI library if not done already
  SPIClass SPI_peripheral_V(VSPI);
  SPIClass SPI_peripheral_H(HSPI);
  //SPI_peripheral_V.begin(V_CLK, V_MISO, V_MOSI, CS_FLASH);
  SPI_peripheral_V.begin();
  SPI_peripheral_H.begin(H_CLK, H_MISO, H_MOSI, CS_SD);
  CS_enable(CS_DATA_LOGGING);  //enabling the CS for data logging board flash chip, W25Q128
  CS_enable(CS_FLASH);  //enabling the CS for flash board flash chip, W25Q128
  CS_enable(CS_SD);  //enabling the CS for SD card
  delay(100);

  uint8_t ret_flash_data_logging;
  uint8_t ret_flash_flash;
  uint8_t ret_LSM6DSO;
  uint8_t ret_MS5611;
  uint8_t ret_TMP177;

/*Typical state flow: INITIALIZATION --> ERASE_MEMORY --> IDLE --> LAUNCH -- POST_LAUNCH
To enter BACKUP, READ_MEMORY_DATA_LOGGING, and READ_MEMORY_FLASH states, provide user input through Serial terminal while in IDLE state
*/

  while(1){
      switch (state){



      case INITIALIZATION:
      {
        Serial.println("Entered Initialization State");

        ret_flash_data_logging = initialize_flash(CS_DATA_LOGGING, SPI_peripheral_V, CHIP_DATA_LOGGING);
        if(ret_flash_data_logging == 0){
          Serial.println("Data Logging Board Flash has been initialized.");
          LED_success();
        }else {
          Serial.println("Failed to initialize Data Logging Board Flash.");
          LED_fail();
        }
        delay(2000); //change to 10000 if needed!
        
        ret_flash_flash = initialize_flash(CS_FLASH, SPI_peripheral_V, CHIP_FLASH);
        if(ret_flash_flash == 0){
          Serial.println("Flash Board Flash has been initialized.");
          LED_success();
        }else {
          Serial.println("Failed to initialize Flash Board Flash.");
          LED_fail();
        }
        delay(2000);  //change to 10000 if needed

        ret_LSM6DSO = initialize_LSM6DSO();
        if(ret_LSM6DSO == 0){
          Serial.println("LSM6DSO has been initialized.");
          LED_success();
        }else {
          Serial.println("Failed to initialize LSM6DSO.");
          LED_fail();
        }
        delay(2000);

        ret_MS5611 = MS5611_init();
        if(ret_MS5611 == 0){
          Serial.println("MS5611 has been initialized.");
          MS5611_reset();
          MS5611_calibration_read();  //storing the calibration values
          LED_success();
        }else {
          Serial.println("Failed to initialize MS5611.");
          LED_fail();
        }
        delay(2000);

        ret_TMP177 = TMP177_init();
        if(ret_TMP177 == 0){
          Serial.println("TMP177 has been initialized.");
          LED_success();
        }else {
          Serial.println("Failed to initialize TMP177.");
          LED_fail();
        }      

      state = BACKUP;
      break;
      }


      case IDLE:
      {
        Serial.println("Entered IDLE state");
        IMU_data_t idle_IMU_data;
        three_axis_float_t idle_accel, idle_gyro;
      
        uint8_t status_ret_value = check_status_reg();
        status_ret_value = status_ret_value & 0b00000111;
        Serial.printf("Status ret value is: 0x%x\n", status_ret_value);
        if(status_ret_value == 0b111){
          read_IMU_data(idle_IMU_data);
        }

        convert_IMU_data_to_float(idle_IMU_data, idle_accel, idle_gyro);
        Serial.printf("In IDLE state, accel_z value is %f\n", idle_accel.z);
        
        if(idle_accel.z > LAUNCH_ACCEL_THRESHOLD){
          state = LAUNCH;
          break;
        }

        Serial.println("Enter State:");
        Serial.println("INITIALIZATION");
        Serial.println("IDLE");
        Serial.println("LAUNCH");
        Serial.println("POST_LAUNCH");
        Serial.println("BACKUP");
        Serial.println("READ_MEMORY_DATA_LOGGING");
        Serial.println("READ_MEMORY_DATA_FLASH");
        Serial.println("ERASE_MEMORY");

        if(Serial.available()){
          user_state = Serial.readString(); // read the incoming data as string
          Serial.println(user_state);    
          user_state.trim();
          

          if(user_state == "INITIALIZATION"){
            state = INITIALIZATION;
            Serial.println("INITIALIZATION state chosen by user.");
          }else if (user_state == "IDLE"){
            state = IDLE;
            Serial.println("IDLE state chosen by user.");
          }else if(user_state == "LAUNCH"){
            state = LAUNCH;
            Serial.println("LAUNCH state chosen by user.");
          }else if(user_state == "POST_LAUNCH"){
            state = POST_LAUNCH;
            Serial.println("POST_LAUNCH state chosen by user.");
          }else if(user_state == "BACKUP"){
            state = BACKUP;
            Serial.println("BACKUP state chosen by user.");
          }else if(user_state == "READ_MEMORY_DATA_LOGGING"){
            state = READ_MEMORY_DATA_LOGGING;
            Serial.println("READ_MEMORY_DATA_LOGGING state chosen by user.");
          }else if(user_state == "READ_MEMORY_FLASH"){
            state = READ_MEMORY_FLASH;
            Serial.println("READ_MEMORY_FLASH state chosen by user.");
          }else if(user_state == "ERASE_MEMORY"){
            state = ERASE_MEMORY;
            Serial.println("ERASE_MEMORY state chosen by user.");
          }
          else{
            Serial.println("Invalid state chosen.");
          }
        }
        delay(IDLE_WAIT);
        break;
      }


      case LAUNCH:
      {
        Serial.println("Entered Launch State");
        data_packet data;
        data_packet data_copy;
        uint8_t* data_ptr = (uint8_t*) &data;
        uint8_t* data_copy_ptr = (uint8_t*) &data_copy;
        uint32_t data_packet_size = sizeof(data_packet);
        three_axis_float_t accel, gyro;
        MS5611_int_temperature_t MS5611_temperature;
        MS5611_int_pressure_t MS5611_pressure;
        MS5611_float_t sensor_values;
        uint64_t temp_count = 0;

        launch_start = millis();

        while(millis() < launch_start + LAUNCH_DURATION){
          data.current_time = millis();

          if(ret_LSM6DSO==0){
            uint8_t LSM6DS0_status_byte = check_status_reg();
            if(LSM6DS0_status_byte & 0b00000111 == 0b111){
              read_IMU_data(data.LSM6DS0_values);
              /*convert_IMU_data_to_float(data.LSM6DS0_values, accel, gyro);
              Serial.println("\n\ngyro x, gyro y, gyro z, accel x, accel y, accel z");
              Serial.printf("%f, %f, %f, %f, %f, %f\n", gyro.x, gyro.y, gyro.z, accel.x, accel.y, accel.z);*/ //Conversion is slowing down the reading
            }
          }

          if(ret_MS5611==0){
            data.MS5611_values = MS5611_read();

            /*MS5611_temperature = MS5611_calculate_temp(data.MS5611_values);
            MS5611_pressure = MS5611_calculate_pressure(data.MS5611_values, MS5611_temperature);
            MS5611_second_order_analysis(&MS5611_temperature, &MS5611_pressure);
            sensor_values = MS5611_convert_to_float(MS5611_temperature, MS5611_pressure);

            
            Serial.println("TEMP, PRESSURE");
            Serial.printf("%f, %f\n", sensor_values.temperature, sensor_values.pressure);      

            Serial.println("Read barometer data");*/ //Conversion is slowing down the reading
          }

          if(ret_TMP177==0){
            data.TMP177_values = TMP177_data_read();
            /*Serial.println("TMP177 TEMP");
            Serial.printf("%f\n", TMP177_convert_to_float(data.TMP177_values));
         
            Serial.println("Read TMP177 data");*/ //Conversion is slowing down the reading
          }

          data_copy = data; //copied data for the second flash to be able to store the same data


          if((ret_flash_data_logging == 0) && ((CURRENT_ADDRESS_DATA_LOGGING + data_packet_size)<=MAX_ADDRESS_DATA_LOGGING)){
            CURRENT_ADDRESS_DATA_LOGGING = write_to_flash(CS_DATA_LOGGING, SPI_peripheral_V, CURRENT_ADDRESS_DATA_LOGGING, data_ptr, data_packet_size, CHIP_DATA_LOGGING);
          }

          if((ret_flash_flash == 0) && ((CURRENT_ADDRESS_FLASH + data_packet_size)<=MAX_ADDRESS_FLASH)){
            CURRENT_ADDRESS_FLASH = write_to_flash(CS_FLASH, SPI_peripheral_V, CURRENT_ADDRESS_FLASH, data_copy_ptr, data_packet_size, CHIP_FLASH);
            //temp_CURRENT_ADDRESS_FLASH = CURRENT_ADDRESS_FLASH;
            //write_to_flash(CS_FLASH, SPI_peripheral_V, 0, temp_CURRENT_ADDRESS_FLASH_ptr, 4, CHIP_FLASH, ADDRESS_UPDATE);
            //Serial.printf("CURRENT_ADDRESS_FLASH is 0x%x",CURRENT_ADDRESS_FLASH);
            //Serial.println("Wrote to flash board flash memory");
          }

          Serial.printf("Wrote %u times\n", temp_count);
          temp_count++;
          //delay(2000);
        }

        state = POST_LAUNCH;
        launch_end = millis();
        Serial.printf("26MHz Launch start is %lu\n", launch_start);
        Serial.printf("Launch end is %lu\n", launch_end);
        Serial.printf("Launch end - launch start is %lu\n", launch_end-launch_start);
        delay(50000);
        break;
      }



      case POST_LAUNCH:
      {
        Serial.println("Entered POST_LAUNCH state");
        SPI_peripheral_V.end();
        SPI_peripheral_H.end();
        state = IDLE;
        break;
      }


      case BACKUP:
      {
        Serial.println("Entered Backup State");


        if(!(SD.begin(CS_SD, SPI_peripheral_H, 26000000))){
          Serial.println("Failed Card Mount.");
          state = ERASE_MEMORY;
          break;
        }
        Serial.println("Card Mount Succeeded");


        if(ret_flash_data_logging == 0){
          Serial.println("Saving data from data logging board's flash chip");

          uint64_t file_number_read_bytes_length;

          file = SD.open("/file_system/file_number_DB.txt", FILE_READ);
          if(!file){
            Serial.println("File failed to open for reading (1)");
            state = IDLE;
            break;
          }
          Serial.println("File opened for reading (1)");
          file_number_read_bytes_length = file.available();
          
          uint8_t buf_file_number[file_number_read_bytes_length];    
          file.read(buf_file_number, file_number_read_bytes_length);

          char file_name_str[file_number_read_bytes_length+7]; //7 extray bytes for: 1 for "/", 2 for "DB" (Data Logging Board Flash chip's data) or "FB" (Flash Board Flash chip's data), 4 for "".txt"
          char* file_name_ptr = file_name_str;

          Serial.printf("Number is %c", buf_file_number[0]);

          sprintf(file_name_ptr, "/");
          for(uint8_t i=0; i<file_number_read_bytes_length; i++){
            sprintf(file_name_ptr+i+1, "%c",buf_file_number[i]);
          }
          file.close();

          uint64_t next_file_number = atoi(file_name_ptr+1)+1;
          Serial.printf("next number is %lu", next_file_number);

          file = SD.open("/file_system/file_number_DB.txt", FILE_WRITE);
          if(!file){
            Serial.println("File failed to open for reading (2)");
            state = IDLE;
            break;
          }
          Serial.println("File opened for reading (2)");
          file.print(next_file_number);
          file.close();
      
          sprintf(file_name_ptr+1+file_number_read_bytes_length, "DB.txt");          

          file = SD.open((const char *)file_name_str, FILE_APPEND);
          if(!file){
            Serial.println("Failed in opening file one for writing. (3)");
            state = IDLE;
            break;
          }
          Serial.println("Succeeded in opening file three for writing. (3)");

          file.print("Data Logging Board Data\n");
          file.print("Time Stamp, Gyro X, Gyro Y, Gyro Z, Accel X, Accel Y, Accel Z, MS5611 TEMP, PRES, TMP177 TEMP\n");
          file.print("\n");


          uint8_t data_read_size = sizeof(data_packet);
          data_packet data_read;
          uint8_t* data_read_ptr = (uint8_t*) &data_read;
          three_axis_float_t accel, gyro;
          MS5611_int_temperature_t MS5611_temperature;
          MS5611_int_pressure_t MS5611_pressure;
          MS5611_float_t sensor_values;
          float TMP177_temperature;
          uint8_t data_end = 1;
          uint32_t count = 0;
          uint32_t next_read;

          while(data_end){
            next_read = data_read_size*count;
            if(next_read+data_read_size > MAX_ADDRESS_DATA_LOGGING){
              data_end = 0;
            }else{
                read_from_flash(CS_DATA_LOGGING, SPI_peripheral_V, (data_read_size*count), data_read_ptr, data_read_size, CHIP_DATA_LOGGING);
                if(data_read.LSM6DS0_values.accel.x == 0xFFFFFFFF && data_read.LSM6DS0_values.accel.y == 0xFFFFFFFF && data_read.LSM6DS0_values.accel.z == 0xFFFFFFFF && data_read.LSM6DS0_values.gyro.x == 0xFFFFFFFF && data_read.LSM6DS0_values.gyro.y == 0xFFFFFFFF && data_read.LSM6DS0_values.gyro.z == 0xFFFFFFFF && data_read.MS5611_values.D1 == 0xFFFFFFFF && data_read.MS5611_values.D2 == 0xFFFFFFFF){
                  data_end = 0; //break out of while loop
                }else{
                    file.print(data_read.current_time);
                    file.print(",");

                    if(ret_LSM6DSO==0){
                      convert_IMU_data_to_float(data_read.LSM6DS0_values, accel, gyro);
                      file.print(gyro.x, 4);
                      file.print(",");
                      file.print(gyro.y, 4);
                      file.print(",");
                      file.print(gyro.z, 4);
                      file.print(",");
                      file.print(accel.x, 4);
                      file.print(",");
                      file.print(accel.y, 4);
                      file.print(",");
                      file.print(accel.z, 4);
                      file.print(",");
                  }else{
                    file.print("-,-,-,-,-,-,");
                  }

                  if(ret_MS5611==0){
                    MS5611_temperature = MS5611_calculate_temp(data_read.MS5611_values);
                    MS5611_pressure = MS5611_calculate_pressure(data_read.MS5611_values, MS5611_temperature);
                    MS5611_second_order_analysis(&MS5611_temperature, &MS5611_pressure);
                    sensor_values = MS5611_convert_to_float(MS5611_temperature, MS5611_pressure);

                    
                    file.print(sensor_values.temperature, 2);
                    file.print(",");
                    file.print(sensor_values.pressure, 2);
                    file.print(",");
                  }else{
                    file.print("-,-,");
                  }

                  if(ret_TMP177==0){
                    TMP177_temperature = TMP177_convert_to_float(data_read.TMP177_values);

                    file.print(TMP177_temperature, 2);
                    file.print("\n");           
                  }else{
                    file.print("-\n");
                  }   
                  Serial.printf("Read to SD %u times\n", count);
                  count++;
              }
            }
          }       
        file.close();     
        }
        



        if(ret_flash_flash == 0){
          Serial.println("Saving data from flash board's flash chip");

          uint64_t file_number_read_bytes_length;

          file = SD.open("/file_system/file_number_FB.txt", FILE_READ);
          if(!file){
            Serial.println("File failed to open for reading (1)");
            state = IDLE;
            break;
          }
          Serial.println("File opened for reading (1)");
          file_number_read_bytes_length = file.available();
          
          uint8_t buf_file_number[file_number_read_bytes_length];    
          file.read(buf_file_number, file_number_read_bytes_length);

          char file_name_str[file_number_read_bytes_length+7]; //7 extray bytes for: 1 for "/", 2 for "DB" (Data Logging Board Flash chip's data) or "FB" (Flash Board Flash chip's data), 4 for "".txt"
          char* file_name_ptr = file_name_str;

          Serial.printf("Number is %c", buf_file_number[0]);

          sprintf(file_name_ptr, "/");
          for(uint8_t i=0; i<file_number_read_bytes_length; i++){
            sprintf(file_name_ptr+i+1, "%c",buf_file_number[i]);
          }
          file.close();

          uint64_t next_file_number = atoi(file_name_ptr+1)+1;
          Serial.printf("next number is %lu", next_file_number);

          file = SD.open("/file_system/file_number_FB.txt", FILE_WRITE);
          if(!file){
            Serial.println("File failed to open for reading (2)");
            state = IDLE;
            break;
          }
          Serial.println("File opened for reading (2)");
          file.print(next_file_number);
          file.close();
      
          sprintf(file_name_ptr+1+file_number_read_bytes_length, "FB.txt");

          file = SD.open((const char *)file_name_str, FILE_APPEND);
          if(!file){
            Serial.println("Failed in opening file one for writing. (3)");
            state = IDLE;
            break;
          }
          Serial.println("Succeeded in opening file three for writing. (3)");

          file.print("Flash Board Data\n");
          file.print("Time Stamp, Gyro X, Gyro Y, Gyro Z, Accel X, Accel Y, Accel Z, MS5611 TEMP, PRES, TMP177 TEMP\n");
          file.print("\n");


          uint8_t data_read_size = sizeof(data_packet);
          data_packet data_read;
          uint8_t* data_read_ptr = (uint8_t*) &data_read;
          three_axis_float_t accel, gyro;
          MS5611_int_temperature_t MS5611_temperature;
          MS5611_int_pressure_t MS5611_pressure;
          MS5611_float_t sensor_values;
          float TMP177_temperature;
          uint8_t data_end = 1;
          uint32_t count = 0;
          uint32_t next_read;

          while(data_end){
            next_read = data_read_size*count;

            if(next_read+data_read_size > MAX_ADDRESS_FLASH){
              data_end = 0;
            }else{
              read_from_flash(CS_FLASH, SPI_peripheral_V, (data_read_size*count), data_read_ptr, data_read_size, CHIP_FLASH);
              if(data_read.LSM6DS0_values.accel.x == 0xFFFFFFFF && data_read.LSM6DS0_values.accel.y == 0xFFFFFFFF && data_read.LSM6DS0_values.accel.z == 0xFFFFFFFF && data_read.LSM6DS0_values.gyro.x == 0xFFFFFFFF && data_read.LSM6DS0_values.gyro.y == 0xFFFFFFFF && data_read.LSM6DS0_values.gyro.z == 0xFFFFFFFF && data_read.MS5611_values.D1 == 0xFFFFFFFF && data_read.MS5611_values.D2 == 0xFFFFFFFF){
                data_end = 0; //break out of while loop
              }else{
                  file.print(data_read.current_time);
                  file.print(",");

                  if(ret_LSM6DSO==0){
                    convert_IMU_data_to_float(data_read.LSM6DS0_values, accel, gyro);
                    file.print(gyro.x, 4);
                    file.print(",");
                    file.print(gyro.y, 4);
                    file.print(",");
                    file.print(gyro.z, 4);
                    file.print(",");
                    file.print(accel.x, 4);
                    file.print(",");
                    file.print(accel.y, 4);
                    file.print(",");
                    file.print(accel.z, 4);
                    file.print(",");
                  }else{
                    file.print("-,-,-,-,-,-,");
                  }

                  if(ret_MS5611==0){
                    MS5611_temperature = MS5611_calculate_temp(data_read.MS5611_values);
                    MS5611_pressure = MS5611_calculate_pressure(data_read.MS5611_values, MS5611_temperature);
                    MS5611_second_order_analysis(&MS5611_temperature, &MS5611_pressure);
                    sensor_values = MS5611_convert_to_float(MS5611_temperature, MS5611_pressure);

                    
                    file.print(sensor_values.temperature, 2);
                    file.print(",");
                    file.print(sensor_values.pressure, 2);
                    file.print(",");
                  }else{
                    file.print("-,-,");
                  }
                  
                  if(ret_TMP177==0){
                    TMP177_temperature = TMP177_convert_to_float(data_read.TMP177_values);

                    file.print(TMP177_temperature, 2);
                    file.print("\n");
                  }else{
                    file.print("-\n");
                  }   
                  Serial.printf("Read to SD %u times\n", count);
                  count++;
              }
            }
          }         
        file.close();     
        }
        state = ERASE_MEMORY;
        break;
      }



      case READ_MEMORY_DATA_LOGGING:
      {
        Serial.println("Entered READ_MEMORY_DATA_LOGGING state");

        if(ret_flash_data_logging == 0){
          data_packet data_read;
          uint8_t* data_read_ptr = (uint8_t*) &data_read;
          uint32_t data_read_size = sizeof(data_packet);
          three_axis_float_t accel, gyro;
          MS5611_int_temperature_t MS5611_temperature;
          MS5611_int_pressure_t MS5611_pressure;
          MS5611_float_t sensor_values;
          float TMP177_temperature;
          uint8_t data_end = 1;
          uint32_t count = 0;
          uint32_t next_read;

          while(data_end){
            next_read = data_read_size*count;

            if(next_read+data_read_size > MAX_ADDRESS_DATA_LOGGING){
              data_end = 0;
            }else{
              read_from_flash(CS_DATA_LOGGING, SPI_peripheral_V, (data_read_size*count), data_read_ptr, data_read_size, CHIP_DATA_LOGGING);
              if(data_read.LSM6DS0_values.accel.x == 0xFFFFFFFF && data_read.LSM6DS0_values.accel.y == 0xFFFFFFFF && data_read.LSM6DS0_values.accel.z == 0xFFFFFFFF && data_read.LSM6DS0_values.gyro.x == 0xFFFFFFFF && data_read.LSM6DS0_values.gyro.y == 0xFFFFFFFF && data_read.LSM6DS0_values.gyro.z == 0xFFFFFFFF && data_read.MS5611_values.D1 == 0xFFFFFFFF && data_read.MS5611_values.D2 == 0xFFFFFFFF){
                  data_end = 0; //break out of while loop
              }else{
                Serial.printf("Time Stamp: %lu\n", data_read.current_time);

                if(ret_LSM6DSO==0){
                  convert_IMU_data_to_float(data_read.LSM6DS0_values, accel, gyro);
                  Serial.println("gyro x, gyro y, gyro z, accel x, accel y, accel z");
                  Serial.printf("%f, %f, %f, %f, %f, %f\n", gyro.x, gyro.y, gyro.z, accel.x, accel.y, accel.z);
                }else{
                  Serial.print("-,-,-,-,-,-,");
                }

                if(ret_MS5611==0){
                  MS5611_temperature = MS5611_calculate_temp(data_read.MS5611_values);
                  MS5611_pressure = MS5611_calculate_pressure(data_read.MS5611_values, MS5611_temperature);
                  MS5611_second_order_analysis(&MS5611_temperature, &MS5611_pressure);
                  sensor_values = MS5611_convert_to_float(MS5611_temperature, MS5611_pressure);

                  Serial.println("MS5611 TEMP, PRESS");
                  Serial.printf("%f, %f\n", sensor_values.temperature, sensor_values.pressure);       
                }else{
                  Serial.print("-,-\n");
                }   

                if(ret_TMP177==0){
                  TMP177_temperature = TMP177_convert_to_float(data_read.TMP177_values);
                  
                  Serial.println("TMP177 TEMP");
                  Serial.printf("%f\n", TMP177_temperature);       
                }else{
                  Serial.print("-\n");
                }   


                Serial.printf("Read %u times\n", count);
                count++;       
              }
            }
          }
        }

        state = IDLE;
        break;
      }



      case READ_MEMORY_FLASH:
      {
        Serial.println("Entered READ_MEMORY_FLASH state");

        if(ret_flash_flash == 0){
          data_packet data_read;
          uint8_t* data_read_ptr = (uint8_t*) &data_read;
          uint32_t data_read_size = sizeof(data_packet);
          three_axis_float_t accel, gyro;
          MS5611_int_temperature_t MS5611_temperature;
          MS5611_int_pressure_t MS5611_pressure;
          MS5611_float_t sensor_values;
          float TMP177_temperature;
          uint8_t data_end = 1;
          uint32_t count = 0;
          uint32_t next_read;

         
          while(data_end){
            if(next_read+data_read_size > MAX_ADDRESS_DATA_LOGGING){
              data_end = 0;
            }else{
              read_from_flash(CS_FLASH, SPI_peripheral_V, (data_read_size*count), data_read_ptr, data_read_size, CHIP_FLASH);
              if(data_read.LSM6DS0_values.accel.x == 0xFFFFFFFF && data_read.LSM6DS0_values.accel.y == 0xFFFFFFFF && data_read.LSM6DS0_values.accel.z == 0xFFFFFFFF && data_read.LSM6DS0_values.gyro.x == 0xFFFFFFFF && data_read.LSM6DS0_values.gyro.y == 0xFFFFFFFF && data_read.LSM6DS0_values.gyro.z == 0xFFFFFFFF && data_read.MS5611_values.D1 == 0xFFFFFFFF && data_read.MS5611_values.D2 == 0xFFFFFFFF){
                data_end = 0; //break out of while loop
              }else{
              Serial.printf("Time Stamp: %lu\n", data_read.current_time);

              if(ret_LSM6DSO==0){
                convert_IMU_data_to_float(data_read.LSM6DS0_values, accel, gyro);
                Serial.println("gyro x, gyro y, gyro z, accel x, accel y, accel z");
                Serial.printf("%f, %f, %f, %f, %f, %f\n", gyro.x, gyro.y, gyro.z, accel.x, accel.y, accel.z);
              }else{
                Serial.print("-,-,-,-,-,-,");
              }

              if(ret_MS5611==0){
                MS5611_temperature = MS5611_calculate_temp(data_read.MS5611_values);
                MS5611_pressure = MS5611_calculate_pressure(data_read.MS5611_values, MS5611_temperature);
                MS5611_second_order_analysis(&MS5611_temperature, &MS5611_pressure);
                sensor_values = MS5611_convert_to_float(MS5611_temperature, MS5611_pressure);

                Serial.println("MS5611 TEMP, PRESS");
                Serial.printf("%f, %f\n", sensor_values.temperature, sensor_values.pressure);       
              }else{
                Serial.print("-,-,\n");
              }   

              if(ret_TMP177==0){
                TMP177_temperature = TMP177_convert_to_float(data_read.TMP177_values);
                
                Serial.println("TMP177 TEMP");
                Serial.printf("%f\n", TMP177_temperature);       
              }else{
                Serial.print("-\n");
              }   


              Serial.printf("Read %u times\n", count);
              count++;       
            
              }
            }
          }
        }

        state = IDLE;
        break;
      }



      case ERASE_MEMORY:
      {
        Serial.println("Entered Erase Memory state");


        if(ret_flash_data_logging == 0){
          CURRENT_ADDRESS_DATA_LOGGING = chip_erase(CS_DATA_LOGGING, SPI_peripheral_V);
          if(CURRENT_ADDRESS_DATA_LOGGING==0){
            Serial.println("Successfully erased Data Logging Board Flash Chip memory");
            LED_success();
          } else{
            Serial.println("Failed to erase Data Logging Board Flash Chip memory");
            LED_fail();
          } 
        }  //visual indication that data logging board's flash chip erased successfully or failed    
        
      
        if(ret_flash_flash == 0){
          CURRENT_ADDRESS_FLASH = chip_erase(CS_FLASH, SPI_peripheral_V);
          if(CURRENT_ADDRESS_FLASH==0){
            Serial.println("Sucessfully erased Flash Board Flash Chip memory");
            LED_success();
          } else{
            Serial.println("Failed to erase Flash Board flash Chip memory");
            LED_fail();
          } //visual indication that flash board's flash chip erased successfully or failed
        }
    
        state = IDLE;
        break;
      }



      default:
      {
        Serial.println("Invalid State");
        LED_fail();
        break;
      }


    }
  }
}

void loop() {

}





/*
      uint8_t current_address = 0;
      IMU_data_t IMU_data_send, IMU_data_receive;
      uint32_t IMU_data_size = sizeof(IMU_data_t);
      three_axis_float_t accel, gyro;
      union test{
        float flt;
        uint8_t c[4];
      } data_receive;
      float d_value;
      char buffer_test[50];
      uint32_t float_size = sizeof(float);

      uint8_t LSM6DS0_status_byte = check_status_reg();
        if(LSM6DS0_status_byte & 0b00000111 == 0b111){
          read_IMU_data(IMU_data_send);
        }
      
      convert_IMU_data_to_float(IMU_data_send, accel, gyro);
      Serial.println("gyro x, gyro y, gyro z, accel x, accel y, accel z");
      Serial.printf("%f, %f, %f, %f, %f, %f\n", gyro.x, gyro.y, gyro.z, accel.x, accel.y, accel.z);

      current_address = write_to_flash(CS_DATA_LOGGING, SPI_peripheral_V, current_address, (uint8_t*) &gyro.x, float_size, CHIP_DATA_LOGGING, DATA_UPDATE);
      current_address = write_to_flash(CS_DATA_LOGGING, SPI_peripheral_V, current_address, (uint8_t*) &gyro.y, float_size, CHIP_DATA_LOGGING, DATA_UPDATE);
      current_address = write_to_flash(CS_DATA_LOGGING, SPI_peripheral_V, current_address, (uint8_t*) &gyro.z, float_size, CHIP_DATA_LOGGING, DATA_UPDATE);
      current_address = write_to_flash(CS_DATA_LOGGING, SPI_peripheral_V, current_address, (uint8_t*) &accel.x, float_size, CHIP_DATA_LOGGING, DATA_UPDATE);
      current_address = write_to_flash(CS_DATA_LOGGING, SPI_peripheral_V, current_address, (uint8_t*) &accel.y, float_size, CHIP_DATA_LOGGING, DATA_UPDATE);
      current_address = write_to_flash(CS_DATA_LOGGING, SPI_peripheral_V, current_address, (uint8_t*) &accel.z, float_size, CHIP_DATA_LOGGING, DATA_UPDATE);

    
      uint32_t address_read = 0;
      uint8_t test[4];
      read_from_flash(CS_DATA_LOGGING, SPI_peripheral_V, address_read, test, float_size, CHIP_DATA_LOGGING);
      for(int i=0; i<4; i++){
        Serial.printf("value at %d is 0x%x\n\n\n", i, test[i]);
      }
      d_value = data_receive.flt;
      dtostrf(d_value,3,3,buffer_test);
      Serial.printf("\n x_gyro: %s\n", buffer_test);

      address_read = address_read + float_size;
      read_from_flash(CS_DATA_LOGGING, SPI_peripheral_V, address_read, data_receive.c, float_size, CHIP_DATA_LOGGING);
      d_value = data_receive.flt;
      dtostrf(d_value,3,3,buffer_test);
      Serial.printf("\n y_gyro: %s\n", buffer_test);

      address_read = address_read + float_size;
      read_from_flash(CS_DATA_LOGGING, SPI_peripheral_V, address_read, data_receive.c, float_size, CHIP_DATA_LOGGING);
      d_value = data_receive.flt;
      dtostrf(d_value,3,3,buffer_test);
      Serial.printf("\n z_gyro: %s\n", buffer_test);

      address_read = address_read + float_size;
      read_from_flash(CS_DATA_LOGGING, SPI_peripheral_V, address_read, data_receive.c, float_size, CHIP_DATA_LOGGING);
      d_value = data_receive.flt;
      dtostrf(d_value,3,3,buffer_test);
      Serial.printf("\n x_accel: %s\n", buffer_test);
      
      address_read = address_read + float_size;
      read_from_flash(CS_DATA_LOGGING, SPI_peripheral_V, address_read, data_receive.c, float_size, CHIP_DATA_LOGGING);
      d_value = data_receive.flt;
      dtostrf(d_value,3,3,buffer_test);
      Serial.printf("\n y_accel: %s\n", buffer_test);

      address_read = address_read + float_size;
      read_from_flash(CS_DATA_LOGGING, SPI_peripheral_V, address_read, data_receive.c, float_size, CHIP_DATA_LOGGING);
      d_value = data_receive.flt;
      dtostrf(d_value,3,3,buffer_test);
      Serial.printf("\n z_accel: %s\n", buffer_test);

      delay(10000);
*/



































































