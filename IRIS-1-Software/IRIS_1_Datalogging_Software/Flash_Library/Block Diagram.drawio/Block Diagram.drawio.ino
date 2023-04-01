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

enum CHIP_BIT_SIZE {
  TWENTY_FOUR_BIT,
  THIRTY_TWO_BIT
};

enum MEMORY{
  ADD,
  RESET
};

CHIP_BIT_SIZE CHIP;

/**
* @brief Will check that the flash chip is returning the correct ID, it will also enable the flash chip's write mode.
*
* @param CS_pin The chip select pin of the flash chip
* @param SPI_peripheral The SPI peripheral to use
* @param chip_type Accepts 128 or 512, as those are the only two chips we are using
*
* @return 1 for correct ID, 0 for incorrect 
*/
void initialize_flash(int CS_pin, SPIClass SPI_peripheral, int chip_type) {
    
    //Setting default non-IDs
    uint8_t manufacturer_ID = 0;
    uint16_t device_ID = 0;
    
    // manufacturer ID for W25Q1 is 0xEF, chip ID for W25Q128 is 0x4018, chip ID for W25Q512 is 0x4020
    if (chip_type == 128) {
        manufacturer_ID = 0xEF;
        device_ID = 0x4018;
        CHIP = TWENTY_FOUR_BIT;
    }
    else if (chip_type == 512){
        manufacturer_ID = 0xEF;
        device_ID = 0x4020;
        CHIP = THIRTY_TWO_BIT;
    } else if (chip_type == 312){
        manufacturer_ID = 0x1F;
        device_ID = 0x8701;
        CHIP = TWENTY_FOUR_BIT;
    }

    // begin the transaction
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first
    
    // send a command to return the chip's IDs
    SPI_peripheral.transfer(0x9F); // 9Fh is the get JEDEC ID command
    
    uint8_t manufacturer_ID_recieved = SPI_peripheral.transfer(0x00); // first byte returned is the manufacturer ID
    uint16_t device_ID_recieved = SPI_peripheral.transfer16(0x0000); // next two bytes are the device ID

    digitalWrite(CS_pin, HIGH); //de-assert the CS line
    SPI_peripheral.endTransaction(); // end the transaction
  

    if(manufacturer_ID_recieved == manufacturer_ID && device_ID_recieved == device_ID){
        Serial.println("SUCCESS");
        Serial.printf("Manufacturer ID: 0x%x and Device ID: 0x%x\n", manufacturer_ID_recieved, device_ID_recieved);
    }else
    {
        Serial.println("ERROR");
        Serial.printf("Manufacturer ID: 0x%x and Device ID: 0x%x\n", manufacturer_ID_recieved, device_ID_recieved);
    }
}





