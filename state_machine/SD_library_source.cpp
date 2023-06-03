#include "SD_library.hh"


uint8_t SD_init(uint8_t CS_SD, SPIClass SPI_peripheral_H){
  if(!(SD.begin(CS_SD, SPI_peripheral_H, 5000000))){
    return 1;
  }
  return 0;
}



uint8_t file_number_read_bytes(File file, uint64_t* file_number_read_bytes_length){
  file = SD.open("/file_system/file_number.txt", FILE_READ);
  if(!file){
    return 1;
  }
  *file_number_read_bytes_length = file.available();
 
  return 0;
}



uint8_t create_file_name(File file, char* file_name_str, uint64_t file_number_read_bytes_length, CHIP_TYPE CHIP){
  file = SD.open("/file_system/file_number.txt", FILE_READ);
  if(!file){
    return 1;
  }

  uint8_t buf_file_number[file_number_read_bytes_length];
  file.read(buf_file_number, file_number_read_bytes_length);
  file.close();

  sprintf(file_name_str, "/");
  for(uint8_t i=0; i<file_number_read_bytes_length; i++){
    sprintf(file_name_str+i+1, "%c",buf_file_number[i]);
  }

  if(CHIP == W25Q128){
    sprintf(file_name_str+1+file_number_read_bytes_length, "DB.txt");
  }else if (CHIP == W25Q512) {
    sprintf(file_name_str+1+file_number_read_bytes_length, "FB.txt");
  }
  return 0;
}



uint8_t increment_file(File file){
  file = SD.open("/file_system/file_number.txt", FILE_READ);
  if(!file){
    return 1;
  }
  Serial.println("Opened");
  uint64_t file_number_read_bytes_length = file.available();
  uint8_t buf_file_number[file_number_read_bytes_length];
  file.read(buf_file_number, file_number_read_bytes_length);
  file.close();
  Serial.println("Closed");

  char file_name_str[file_number_read_bytes_length];
  char* file_name_ptr = file_name_str;
  for(uint8_t i=0; i<file_number_read_bytes_length; i++){
    sprintf(file_name_ptr+i, "%c",buf_file_number[i]);
  }
    Serial.println("Copied");


  uint64_t next_file_number = atoi(file_name_str)+1;

    if(SD.open("/file_system/file_number.txt", FILE_WRITE)){
      Serial.println("Succeeded in opening for renumbering.");
      file.print(next_file_number);
    }else {
      Serial.println("Failed in opening file for renumbering.");
      return 1;
    }
    file.close();
    Serial.println("Renumbering complete!");
    return 0; 
}














