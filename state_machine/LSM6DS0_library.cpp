#include "LSM6DSO_library.hh"

uint8_t initialize_LSM6DSO(void) {
    
    // initialize return value to 0
    uint8_t ret_val = 1;

    //temporary read variable/storage
    uint8_t c;
    
    // request the device's ID
    Wire.beginTransmission(LSM6DSO_ADDRESS);
    Wire.write(WHO_AM_I);
    Wire.endTransmission(RESTART_BIT);
    Wire.requestFrom(LSM6DSO_ADDRESS, 1, END_BIT);
    
    // slave may send less than requested
    while(Wire.available()) {
        c = Wire.read();    // receive a byte
        if (c == LSM6DSO32_ID) {
            ret_val = 0; // set ret_val to 0 if the correct device ID is read
        }
    }
    
    // set the mode of the acclerometer and gyroscope to 6.66kHz 
    // accelerometer
    Wire.beginTransmission(LSM6DSO_ADDRESS);
    Wire.write(CTRL1_XL);
    Wire.write(ODR_XL_6660HZ);
    Wire.endTransmission(END_BIT);
    // gyroscope
    Wire.beginTransmission(LSM6DSO_ADDRESS);
    Wire.write(CTRL2_G);
    Wire.write(ODR_G_6660HZ);
    Wire.endTransmission(END_BIT);
    
    //LPF activation
    Wire.beginTransmission(LSM6DSO_ADDRESS);
    Wire.write(CTRL4_C);
    Wire.endTransmission(RESTART_BIT);
    Wire.requestFrom(LSM6DSO_ADDRESS, 1, END_BIT);

    // slave may send less than requested
    while(Wire.available()) {
      c = Wire.read();    // receive a byte
    }
    c = c & 0b11111101;
    uint8_t lpf_activate = c | 0b00000010;

    Wire.beginTransmission(LSM6DSO_ADDRESS);
    Wire.write(CTRL4_C);
    Wire.write(lpf_activate);
    Wire.endTransmission(END_BIT);

    //LPF bandwidth
    Wire.beginTransmission(LSM6DSO_ADDRESS);
    Wire.write(CTRL6_C);
    Wire.endTransmission(RESTART_BIT);
    Wire.requestFrom(LSM6DSO_ADDRESS, 1, END_BIT);
    // slave may send less than requested
    while(Wire.available()) {
      c = Wire.read();    // receive a byte
    }
    c = c & 0b11111000;
    uint8_t lpf_bandwidth = c | 0b00000000;
    Wire.beginTransmission(LSM6DSO_ADDRESS);
    Wire.write(CTRL6_C);
    Wire.write(lpf_bandwidth);
    Wire.endTransmission(END_BIT);

    // return 1 if success, 0 otherwise
    return ret_val;
}


uint8_t check_status_reg(void) {
    // the status byte
    uint8_t status_byte;
    
    // requesting the status register byte
    Wire.beginTransmission(LSM6DSO_ADDRESS);
    Wire.write(STATUS_REG);
    Wire.endTransmission(RESTART_BIT);
    Wire.requestFrom(LSM6DSO_ADDRESS, 1, END_BIT);
    
     // slave may send less than requested
    while(Wire.available()) {
        status_byte = Wire.read();    // receive a byte
    }
    
    return status_byte;
}


void read_IMU_data(IMU_data_t &IMU_data) {
    // writing the address of the gyroscope register, accelerometer comes right after
    Wire.beginTransmission(LSM6DSO_ADDRESS);
    Wire.write(OUTX_L_G);
    Wire.endTransmission(RESTART_BIT);
    Wire.requestFrom(LSM6DSO_ADDRESS, 12, END_BIT); // reading 12 bytes -> 2-bytes of x,y,z for gyro and accel
    
    // casting the IMU_data variable to an 8-bit pointer
    uint8_t* IMU_data_as_bytes = (uint8_t*) &IMU_data;
    
    for (int i = 0; i < 12; i++) {
        // the IMU register data is little endian, as are the 16-bit values in IMU_data_t, so we can just read the wire
        IMU_data_as_bytes[i] = Wire.read();
    }
}


void convert_IMU_data_to_float(IMU_data_t &IMU_data, three_axis_float_t &accel, three_axis_float_t &gyro) {
    // converting IMU data to float, multiplying the data by the scale factors, and storing them the in the float data variables
    accel.x = ((float) IMU_data.accel.x) * (ACCEL_RANGE / 32768.0) + ACCEL_X_OFFSET;
    accel.y = ((float) IMU_data.accel.y) * (ACCEL_RANGE / 32768.0) + ACCEL_Y_OFFSET;
    accel.z = ((float) IMU_data.accel.z) * (ACCEL_RANGE / 32768.0) + ACCEL_Z_OFFSET;
    gyro.x = ((float) IMU_data.gyro.x) * (0.0175) + GYRO_X_OFFSET;
    gyro.y = ((float) IMU_data.gyro.y) * (0.0175) + GYRO_Y_OFFSET;
    gyro.z = ((float) IMU_data.gyro.z) * (0.0175) + GYRO_Z_OFFSET;
}