/**
* @brief Writes the number of given bytes in a buffer of 8-bit integers.
*/
uint32_t write_to_flash(int CS_pin, SPIClass SPI_peripheral, uint32_t current_address, uint8_t* buffer, uint32_t number_of_bytes) {
//IT IS JUST NOT WORKING IF THE PAGE IS NOT AN EVEN MULTIPLE OF 256 BYTE
    uint32_t temp;
    uint8_t address_send;
    uint8_t page_program_opcode = 0;
    int address_counter = 0;
    uint8_t BUSY_bit = 0;
    
    if(number_of_bytes > 256){
        Serial.println("Page Execute cannot run if the buffer is largen than 265 bytes!");
        return current_address; //if the number of bytes to write to memory exceeds a page (256 bytes), the write_to_flash function should not execute and the current_address remains the same
    } 

    // checks if address is 4 bytes or 3 bytes
    if (0xFF000000 & current_address) {  //TODO: WHAT IF WE NEED TO START WRITING AT 24 BIT ADDRESS AND THEN START WRITING AT 32 BIT ADDRESSES? THEN OP CODE WILL HAVE TO CHANGE RIGHT? MAYBE USE CHIP TYPE NI THIS IF STATEMENT? IT WOULD BE EASIER MAYBE
        page_program_opcode = 0x12; // for 4 byte address (W25Q512 only)
        address_counter = 4; //address is 32 bits
    } else {
        page_program_opcode = 0x02; // for 3 byte address
        address_counter = 3; //address is 24 bits
    }
    /*if (CHIP = THIRTY_TWO_BIT) {  //TODO: WHAT IF WE NEED TO START WRITING AT 24 BIT ADDRESS AND THEN START WRITING AT 32 BIT ADDRESSES? THEN OP CODE WILL HAVE TO CHANGE RIGHT? MAYBE USE CHIP TYPE NI THIS IF STATEMENT? IT WOULD BE EASIER MAYBE
        page_program_opcode = 0x12; // for 4 byte address (W25Q512 only)
        address_counter = 4; //address is 32 bits
    } else {
        page_program_opcode = 0x02; // for 3 byte address
        address_counter = 3; //address is 24 bits
    }
    */
    SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first
    //SPI_peripheral.endTransaction(); // end the transaction

    // begin the transaction
    
    //SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first
    
    uint32_t end_current_page = (0xFFFFFF00 & current_address) + 0xFF; //isolate the most significant bits and add 1 page size (0xFF = 256 bytes)
    uint32_t current_page_space = (end_current_page - current_address) + 1; //adding 1 as both bounds are valid space as well
        
    if (current_page_space >= number_of_bytes){ //if the amount of data to write fits within the current page, write normally

        digitalWrite(CS_pin, LOW);
        SPI_peripheral.transfer(0x06); // op-code for write enable
        digitalWrite(CS_pin, HIGH); //de-assert the chip select line
        
        digitalWrite(CS_pin, LOW);
        SPI_peripheral.transfer(page_program_opcode);
        //for(int i = 0; i < address_counter; i++) SPI_peripheral.transfer(current_address >> 8*i); //transferring address
            for(int i = 0; i < address_counter; i++){
            address_send = (current_address & (0xFF << 8*(address_counter-1-i))) >> (8*(address_counter-1-i));
            Serial.printf("WITHIN THE LOOP, CURRENT ADDRESS IS: 0x%x AND THE MULTIPLIER IS 0x%x", current_address, (0xFF << 8*(address_counter-1-i)));
            Serial.printf("     Address being written to is: %x with i: %d\n", address_send, i);
            SPI_peripheral.transfer(address_send); //transferring address
        }
        for(int i=0; i<number_of_bytes; i++){
            //Serial.printf("Data being written to: 0x%x\n", buffer[i]);
            SPI_peripheral.transfer(buffer[i]); //transfering data
        }
 
        digitalWrite(CS_pin, HIGH); //de-assert the CS pin for the command to execute

        //poll the BUSY bit to ensure that the data has been written before de-asserting the chip select line
        do{
            digitalWrite(CS_pin, LOW);
            SPI_peripheral.transfer(0x05);
            uint8_t status_register = SPI_peripheral.transfer(0); //obtains the values within status register 1
            uint8_t BUSY_bit = 0x1 & status_register;
            digitalWrite(CS_pin, HIGH);
            //Serial.println("STUCK");
        }while(BUSY_bit);
        
        current_address = address_manager(current_address, number_of_bytes, 0); //updating the address
                

    }
    else{ //if the amount of data to write does not fit within the current page, split the data into seperate packages
        
      digitalWrite(CS_pin, LOW);
      SPI_peripheral.transfer(0x06); // op-code for write enable
      digitalWrite(CS_pin, HIGH); //de-assert the chip select line
    
      digitalWrite(CS_pin, LOW);    
        SPI_peripheral.transfer(page_program_opcode);
        //for(int i = 0; i < address_counter; i++) SPI_peripheral.transfer(current_address >> 8*i); //transferring address
        //SPI_peripheral.transfer(buffer, current_page_space); //transfering data
        for(int i = 0; i < address_counter; i++){
            address_send = (current_address & (0xFF << 8*(address_counter-1-i))) >> (8*(address_counter-1-i));
            Serial.printf("WITHIN THE SECOND LOOP, CURRENT ADDRESS IS: 0x%x AND THE MULTIPLIER IS 0x%x", current_address, (0xFF << 8*(address_counter-1-i)));
            Serial.printf("     Address being written to is: %x with i: %d\n", address_send, i);
            SPI_peripheral.transfer(address_send); //transferring address
        }
        for(int i =0; i<current_page_space; i++){
            //Serial.printf("Data being written to: 0x%x\n", buffer[i]);
            Serial.printf("Current i value is: %d and its value is: %d\n", i, buffer[i]);
            SPI_peripheral.transfer(buffer[i]); //transfering data
        }
        digitalWrite(CS_pin, HIGH); //de-assert the CS pin for the command to execute

        //poll the BUSY bit to ensure that the data has been written before de-asserting the chip select line
        do{
            digitalWrite(CS_pin, LOW);
            SPI_peripheral.transfer(0x05);
            uint8_t status_register = SPI_peripheral.transfer(0); //obtains the values within status register 1
            uint8_t BUSY_bit = 0x1 & status_register;
            digitalWrite(CS_pin, HIGH);
            //Serial.println("STUCK2");
        }while(BUSY_bit);
        
        current_address = address_manager(current_address, current_page_space, 0); //updating the address
        uint32_t remaining_bytes = number_of_bytes - current_page_space;

        delay(100);
        
        digitalWrite(CS_pin, LOW);
        SPI_peripheral.transfer(0x06); // op-code for write enable
        digitalWrite(CS_pin, HIGH); //de-assert the chip select line
        
        digitalWrite(CS_pin, LOW);
        SPI_peripheral.transfer(page_program_opcode); //transfering operation code
        //for(int i = 0; i < address_counter; i++) SPI_peripheral.transfer(current_address >> 8*i); //transferring address
        //SPI_peripheral.transfer((buffer+current_page_space), remaining_bytes); //transfering data
        for(int i = 0; i < address_counter; i++){
            address_send = (current_address & (0xFF << 8*(address_counter-1-i))) >> (8*(address_counter-1-i));
            Serial.printf("WITHIN THE 'THIRD' LOOP, CURRENT ADDRESS IS: 0x%x AND THE MULTIPLIER IS 0x%x", current_address, (0xFF << 8*(address_counter-1-i)));
            Serial.printf("     Address being written to is: %x with i: %d\n", address_send, i);
            SPI_peripheral.transfer(address_send); //transferring address
        }
       for(int i=current_page_space; i<number_of_bytes; i++){
            //Serial.printf("Data being written to: 0x%x\n", buffer[i]);
            Serial.printf("Current i value in ''THIRD' LOOP is:%d  and its value is: %d\n", i, buffer[i]);            
            SPI_peripheral.transfer(buffer[i]); //transfering data
        }
        //SPI_peripheral.transfer((buffer+current_page_space), remaining_bytes); //transfering data
        digitalWrite(CS_pin, HIGH); //de-assert the CS pin for the command to successfully execute

        //poll the BUSY bit to ensure that the data has been written before de-asserting the chip select line
        do{
            digitalWrite(CS_pin, LOW);
            SPI_peripheral.transfer(0x05);
            uint8_t status_register = SPI_peripheral.transfer(0); //obtains the values within status register 1
            uint8_t BUSY_bit = 0x1 & status_register;
            digitalWrite(CS_pin, HIGH);
            //Serial.println("STUCK2");
        }while(BUSY_bit);
        
        current_address = address_manager(current_address, remaining_bytes, 0); //updating the address
    }  
      SPI_peripheral.endTransaction();
      return current_address; //the next available address to write to is returned
    // end the transaction
}




