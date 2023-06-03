#include "flash_library.hh"

#define SPI_HZ 26000000

uint8_t initialize_flash(uint8_t CS_pin, SPIClass SPI_peripheral, CHIP_TYPE CHIP) {
    
    //Setting default non-IDs
    uint8_t manufacturer_ID = 0;
    uint16_t device_ID = 0;
    
    // manufacturer ID for W25Q1 is 0xEF, chip ID for W25Q128 is 0x4018, chip ID for W25Q512 is 0x7020
    if (CHIP == W25Q128) {
        manufacturer_ID = 0xEF;
        device_ID = 0x4018;
    }else if (CHIP == W25Q512){
        manufacturer_ID = 0xEF;
        device_ID = 0x7020;
    } else if (CHIP == AT25SF){
        manufacturer_ID = 0x1F;
        device_ID = 0x8701;
    }

    // begin the transaction
    digitalWrite(CS_pin, LOW);

    //delay(1000);
    SPI_peripheral.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first
    
    // send a command to return the chip's IDs
    SPI_peripheral.transfer(0x9F); // 9Fh is the get JEDEC ID command
    
    uint8_t manufacturer_ID_recieved = SPI_peripheral.transfer(0x00); // first byte returned is the manufacturer ID
    uint16_t device_ID_recieved = SPI_peripheral.transfer16(0x0000); // next two bytes are the device ID

    digitalWrite(CS_pin, HIGH); //de-assert the CS line
    SPI_peripheral.endTransaction(); // end the transaction

    if(manufacturer_ID_recieved == manufacturer_ID && device_ID_recieved == device_ID){
        Serial.println("SUCCESS");
        Serial.printf("Manufacturer ID: 0x%x and Device ID: 0x%x\n", manufacturer_ID_recieved, device_ID_recieved);
        return 0;
    }else
    {
        Serial.println("ERROR");
        Serial.printf("Manufacturer ID: 0x%x and Device ID: 0x%x\n", manufacturer_ID_recieved, device_ID_recieved);
        return 1;
    }
}


