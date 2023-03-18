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
    uint16_t device_ID_recieved = SPI_peripheral.transfer16(0x0000); // next two bytes are the device ID
    
    digitalWrite(CS_pin, HIGH); //de-assert the CS line
    SPI_peripheral.endTransaction(); // end the transaction

    //Setting default non-IDs
    uint8_t manufacturer_ID = 0x11;
    uint16_t device_ID = 0x1111;
    
    // manufacturer ID for W25Q1 is 0xEF, chip ID for W25Q128 is 0x4018, chip ID for W25Q512 is 0x4020
    if (chip_type == 128) {
        manufacturer_ID == 0xEF;
        device_ID = 0x4018;
    }
    else if (chip_type == 512){
        manufacturer_ID == 0xE;
        device_ID = 0x4020;
    } else if (chip_type == 312){
        manufacturer_ID == 0x1F;
        device_ID = 0x8701;
    }
    
    return (manufacturer_ID_recieved == manufacturer_ID || device_ID_recieved == device_ID) ? 1 : 0;
}





/**
* @brief Writes the number of given bytes in a buffer of 8-bit integers.
*/
uint32_t write_to_flash(int CS_pin, SPIClass SPI_peripheral, uint32_t current_address, uint8_t* buffer, int number_of_bytes) {
    
    if(number_of_bytes > 256){
        return current_address; //if the number of bytes to write to memory exceeds a page (256 bytes), the write_to_flash function should not execute and the current_address remains the same
    } 
    
    uint8_t page_program_opcode = 0;
    
    // checks if address is 4 bytes or 3 bytes
    if (0xFF000000 & address) {
        page_program_opcode = 0x12; // for 4 byte address (W25Q512 only)
        int address_counter = 4; //address is 32 bits
    } else {
        page_program_opcode = 0x02; // for 3 byte address
        int address_counter = 3; //address is 24 bits
    }
    
    write_enable(CS_pin, SPI_peripheral);   //enabling the write bit within Status Register 1

    // begin the transaction
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first
    
    uint32_t end_current_page = (0xFFFFFF00 & current_address) + 0xFF; //isolate the most significant bits and add 1 page size (0xFF = 256 bytes)
    uint32_t current_page_space = end_current_page - current_address;
        
    if (current_page_space >= number_of_bytes){ //if the amount of data to write fits within the current page, write normally

        SPI_peripheral.transfer(page_program_opcode);
        for(int i = 0; i < address_counter; i++) SPI_peripheral.transfer(current_address >> 8*i); //transferring address
        SPI_peripheral.transfer(buffer[0], number_of_bytes); //transfering data
 
        digitalWrite(CS_pin, HIGH); //de-assert the CS pin for the command to execute

        //poll the BUSY bit to ensure that the data has been written before de-asserting the chip select line
        while (BUSY_bit_check(CS_pin, SPI_peripheral)){};

        current_address = address_manager(current_address, number_of_bytes); //updating the address

        return current_address; //the next available address to write to is returned
    }
    else{ //if the amount of data to write does not fit within the current page, split the data into seperate packages
            
        SPI_peripheral.transfer(page_program_opcode);
        for(int i = 0; i < address_counter; i++) SPI_peripheral.transfer(current_address >> 8*i); //transferring address
        SPI_peripheral.transfer(buffer[0], current_page_space); //transfering data

        digitalWrite(CS_pin, HIGH); //de-assert the CS pin for the command to execute

        current_address = address_manager(current_address, current_page_space); //updating the address
        uint32_t remaining_bytes = number_of_bytes - current_page_space;

        SPI_peripheral.transfer(page_program_opcode); //transfering operation code
        for(int i = 0; i < address_counter; i++) SPI_peripheral.transfer(current_address >> 8*i); //transferring address
        SPI_peripheral.transfer(buffer[current_page_space], remaining_bytes); //transfering data

        digitalWrite(CS_pin, HIGH); //de-assert the CS pin for the command to successfully execute

        //poll the BUSY bit to ensure that the data has been written before de-asserting the chip select line
        while (BUSY_bit_check(CS_pin, SPI_peripheral)){};
        
        current_address = address_manager(current_address, remaining_bytes); //updating the address
       
        return current_address; //the next available address to write to is returned
    }  

    // end the transaction
    SPI_peripheral.endTransaction();

    return status;
}




