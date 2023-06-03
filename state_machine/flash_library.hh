/**
PAYLOAD FLASH LIBRARY

Intended for use with Iris I and II

In reference to the W25Q128 and W25Q512 datasheets:
https://www.winbond.com/resource-files/W25Q512JV%20SPI%20RevB%2006252019%20KMS.pdf
https://www.winbond.com/resource-files/w25q128jv%20spi%20revc%2011162016.pdf
https://www.mouser.ca/datasheet/2/698/REN_DS_AT25SF321B_179F_062022_DST_20220603_1-3075940.pdf

All functions require an SPI peripheral from the ESP32, this is how you would initialize one
SPIClass SPI_FLASH(VSPI); or HSPI
some_func(SPI1);

INCLUDED FUNCTIONS:
initialize_flash()
write_to_flash()
read_from_flash()
chip_erase()
write_enable()
busy_bit_check()
address_maanager()
*/
#ifndef FLASH_LIBRARY_H
#define FLASH_LIBRARY_H

#include "Arduino.h"
#include "SPI.h"

#define ADD 0
#define ERASE 1

enum CHIP_TYPE {
  W25Q512,
  W25Q128,
  AT25SF
};


/**
* @brief Will check that the flash chip is returning the correct ID, it will also enable the flash chip's write mode.
*
* @param CS_pin The chip select pin for the flash chip.
* @param SPI_peripheral The SPI peripheral to use.
* @param chip_type Accepts 128 or 512, as those are the only two chips we are using (using 312 for prototyping).
* @return 0 if success, 1 otherwise
*/
uint8_t initialize_flash(uint8_t CS_pin, SPIClass SPI_peripheral, CHIP_TYPE CHIP);

/**
* @brief Writes the number of given bytes in a buffer of 8-bit integers.
*
* @param CS_pin The chip select pin fpr the flash chip.
* @param SPI_peripheral The SPI peripheral to use.
* @param current_address Accepts up to a 32 bit address to start writing at.
* @param buffer The buffer that contains the data to be written to flash memory.
* @param number_of_bytes The number of bytes to be stored in flash memory.
* @param CHIP The type of flash chip being used.
* @param state The state in which the write_to_flash function is being used, i.e. to update the last address written to, or to write more data to the flash chip.
*
* @return The last address to which data was written to + 1 to keep track of the address in flash memory
*/
uint32_t write_to_flash(uint8_t CS_pin, SPIClass SPI_peripheral, uint32_t current_address, uint8_t* buffer, uint32_t number_of_bytes, CHIP_TYPE CHIP);

/**
* @brief Reads read_size data from flash memory starting at given address.
*
* @param CS_pin The chip select pin for the flash chip.
* @param SPI_peripheral The SPI peripheral to use.
* @param address_start The starting address to read data from.
* @param data The array that stores the read data.
* @param read_size The number of bytes to be read.
*/
void read_from_flash(uint8_t CS_pin, SPIClass SPI_peripheral, uint32_t address_start, uint8_t* data, uint32_t read_size, CHIP_TYPE CHIP);

/**
* @brief Erases all of the data in the flash chip
*
* @param CS_pin The chip select pin for the flash chip.
* @param SPI_peripheral The SPI peripheral to use.
*
* @return The address to start writing to after the flash chip's data has been erased.
*/
uint32_t chip_erase(uint8_t CS_pin, SPIClass SPI_peripheral);

/**
* @brief Function used to monitor the current address.
*
* @param current_address The previous point in flash memory before data was written or erased.
* @param bytes_written The number of bytes that were written.
* @param state Determines if chip is to be erased (ERASE) or if data was written and the point in flash memory is to be incremented (ADD)
*/
uint32_t address_manager(uint32_t previous_address, uint32_t bytes_written, uint8_t state);

/**
* @brief Enables the WEL bit.
*
* @param CS_pin The chip select pin for the flash chip.
* @param SPI_peripheral The SPI peripheral to use.
*/
void write_enable(uint8_t CS_pin, SPIClass SPI_peripheral);

/**
* @brief Polling the BUSY bit before progressing to other parts of the software.
*
* @param CS_pin The chip select pin for the flash chip.
* @param SPI_peripheral The SPI peripheral to use.
*
*/
int busy_bit_check(uint8_t CS_pin, SPIClass SPI_peripheral);

/**
* @brief Enabling chip select pin as an output.
* @param CS_pin The chosen pin number for the chip select function.
*/
void CS_enable(uint8_t CS_pin);

#endif