uint32_t write_to_flash(uint8_t CS_pin, SPIClass SPI_peripheral, uint32_t current_address, uint8_t* buffer, uint32_t number_of_bytes, CHIP_TYPE CHIP) {

    uint8_t address_send;
    uint8_t page_program_opcode = 0;
    uint8_t address_counter = 0;
    uint8_t BUSY_bit = 0;
    
    if(number_of_bytes > 256){
        Serial.println("Page Execute cannot run if the buffer is largen than 265 bytes!");
        return current_address; //if the number of bytes to write to memory exceeds a page (256 bytes), the write_to_flash function should not execute and the current_address remains the same
    } 

    // checks if address is 4 bytes or 3 bytes
    if (CHIP == W25Q512) {
        page_program_opcode = 0x12; // for 4 byte address (W25Q512 only)
        address_counter = 4; //address is 32 bits
    } else {
        page_program_opcode = 0x02; // for 3 byte address
        address_counter = 3; //address is 24 bits
    }
    
    SPI_peripheral.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first
    
    uint32_t end_current_page = (0xFFFFFF00 & current_address) + 0xFF; //isolate the most significant bits and add 1 page size (0xFF = 256 bytes)
    uint32_t current_page_space = (end_current_page - current_address) + 1; //adding 1 as both bounds are valid space as well
        
    if (current_page_space >= number_of_bytes){ //if the amount of data to write fits within the current page, write normally

        write_enable(CS_pin, SPI_peripheral);
        
        digitalWrite(CS_pin, LOW);
        SPI_peripheral.transfer(page_program_opcode);
        //for(int i = 0; i < address_counter; i++) SPI_peripheral.transfer(current_address >> 8*i); //transferring address
        for(uint8_t i = 0; i < address_counter; i++){
          address_send = (current_address & (0xFF << 8*(address_counter-1-i))) >> (8*(address_counter-1-i));
          //Serial.printf("WITHIN THE LOOP, CURRENT ADDRESS IS: 0x%x AND THE MULTIPLIER IS 0x%x\n", current_address, (0xFF << 8*(address_counter-1-i)));
          //Serial.printf("     Address being written to is: %x with i: %d\n", address_send, i);            
          SPI_peripheral.transfer(address_send); //transferring address
        }

        /*for(uint8_t i=0; i<4; i++){
          Serial.printf("\n\n\nbuffer value at %d is 0x%x", i, *(buffer+i));
        }*/
        

        SPI_peripheral.transfer(buffer, number_of_bytes);

        digitalWrite(CS_pin, HIGH); //de-assert the CS pin for the command to execute

        //Poll the BUSY bit.
        while(busy_bit_check(CS_pin, SPI_peripheral)){}
        
        current_address = address_manager(current_address, number_of_bytes, ADD); //updating the address
    }
    else{ //if the amount of data to write does not fit within the current page, split the data into seperate packages
        
      write_enable(CS_pin, SPI_peripheral);
    
      digitalWrite(CS_pin, LOW);    
        SPI_peripheral.transfer(page_program_opcode);
        //for(int i = 0; i < address_counter; i++) SPI_peripheral.transfer(current_address >> 8*i); //transferring address
        //SPI_peripheral.transfer(buffer, current_page_space); //transfering data
        for(int i = 0; i < address_counter; i++){
            address_send = (current_address & (0xFF << 8*(address_counter-1-i))) >> (8*(address_counter-1-i)); //transferring MSB of address first through the use of address_counter-1-i arithmetic
            //Serial.printf("WITHIN THE SECOND LOOP, CURRENT ADDRESS IS: 0x%x AND THE MULTIPLIER IS 0x%x", current_address, (0xFF << 8*(address_counter-1-i)));
            //Serial.printf("     Address being written to is: %x with i: %d\n", address_send, i);
            SPI_peripheral.transfer(address_send); //transferring address
        }
        
        SPI_peripheral.transfer(buffer, current_page_space);
        
        digitalWrite(CS_pin, HIGH); //de-assert the CS pin for the command to execute

        //Poll the BUSY bit.
        while(busy_bit_check(CS_pin, SPI_peripheral)){};

        current_address = address_manager(current_address, current_page_space, ADD); //updating the address

        uint32_t remaining_bytes = number_of_bytes - current_page_space;

        delay(100); //without the delay, the second writing part of this function (see below) will not execute correctly
        
        write_enable(CS_pin, SPI_peripheral);
        
        digitalWrite(CS_pin, LOW);
        SPI_peripheral.transfer(page_program_opcode); //transfering operation code
        //for(int i = 0; i < address_counter; i++) SPI_peripheral.transfer(current_address >> 8*i); //transferring address
        //SPI_peripheral.transfer((buffer+current_page_space), remaining_bytes); //transfering data
        for(int i = 0; i < address_counter; i++){
            address_send = (current_address & (0xFF << 8*(address_counter-1-i))) >> (8*(address_counter-1-i)); //transferring MSB of address first through the use of address_counter-1-i arithmetic
            //Serial.printf("WITHIN THE 'THIRD' LOOP, CURRENT ADDRESS IS: 0x%x AND THE MULTIPLIER IS 0x%x", current_address, (0xFF << 8*(address_counter-1-i)));
            //Serial.printf("     Address being written to is: %x with i: %d\n", address_send, i);
            SPI_peripheral.transfer(address_send); //transferring address
        }
  
        SPI_peripheral.transfer((buffer+current_page_space), remaining_bytes); //transfering data
        digitalWrite(CS_pin, HIGH); //de-assert the CS pin for the command to successfully execute

      //Poll the BUSY bit.
        while(busy_bit_check(CS_pin, SPI_peripheral)){};
        
        current_address = address_manager(current_address, remaining_bytes, ADD); //updating the address
    }  
      SPI_peripheral.endTransaction();
      return current_address; //the next available address to write to is returned
    // end the transaction
}


