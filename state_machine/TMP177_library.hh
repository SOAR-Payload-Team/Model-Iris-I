/**
  This is a library for the TMP177 IC present on IRIS I and IRIS II.
*/
#ifndef TMP177_LIBRARY_H
#define TMP177_LIBRARY_H

#include "Arduino.h"
#include "Wire.h"

#define TMP177_ADDRESS 0x48 //ADD0 is connected to ground (refer to datasheet)
#define DEVICE_ID_REGISTER 0x0F
#define CONFIG_REGISTER 0x01
#define DATA_REGISTER 0x00
#define DEVICE_ID 0x0117

#define RESTART_BIT 0
#define END_BIT 1

/**
* @brief Will collect the device ID of the TMP177 to ensure that that the IC can be accessed.
*/
uint8_t TMP177_init();

/**
* @brief Will collect the digital temperature data.
* @return The digital value of the temperature data as a signed 16 bit integer.
*/
int16_t TMP177_data_read();

/**
* @brief Checks the status of the data ready bit.
* @return The data ready bit's value (1 means data is read, 0 means conversion has not been completed yet)
*/
uint16_t TMP177_data_ready();


/**
* @brief Will convert the digital read to a float to display.
* @return The temperature as a float.
*/
float TMP177_convert_to_float(int16_t data_digital);


#endif