#include "MS5611_library.hh"

uint8_t MS5611_init(){
  int8_t ret;

  Wire.beginTransmission(MS5611_ADDRESS);
  Wire.write(0x00);
  ret = Wire.endTransmission(END_BIT);
  
  return ret;
}


void MS5611_reset(){
    int8_t ret=-1;
    uint8_t count = 0;

    //repeat reset until the MS5611 chip registers the reset or three tries have been attempted (do not want to end up in an infinte loop in case the MS5611 stops functioning during launch)
    while(ret != 0 && count < 3){
        Wire.beginTransmission(MS5611_ADDRESS);
        Wire.write(MS5611_RESET);
        ret = Wire.endTransmission();
        delay(1);
        count++;
    }   
}


void MS5611_calibration_read(){
    int8_t ret=-1;
    uint16_t read1, read2;
    uint8_t reg = 0b0010;

    for(int i=0; i<6; i++){
      Wire.beginTransmission(MS5611_ADDRESS);
      Wire.write(PROM_READ | reg);
      ret = Wire.endTransmission(RESTART_BIT);
      delay(10);
    
      Wire.requestFrom(MS5611_ADDRESS, 2, END_BIT);
      while(Wire.available()){
        read1 = Wire.read(); 
        read1 = read1 << 8;
        read2 = Wire.read();
        C[i] = read1 | read2;
      }
      ret = -1;
      reg = reg + 2;
    }    
}


MS5611_int_digital_t MS5611_read(){
  int8_t ret = -1;
  uint32_t read1, read2, read3;
  MS5611_int_digital_t MS5611_digital_reads;
  uint8_t count = 0;
  
  while(ret != 0 && count < 3){
      Wire.beginTransmission(MS5611_ADDRESS);
      Wire.write(CONVERSION_PRESSURE);
      ret = Wire.endTransmission(END_BIT);
      delay(1);
      count++;
  }
  delay(1);  //wait necessary conversion time
  
  ret = -1;
  count = 0;
  while(ret != 0 && count < 3){
      Wire.beginTransmission(MS5611_ADDRESS);
      Wire.write(ADC_READ);
      ret = Wire.endTransmission(RESTART_BIT);
      delay(1);
      count++;
  }
  Wire.requestFrom(MS5611_ADDRESS, 3, END_BIT);
  while(Wire.available()){
      read1 = Wire.read(); 
      read1 = read1 << 16;
      read2 = Wire.read();
      read2 = read2 << 8;
      read3 = Wire.read();
      MS5611_digital_reads.D1 = read1 | read2 | read3;
  }

  ret = -1;
  count = 0;
  while(ret != 0 && count < 3){
      Wire.beginTransmission(MS5611_ADDRESS);
      Wire.write(CONVERSION_TEMPERATURE);
      ret = Wire.endTransmission(END_BIT);
      delay(1);
      count++;
  }
  delay(1);  //wait necessary conversion time

  ret = -1;
  count = 0;
  while(ret != 0 && count < 3){
      Wire.beginTransmission(MS5611_ADDRESS);
      Wire.write(ADC_READ);
      ret = Wire.endTransmission(RESTART_BIT);
      delay(1);
      count++;
  }
  Wire.requestFrom(MS5611_ADDRESS, 3, END_BIT);
  while(Wire.available()){
      read1 = Wire.read(); 
      read1 = read1 << 16;
      read2 = Wire.read();
      read2 = read2 << 8;
      read3 = Wire.read();
      MS5611_digital_reads.D2 = read1 | read2 | read3;
  }

  return MS5611_digital_reads;
}


MS5611_int_temperature_t MS5611_calculate_temp(MS5611_int_digital_t MS5611_digital_reads){
  MS5611_int_temperature_t MS5611_temperature;

  //Serial.printf("D2: %u\n", MS5611_digital_reads.D2 );
  
  MS5611_temperature.dT = MS5611_digital_reads.D2-(C[4]*(pow(2,8)));
  //Serial.printf("dT: %d\n",  MS5611_temperature.dT );

  MS5611_temperature.TEMP = 2000 + (MS5611_temperature.dT * (C[5]/(pow(2,23))));
  //Serial.printf("TEMP: %d\n",   MS5611_temperature.TEMP);


  return MS5611_temperature;
}


MS5611_int_pressure_t MS5611_calculate_pressure(MS5611_int_digital_t MS5611_digital_reads, MS5611_int_temperature_t MS5611_temperature){
  MS5611_int_pressure_t MS5611_pressure;

  MS5611_pressure.OFF = (C[1]*(pow(2,16)))+((C[3]*MS5611_temperature.dT)/(pow(2,7)));
  MS5611_pressure.SENS = (C[0]*(pow(2,15)))+((C[2]*MS5611_temperature.dT)/(pow(2,8)));
  MS5611_pressure.P = (((MS5611_digital_reads.D1*MS5611_pressure.SENS)/(pow(2,21))) - MS5611_pressure.OFF)/(pow(2,15));

  return MS5611_pressure;
}

void MS5611_second_order_analysis(MS5611_int_temperature_t* MS5611_temperature, MS5611_int_pressure_t* MS5611_pressure){
  int32_t T2;
  int64_t OFF2;
  int64_t SENS2;

  if((MS5611_temperature->TEMP/1000.0) < 20){
    T2 = (pow(MS5611_temperature->dT,2))/(pow(2,31));
    OFF2 = (5*(pow((MS5611_temperature->TEMP-2000),2)))/2;
    SENS2 = (5*(pow((MS5611_temperature->TEMP-2000),2)))/(pow(2,2));
    if((MS5611_temperature->TEMP/1000.0) < -15){
      OFF2 = OFF2 + (7 * (pow((MS5611_temperature->TEMP+1500),2)));
      SENS2 = SENS2 + ((11*(pow((MS5611_temperature->TEMP+1500),2)))/2);
    }
  }else{
    T2 = 0;
    OFF2 = 0;
    SENS2 = 0;
  }
  MS5611_temperature->TEMP = MS5611_temperature->TEMP - T2;
  MS5611_pressure->OFF = MS5611_pressure->OFF - OFF2;
  MS5611_pressure->SENS = MS5611_pressure->SENS - SENS2;
}



MS5611_float_t MS5611_convert_to_float(MS5611_int_temperature_t MS5611_temperature, MS5611_int_pressure_t MS5611_pressure){
  MS5611_float_t sensor_values;
  //Serial.printf("TEMP: %u\n",MS5611_temperature.TEMP);
  //Serial.printf("PRESSURE IS: %u\n",MS5611_pressure.P);
  sensor_values.temperature = MS5611_temperature.TEMP/100.0;
  sensor_values.pressure = MS5611_pressure.P/100.0;
 
  return sensor_values;
}