void read_from_flash(uint8_t CS_pin, SPIClass SPI_peripheral, uint32_t address_start, uint8_t* data, uint32_t read_size, CHIP_TYPE CHIP) {
    // distinguishing between chip types
   
    uint8_t read_data_op_code;
    int address_counter;
    uint8_t address_send;

    // checks if address is 4 bytes or 3 bytes
    if (CHIP == W25Q512) {
        read_data_op_code = 0x13; // for 4 byte address (W25Q512 only)
        address_counter = 4; //address is 32 bits
    } else {
        read_data_op_code = 0x03; // for 3 byte address
        address_counter = 3; //address is 24 bits
    }

    // begin the transaction
    digitalWrite(CS_pin, LOW);
    //delay(1000);
    SPI_peripheral.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first
    //delay(100);

     // send a command to read data starting at address_start
    SPI_peripheral.transfer(read_data_op_code); //transferring operation code to read data from flash memory
        //delay(100);
    //Serial.printf("Op code is: 0x%x and address_counter is: %d\n", read_data_op_code,address_counter);
    /*for(int i = 0; i < address_counter; i++){
      temp = address_start >> 8*i;
      Serial.printf("Address being sent is: %x with i: %d\n", temp, i);
      SPI_peripheral.transfer(temp); //transferring address
    }*/
    for(int i = 0; i < address_counter; i++){
      address_send = (address_start & (0xFF << 8*(address_counter-1-i))) >> (8*(address_counter-1-i));
      //Serial.printf("Address being read from is: %x with i: %d\n", address_send, i, 0xFF << 8*i);
      SPI_peripheral.transfer(address_send); //transferring address
      //Serial.printf("Address sent and read size is: %u", read_size);
          //delay(100);
    }
      
    //for(int i = 0; i < address_counter; i++) SPI_peripheral.transfer(current_address >> 8*i); //transferring address

    for(uint32_t i=0; i < read_size; i++){
      data[i] = SPI_peripheral.transfer(0x00); //collecting data from flash memory in 8 bit chunks
          //delay(100);
    }

    digitalWrite(CS_pin, HIGH);
        //delay(1000);
    SPI_peripheral.endTransaction();
        //delay(100);
}


uint32_t chip_erase(uint8_t CS_pin, SPIClass SPI_peripheral){
    
    uint8_t BUSY_bit = 0;
    
    SPI_peripheral.beginTransaction(SPISettings(SPI_HZ, MSBFIRST, SPI_MODE0)); // rising edge of clock, MSB first
    write_enable(CS_pin, SPI_peripheral);
    //SPI_peripheral.endTransaction(); // end the transaction

    // begin the transaction
    digitalWrite(CS_pin, LOW);

    // send a command to erase ALL data on flash chip
    SPI_peripheral.transfer(0xC7);
    digitalWrite(CS_pin, HIGH); //de-assert the CS line

    //Wait for chip erase to take its typical 40 seconds. After that, poll the BUSY bit.
    delay(40000);
    while(busy_bit_check(CS_pin, SPI_peripheral)){}

    SPI_peripheral.endTransaction(); // end the transaction

    //resseting the address
    uint32_t address = address_manager(0,0,ERASE);
    return address;
}


uint32_t address_manager(uint32_t current_address, uint32_t bytes_written, uint8_t state){
    uint32_t address;
    if(state == ERASE) address = 0x0;
    else address = current_address + bytes_written; //function call occured to increment the current address to the next available address to which data can be written

    return address;
}


void write_enable(uint8_t CS_pin, SPIClass SPI_peripheral){
  digitalWrite(CS_pin, LOW);
  SPI_peripheral.transfer(0x06); // op-code for write enable
  digitalWrite(CS_pin, HIGH); //de-assert the chip select line
}


int busy_bit_check(uint8_t CS_pin, SPIClass SPI_peripheral){
    //delay(100);  UNCOMMENT IF FLASH CHIP CODE STOPS WORKING!
    digitalWrite(CS_pin, LOW);
    SPI_peripheral.transfer(0x05); //op-code to obtain the values from status register 1
    uint8_t status_register = SPI_peripheral.transfer(0); //obtains the values within status register 1
    //Serial.printf("\n\n\n\n\n\n\nThe status register value is 0x%x\n", status_register);
    uint8_t BUSY_bit = 0x1 & status_register; //isolating the BUSY bit
    digitalWrite(CS_pin, HIGH);
/*
    delay(1000);
    digitalWrite(CS_pin, LOW);

    SPI_peripheral.transfer(0x15); //op-code to obtain the values from status register 1
    status_register = SPI_peripheral.transfer(0);
    Serial.printf("\n\n\n\n\n\n\nThe status register THREE VALUS is 0x%x\n", status_register);
    digitalWrite(CS_pin, HIGH);
*/
    return BUSY_bit;
}


void CS_enable(uint8_t CS_pin){
  pinMode(CS_pin, OUTPUT); //setting the CS pin as an output
  digitalWrite(CS_pin, HIGH); //pulling the CS line high before the initial SPI transmission
}
