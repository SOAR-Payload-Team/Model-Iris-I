/*
This is a library for the MS5611 barometer IC present on IRIS I and IRIS II.
*/

#ifndef MS5611_LIBRARY_H
#define MS5611_LIBRARY_H

#include "Arduino.h"
#include <Wire.h>

#define MS5611_ADDRESS 0x77 //CSB is connected to ground, thus /CSB=1. W = 0
#define MS5611_RESET 0x1E
#define PROM_READ 0b10100000 //bit 3 to 1 to be modified based on the value being retrieved
#define CONVERSION_PRESSURE 0b01000000  //Pressure value at OSR=256
#define CONVERSION_TEMPERATURE 0b01010000  //Temperature value at OSR=256
#define ADC_READ 0b00000000

#define RESTART_BIT 0
#define END_BIT 1

typedef struct{
  uint32_t D1;
  uint32_t D2;
}__attribute__((packed)) MS5611_int_digital_t;


typedef struct{
  int32_t dT;
  int32_t TEMP;
}MS5611_int_temperature_t;

typedef struct{
  int64_t OFF;
  int64_t SENS;
  int64_t P;
}MS5611_int_pressure_t;

typedef struct{
  float temperature;
  float pressure;
}MS5611_float_t;

extern uint16_t C[6]; //array to store the calibration values for the MS5611

/**
* @brief Will determine if MS5611 is responding.
* @return Return value from Wire.endTransmission() function.
*/
uint8_t MS5611_init();

/**
* @brief Will reset the barometer chip.
*/
void MS5611_reset();

/**
* @brief Will read the calibration factors from the barometer chip.
* The factory calibration values are stored in a global variable.
*/
void MS5611_calibration_read();


/**
* @brief Will initiate conversion sequence for the temperature and pressure sensors.
* @return The digital values from the ADC read.
*/
MS5611_int_digital_t MS5611_read();

/**
* @brief Will calculate temperature based on digital temperature value. Calculations are determined through datasheet.
* @param MS5611_digital_reads The ADC read values.
* @return Integer values for the temperature.
*/
MS5611_int_temperature_t MS5611_calculate_temp(MS5611_int_digital_t MS5611_digital_reads);

/**
* @brief Will calculate temperature compensated pressure. Calculations are determined through datasheet.
* @param MS5611_digital_reads The ADC read values.
* @param MS5611_pressure Integer values for the pressure.
* @return Integer values for the pressure.
*/
MS5611_int_pressure_t MS5611_calculate_pressure(MS5611_int_digital_t MS5611_digital_reads, MS5611_int_temperature_t MS5611_temperature);

/**
* @brief Will perform a second order analysis of the MS5611's readings to optimize the values. Calculations are determined through datasheet.
* @param MS5611_temperature Integer values for the temeprature.
* @param MS5611_pressure Integer values for the pressure.
* @return The temperature and terms used to calculate the temperature as well as pressure through a second order analysis.
* These values are changed within the original structure itself that is passed as a pointer to the function.
*/
void MS5611_second_order_analysis(MS5611_int_temperature_t* MS5611_temperature, MS5611_int_pressure_t* MS5611_pressure);


/**
* @brief Will convert integer values to temperature (degrees Celsius) and pressure (mbar). Calculations are determined through datasheet.
* @param MS5611_temperature Integer values for the temeprature.
* @param MS5611_pressure Integer values for the pressure.
* @return The temperature (degrees Celsius) and pressure (mbar) values.
*/
MS5611_float_t MS5611_convert_to_float(MS5611_int_temperature_t MS5611_temperature, MS5611_int_pressure_t MS5611_pressure);

#endif
