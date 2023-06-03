#ifndef SD_LIBRARY_H
#define SD_LIBRARY_H

#include "SD.h"
#include <string.h>
#include <stdlib.h>
#include "flash_library.hh"

extern uint8_t CS_SD;
extern SPIClass SPI_peripheral_H;

uint8_t SD_init(uint8_t CS_SD, SPIClass SPI_peripheral_H);

uint8_t file_number_read_bytes(File file, uint64_t* file_number_read_bytes_value);

uint8_t create_file_name(File file, char* file_name_str, uint64_t file_number_read_bytes_value, CHIP_TYPE CHIP);

uint8_t increment_file(File file);

//uint8_t write_to_SD(uint8_t CS_SD, SPIClass SPI_peripheral, File file, CHIP_TYPE CHIP);


#endif

