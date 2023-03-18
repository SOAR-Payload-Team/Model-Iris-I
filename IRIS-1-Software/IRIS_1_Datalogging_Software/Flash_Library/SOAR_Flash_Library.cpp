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
    
    uint8_t manufacturer_ID_recieved = SPI_peripheral.transfer(0x00); // first byte returned is the manufacturer ID
    uint16_t device_ID = SPI_peripheral.transfer16(0x0000); // next two bytes are the device ID
    
    // end the transaction
    SPI_peripheral.endTransaction();
    digitalWrite(CS_pin, HIGH);
    
    delay(1);
    
    // enable the write mode <--- THIS NEEDS TO BE CALLED EVERYTIME, IT IS RESET TO 0 AFTER THE WRITE INSTRUCTION IS CMOPLETED, SO A SEPERATE FUNCTION MAY BE REQUIRED
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0));
    SPI_peripheral.transfer(0x06); // op-code for write enable
    digitalWrite(CS_pin, HIGH);     //de-assert the chip select line
    SPI_peripheral.endTransaction();
    
    //Setting default non-IDs
    uint16_t chip_ID = 0x1111;
    uint8_t manufacturer_ID = 0x11

    // manufacturer ID for W25Q1 is 0xEF, chip ID for W25Q128 is 0x4018, chip ID for W25Q512 is 0x4020
    // manufacturer ID for AT25SF is 0x1F, chip ID for AT25SF321B is 0x8701
    if (chip_type == 128) {
        manufacturer_ID == 0xEF;
        chip_ID = 0x4018;
    }
    else if (chip_type == 512) {
        manufacturer_ID == 0xEF;
        chip_ID = 0x4020;
    }
    else if (chip_type == 321) {
        manufacturer_ID == 0x1F;
        chip_ID = 0x8701;
    }
    
    return (manufacturer_ID_recieved == manufacturer_ID || device_ID == chip_ID) ? 1 : 0;
}

/**
* @brief Writes the number of given bytes in a buffer of 8-bit integers
*/
uint8_t write_to_flash(int CS_pin, SPIClass SPI_peripheral, uint32_t* address, uint8_t* buffer, int number_of_bytes) {
    
    uint8_t status; // returns 0 on failure and 1 on success                    

    if(number_of_bytes > 256){
        status = 1;
        return status; //if the number of bytes to write to memory exceeds a page (256 bytes), the write_to_flash function should not execute
    } 
    
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
    
    //add code here

    uint32_t end_current_page = (0xFFFFFF00 & *address) + 0xFF; //isolate the most significant bits and add 1 page size (0xFF = 256 bytes)
    uint32_t current_page_space = end_current_page - *address;
    uint32_t write_address;

    if (current_page_space >= number_of_bytes){ //if the amount of data to write fits within the current page, write normally
        write_address = *address;

        SPI_peripheral.transfer(page_program_opcode);
        SPI_peripheral.transfer(write_address);
        for (int i = 0; i < number_of_bytes; i++) {
            SPI_peripheral.transfer(buffer[i]);
        }

        digitalWrite(CS_pin, HIGH); //de-assert the CS pin for the command to execute

        *address = *address + number_of_bytes;

        //poll the BUSY bit to ensure that the data has been written before de-asserting the chip select line
        while (BUSY_bit_check(CS_pin, SPI_peripheral)){};
        status = 1;
    }
    else{ //if the amount of data to write does not fit within the current page, split the data into seperate packages
        write_address = *address;
        SPI_peripheral.transfer(page_program_opcode);
        SPI_peripheral.transfer(write_address);
        for (int i = 0; i < current_page_space; i++) {
            SPI_peripheral.transfer(buffer[i]);
        }

        *address = *address + current_page_space;
        write_address = *address;
        uint32_t remaining_bytes = number_of_bytes - current_page_space;

        SPI_peripheral.transfer(page_program_opcode);
        SPI_peripheral.transfer(write_address);
        for (int i = 0; i < remaining_bytes; i++) {
            SPI_peripheral.transfer(buffer[i]);
        }
        
        digitalWrite(CS_pin, HIGH); //de-assert the CS pin for the command to execute

         *address = *address + remaining_bytes;

        //poll the BUSY bit to ensure that the data has been written before de-asserting the chip select line
        while (BUSY_bit_check(CS_pin, SPI_peripheral)){};
        status = 1;

    }  

    // end the transaction
    SPI_peripheral.endTransaction();

    return status;
}

uint8_t BUSY_bit_check(int CS_pin, SPIClass SPI_peripheral){
    
    //SPI_peripheral.beginTransaction is not executed as BUSY_bit_check should only be executed within another function where the SPI settings have already been set
    
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.transfer(0x05);
    uint8_t status_register = SPI.transaction(0); //obtains the values within status register 1
    uint8_t BUSY_bit = 0x1 & status_register;
    digitalWrite(CS_pin, HIGH);

    return BUSY_bit;
}

uint8_t erase_flash(){

}
//TODO: use assert statements to stop the program from running a function if an error occurs. maybe try to error handle though!