/**
* @brief Reads from flash memory.
*/
uint8_t* read_from_flash(int CS_pin, SPIClass SPI_peripheral, uint32_t address_start, uint32_t address_end) {
    // distinguishing between chip types
   
    uint8_t read_data_op_code = 0;
    uint32_t read_size = address_end - address_start;
    uint8_t static data[read_size];

    // checks if address is 4 bytes or 3 bytes
    if (0xFF000000 & address) {
        read_data_op_code = 0x13; // for 4 byte address (W25Q512 only)
        int address_counter = 4; //address is 32 bits
    } else {
        read_data_op_code = 0x03; // for 3 byte address
        int address_counter = 3; //address is 24 bits
    }
    // begin the transaction
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first

     // send a command to read data starting at address_start
    SPI_peripheral.transfer(read_data_op_code); //transferring operation code to read data from flash memory
    for(int i = 0; i < address_counter; i++) SPI_peripheral.transfer(start_address >> 8*i); //transferring address
    for(int i=0; i < read_size; i++) data[i] = SPI_peripheral.transfer(0x00); //collecting data from flash memory

    digitalWrite(CS_pin, HIGH);
    SPI_peripheral.endTransaction();

    return data;
}





/**
* @brief Erases all of the data in the flash chip
*/
uint8_t chip_erase(int CS_pin, SPIClass SPI_peripheral){
    
    write_enable(CS_pin, SPI_peripheral);   //enabling the write bit within Status Register 1

    // begin the transaction
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first

    // send a command to erase ALL data on flash chip
    SPI_peripheral.transfer(0xC7);
    digitalWrite(CS_pin, HIGH); //de-assert the CS line

    //Wait for chip erase to take its typiacl 40 seconds. After that, poll the BUSY bit to ensure that the data has been written before de-asserting the chip select line
    delaty(40000);
    while (BUSY_bit_check(CS_pin, SPI_peripheral)){};

    SPI_peripheral.endTransaction(); // end the transaction
}





/**
* @brief Checks the status of the BUSY bit lcoated in Status Register 1.
*/
uint8_t BUSY_bit_check(int CS_pin, SPIClass SPI_peripheral){
    //SPI_peripheral.beginTransaction(...) and SPI_peripheral.endTransaction() is not executed as BUSY_bit_check should only be executed within another function where the SPI settings have already been set
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.transfer(0x05);
    uint8_t status_register = SPI.transaction(0); //obtains the values within status register 1
    uint8_t BUSY_bit = 0x1 & status_register;
    digitalWrite(CS_pin, HIGH);

    return BUSY_bit;
}





/**
* @brief Write enable to allow Page Program, Chip Erase and Write Status Register commansd to be executed.
*/
void write_enable(int CS_pin, SPICLass SPI_peripheral){

    digitalWrite(CS_pin, LOW);
    SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first
    SPI_peripheral.transfer(0x06); // op-code for write enable
    digitalWrite(CS_pin, HIGH); //de-assert the chip select line
    SPI_peripheral.endTransaction(); // end the transaction
}




/**
* @brief Function used to monitor the current address.
*/
uint32_t address_manager(uint32_t current_address, int bytes_written){
    address = current_address + bytes_written; //function call occured to increment the current address to the next available address to which data can be written
    return address;
}


int main(){
    int CS_pin = 27;
    SPIClass SPI_peripheral;
    int chip_type = 128;
    uint32_t current_address = 0;
    int size = 10;
    uint8_t write_buffer[size] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t* read_buffer;
    int out1;

    out1 = initialize_flash(CS_pin, SPI_peripheral, chip_type);
    assert(out1 == 1);

    current_address = write_to_flash(CS_pin, SPI_peripheral, current_address, write_buffer, size);

    read_buffer = read_from_flash(CS_pin, SPI_peripheral, 0, current_address);

    for(int i=0; i < size; i++) printf("%d, ", read_buffer[i]);
    return 1; //the main function executed properly
}