#include "SPI.h"
#include "Arduino.h"
/** 

PAYLOAD FLASH LIBRARY

Intended for use with Iris I and II

In reference to the W25Q128 and W25Q512 datasheets:
https://www.winbond.com/resource-files/W25Q512JV%20SPI%20RevB%2006252019%20KMS.pdf
https://www.winbond.com/resource-files/w25q128jv%20spi%20revc%2011162016.pdf


// All functions require an SPI peripheral from the ESP32, this is how you would initialize one
SPIClass SPI_FLASH(VSPI); // or HSPI
some_func(SPI1);

INCLUDED FUNCTIONS:
initialize_flash()
write_to_flash()
read_from_flash()
erase_flash()

*/

/**
* @brief Will check that the flash chip is returning the correct ID, it will also enable the flash chip's write mode.
*
* @param CS_pin The chip select pin of the flash chip
* @param SPI_peripheral The SPI peripheral to use
* @param chip_type Accepts 128 or 512, as those are the only two chips we are using
*
* @return 1 for correct ID, 0 for incorrect 
*/
uint8_t initialize_flash(int CS_pin, SPIClass SPI_peripheral, int chip_type) {
    
    // begin the transaction
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first
    
    // send a command to return the chip's IDs
    SPI_peripheral.transfer(0x9F); // 9Fh is the get JEDEC ID command
    
    uint8_t manufacturer_ID = SPI_peripheral.transfer(0x00); // first byte returned is the manufacturer ID
    uint16_t device_ID = SPI_peripheral.transfer16(0x0000); // next two bytes are the device ID
    
    // end the transaction
    SPI_peripheral.endTransaction();
    digitalWrite(CS_pin, HIGH);
    
    delay(1);
    
    // enable the write mode
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0));
    SPI_peripheral.transfer(0x06); // op-code for write enable
    SPI_peripheral.endTransaction();
    digitalWrite(CS_pin, HIGH);
    
    // manufacturer ID is 0xEF, chip ID for W25Q128 is 0x4018, chip ID for W25Q512 is 0x4020
    uint16_t chip_ID = 0x1111; // a non-ID
    if (chip_type == 128) chip_ID = 0x4018;
    else if (chip_type == 512) chip_ID = 0x4020;
    
    return (manufacturer_ID == 0xEF || device_ID == chip_ID) ? 1 : 0;
}

/**
* @brief Writes the number of given bytes in a buffer of 8-bit integers
*/
uint8_t write_to_flash(int CS_pin, SPIClass SPI_peripheral, uint32_t address, uint8_t* buffer, int number_of_bytes) {
    uint8_t page_program_opcode = 0;
    
    // checks if address is 4 bytes or 3 bytes
    if (0xFF000000 & address) {
        page_program_opcode = 0x12; // for 4 byte address (W25Q512 only)
    } else {
        page_program_opcode = 0x02; // for 3 byte address
    }
    
    // begin the transaction
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first
 
    // add some stuff here
    
    for (int i = 0; i < number_of_bytes; i++) {
        SPI_peripheral.transfer(buffer[i]);
    }
    
    // end the transaction
    SPI_peripheral.endTransaction();
    digitalWrite(CS_pin, HIGH);
}