/**
* @brief Reads from flash memory.
*/
void read_from_flash(int CS_pin, SPIClass SPI_peripheral, uint32_t address_start, uint32_t address_end, uint8_t* data, uint32_t read_size) {
    // distinguishing between chip types
   
    uint8_t read_data_op_code = 0;

    int address_counter = 0;

    uint32_t temp = 0;

    uint8_t address_send;

    // checks if address is 4 bytes or 3 bytes
    if (0xFF000000 & address_start || 0xFF000000 & address_end) {
        read_data_op_code = 0x13; // for 4 byte address (W25Q512 only)
        address_counter = 4; //address is 32 bits
    } else {
        read_data_op_code = 0x03; // for 3 byte address
        address_counter = 3; //address is 24 bits
    }
    // begin the transaction
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first

     // send a command to read data starting at address_start
    SPI_peripheral.transfer(read_data_op_code); //transferring operation code to read data from flash memory
    Serial.printf("Op code is: 0x%x and address_counter is: %d\n", read_data_op_code,address_counter);
    /*for(int i = 0; i < address_counter; i++){
      temp = address_start >> 8*i;
      Serial.printf("Address being sent is: %x with i: %d\n", temp, i);
      SPI_peripheral.transfer(temp); //transferring address
    }*/
    for(int i = 0; i < address_counter; i++){
      address_send = (address_start & (0xFF << 8*(address_counter-1-i))) >> (8*(address_counter-1-i));
      Serial.printf("Address being read from is: %x with i: %d\n", address_send, i, 0xFF << 8*i);
      SPI_peripheral.transfer(address_send); //transferring address
    }
      
    //for(int i = 0; i < address_counter; i++) SPI_peripheral.transfer(current_address >> 8*i); //transferring address

    for(int i=0; i < read_size; i++){
      data[i] = SPI_peripheral.transfer(0x00); //collecting data from flash memory
    }

    digitalWrite(CS_pin, HIGH);
    SPI_peripheral.endTransaction();
}





