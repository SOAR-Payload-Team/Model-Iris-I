#include "TMP177_library.hh"

uint8_t TMP177_init(){
  int8_t I2C_ret;
  uint8_t ret;
  uint8_t count=0;
  uint16_t read1, read2, ID_receive, config_receive;
  uint8_t config_send_LSB, config_send_MSB;


  while(I2C_ret!=0 || count<3){
    Wire.beginTransmission(TMP177_ADDRESS);
    Wire.write(DEVICE_ID_REGISTER);
    I2C_ret = Wire.endTransmission(RESTART_BIT);
    delay(1);
    count++;
  }

  Wire.requestFrom(TMP177_ADDRESS, 2, END_BIT);
    while(Wire.available()){
      read1 = Wire.read(); 
      read1 = read1 << 8;
      read2 = Wire.read();
      ID_receive = read1 | read2;
  }

  if(ID_receive == DEVICE_ID){
    ret = 0;
  } else{
    ret = 1;
  }

  count=0;
  while(I2C_ret!=0 || count<3){
    Wire.beginTransmission(TMP177_ADDRESS);
    Wire.write(CONFIG_REGISTER);
    I2C_ret = Wire.endTransmission(RESTART_BIT);
    delay(1);
    count++;
  }

    Wire.requestFrom(TMP177_ADDRESS, 2, END_BIT);
    while(Wire.available()){
      read1 = Wire.read(); 
      read1 = read1 << 8;
      read2 = Wire.read();
      config_receive = read1 | read2;
  }

  //Making it so that AVG[1:0] = 00 and CONV[2:0] == 000 in the configuration register to allow for data to be clocked in quickly

  config_receive = config_receive & 0xFC1F;
  config_send_LSB = (uint8_t) config_receive;
  config_send_MSB = (uint8_t) (config_receive >> 8);

  Wire.beginTransmission(TMP177_ADDRESS);
  Wire.write(CONFIG_REGISTER);
  Wire.write(config_send_MSB);
  Wire.write(config_send_LSB);
  Wire.endTransmission(END_BIT);

  return ret;
}


int16_t TMP177_data_read(){
  int8_t I2C_ret;
  uint8_t ret;
  uint8_t count=0;
  uint16_t read1, read2;
  int16_t data_digital;
  uint16_t config_receive;

  do{
    config_receive = TMP177_data_ready();
  }while(config_receive != 0x2000);

  while(I2C_ret!=0 || count<3){
    Wire.beginTransmission(TMP177_ADDRESS);
    Wire.write(DATA_REGISTER);
    I2C_ret = Wire.endTransmission(RESTART_BIT);
    delay(1);
    count++;
  }

   Wire.requestFrom(TMP177_ADDRESS, 2, END_BIT);
    while(Wire.available()){
      read1 = Wire.read(); 
      read1 = read1 << 8;
      read2 = Wire.read();
      data_digital = (int16_t) (read1 | read2);
  }

  return data_digital;
}


uint16_t TMP177_data_ready(){
  int8_t I2C_ret;
  uint8_t ret;
  uint8_t count=0;
  uint16_t read1, read2, config_receive;

  while(I2C_ret!=0 || count<3){
    Wire.beginTransmission(TMP177_ADDRESS);
    Wire.write(CONFIG_REGISTER);
    I2C_ret = Wire.endTransmission(RESTART_BIT);
    delay(1);
    count++;
  }

    Wire.requestFrom(TMP177_ADDRESS, 2, END_BIT);
    while(Wire.available()){
      read1 = Wire.read(); 
      read1 = read1 << 8;
      read2 = Wire.read();
      config_receive = read1 | read2;
  }

  config_receive = config_receive & 0x2000;
  return config_receive;
}


float TMP177_convert_to_float(int16_t data_digital){
  float data_float = ((float) data_digital)*0.0078125; //data sheet states that one LSB equals 7.8125mC
  return data_float;
}