/**
* @brief Erases all of the data in the flash chip
*/
uint32_t chip_erase(int CS_pin, SPIClass SPI_peripheral){
    
    uint8_t BUSY_bit = 0;
    
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.beginTransaction(SPISettings(26000000, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first
    SPI_peripheral.transfer(0x06); // op-code for write enable
    digitalWrite(CS_pin, HIGH); //de-assert the chip select line
    //SPI_peripheral.endTransaction(); // end the transaction

    // begin the transaction
    digitalWrite(CS_pin, LOW);

    // send a command to erase ALL data on flash chip
    SPI_peripheral.transfer(0xC7);
    digitalWrite(CS_pin, HIGH); //de-assert the CS line

    //Wait for chip erase to take its typiacl 40 seconds. After that, poll the BUSY bit to ensure that the data has been written before de-asserting the chip select line
    delay(40000);
    do{
      digitalWrite(CS_pin, LOW);
      SPI_peripheral.transfer(0x05);
      uint8_t status_register = SPI_peripheral.transfer(0); //obtains the values within status register 1
      uint8_t BUSY_bit = 0x1 & status_register;
      digitalWrite(CS_pin, HIGH);
      //Serial.println("STUCK2");
    }while(BUSY_bit);

    SPI_peripheral.endTransaction(); // end the transaction

    //resseting the address
    uint32_t address = address_manager(0,0,1);
    Serial.printf("Erased address is: 0x%x", address);
    return address;

}












/**
* @brief Function used to monitor the current address.
*/
uint32_t address_manager(uint32_t current_address, int bytes_written, int erase){
    uint32_t address;
    if(erase == 1) address = 0x100;
    else address = current_address + bytes_written; //function call occured to increment the current address to the next available address to which data can be written

    return address;
}


void setup(){
    Serial.begin(115200);
    delay(500);
    int CS_pin = 5;
    pinMode(CS_pin, OUTPUT);
    SPIClass SPI_peripheral(VSPI);
    SPI_peripheral.begin(18, 19, 23, CS_pin);
    delay(2000);
    int chip_type = 312;
    uint32_t current_address = 0;
    int size = 10;
    uint8_t write_buffer[size] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    uint8_t* read_buffer;


    initialize_flash(CS_pin, SPI_peripheral, chip_type);
    Serial.println("Test compleition of flash");
    current_address = write_to_flash(CS_pin, SPI_peripheral, current_address, write_buffer, size);
    Serial.printf("Current address is: 0x%x\n", current_address);

    uint32_t address_start = 0;
    uint32_t address_end = current_address;
    uint32_t read_size = address_end - address_start;
    uint8_t data[read_size];
    read_from_flash(CS_pin, SPI_peripheral, address_start, address_end, data, read_size);
    for(int i=0; i < size; i++) Serial.printf("0x%x, ",data[i]);

    current_address = chip_erase(CS_pin, SPI_peripheral);
    read_from_flash(CS_pin, SPI_peripheral, address_start, address_end, data, read_size);
    for(int i=0; i < size; i++) Serial.printf("0x%x, ",data[i]);
    Serial.println("CHIP ERASED!");

    uint8_t write_buffer_one[100] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36,
     37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 
     80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99};
    Serial.printf("Try 0: Current address is: 0x%x\n", current_address);
    current_address = write_to_flash(CS_pin, SPI_peripheral, current_address, write_buffer_one, 100);
    Serial.printf("Try 1: Current address is: 0x%x\n", current_address);
    uint8_t write_buffer_two[100] = {100, 101, 102, 103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,
     122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,160,161,162,163,164,165,166,167,
     168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,192,193,194,195,196,197,198,199};
    current_address = write_to_flash(CS_pin, SPI_peripheral, current_address, write_buffer_two, 100);
    Serial.printf("Try 2: Current address is: 0x%x\n", current_address);
    uint8_t write_buffer_three[100] = { 200,201,202,203,204,205,206,207,208,209,210,211,212,213,
     214,215,216,217,218,219,220,221,222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,1, 
     0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43};
    current_address = write_to_flash(CS_pin, SPI_peripheral, current_address, write_buffer_three, 100);
    Serial.printf("Try 3: Current address is: 0x%x\n", current_address);


    address_start = 0x100;
    address_end = current_address;
    read_size = address_end - address_start;
    uint8_t data_two[read_size];
    Serial.printf("Read size is: %u", read_size);
    read_from_flash(CS_pin, SPI_peripheral, address_start, 0x1ff, data_two, 256);
    for(int i=0; i < 256; i++) Serial.printf("0x%x, ",data_two[i]);
    Serial.println("\nNEXT READ FN");
    read_from_flash(CS_pin, SPI_peripheral, 0x200, 0x22b, data_two, 44);
    for(int i=0; i < 44; i++) Serial.printf("0x%x, ",data_two[i]);



    SPI_peripheral.end();

}

void loop(){

}

//TODO: account for non uint8_t data 
//TODO: implement the flot to uint conversaion and ht esturcuts for data
//TODO: account for the chip type in command determination