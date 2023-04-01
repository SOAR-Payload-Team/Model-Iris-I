/*
This is a library for the LSM6DSO inertial measurement unit (IMU) IC present on IRIS I and IRIS II.
*/

/*
TO-DO/General notes:
 - Register:              // all h's should be b's
FUNC_CFG_ACCESS (01h)   : b0000_0000 (should be like this by default)
PIN_CTRL (02h)          : h0011_1111 (default)
FIFO_CTRL1 (07h)        : h0000_0000 (default?)
FIFO_CTRL2 (08h)        : h0000_0000 (default?)
FIFO_CTRL4 (0Ah)        : h0000_0000 (this disables all FIFO functionality so hopefully we don't have to write to the other FIFO registers)
INT1_CTRL (0Dh)         : h0000_0000 (disables all interrupt functionality)
INT2_CTRL (0Eh)         : h0000_0000 ("")
WHO_AM_I (0Fh)          : read only, returns 0x6C
CTRL1_XL (10h)          : h1010_0100 - Op-code : ODR_XL[3:0] | SCALE[1:0] | LFP2_EN | X. ODR_XL = 1010 is 6.66kHz, SCALE = 01 is 16g's
CTRL2_G (11h)           : h1010_0100 - Op-code : ODR_G[3:0] | SCALE[1:0] | SCALE_125DPS | X. ODR_G = 1010 is 6.66kHz, SCALE = 01 is 500dps, SCALE_125DPS = 0 is follow SCALE
CTRL3_C (12h)           : h0000_0000
CTRL4_C (13h)           : h0000_0000
CTRL5_C (14h)           : h0000_0000
CTRL6_C (15h)           : h0000_0000 (if something goes wrong take a look at this register)
CTRL7_G (16h)           : h0000_0000
CTRL8_XL (17h)          : h0000_0000
CTRL9_XL (18h)          : h0000_0000 (also look at this register)
CTRL10_C (19h)          : h0000_0000
STATUS_REG (1Eh)        : read only, reads: 0_0000 | TEMPERATURE_DATA_AVAILABE | GYRO_DATA_AVAILABLE | XL_DATA_AVAILABLE
TAP_CFG0 (56h)          : h0000_0000 (default)
TAP_CFG1 (57h)          : h0000_0000 (default)
TAP_THS_6D (59h)        : h0000_0000 (default)



DATA REGISTERS
OUT_TEMP_L (20h) and OUT_TEMP_H (21h)       : read only, 2's complement
OUTX_L_G (22h) and OUTX_H_G (23h)           : read only, 2's complement
OUTY_L_G (24h) and OUTY_H_G (25h)           : ""
OUTZ_L_G (26h) and OUTZ_H_G (27h)           : ""
OUTX_L_A (28h) and OUTX_H_A (29h)           : ""
OUTY_L_A (2Ah) and OUTY_H_A (2Bh)           : ""
OUTZ_L_A (2Ch) and OUTZ_H_A (2Dh)           : ""


HOW IT WORKS FOR TRANSMITTING DATA TO THE DEVICE:
 - write the slave address with a write bit
 - write the sub-address (register address)
 - write as many bytes as you want (register address auto-increment is enabled by default)
 
HOW IT WORKS FOR READING DATA FROM THE DEVICE:
 - write the slave address with a write bit
 - write the sub-address (register address)
 - write the slave address with a read bit
 - read as many bytes as you want


*/

typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} __attribute__((packed)) three_axis_int_t;

typedef struct {
    three_axis_int_t gyro;
    three_axis_int_t accel;
} __attribute__((packed)) IMU_data_t;

typedef struct {
    float x;
    float y;
    float z;
} __attribute__((packed)) three_axis_float_t;

#include <Wire.h>
#define LSM6DSO_WRITE 0b1101010
#define LSM6DSO_READ 0b1101010
#define WHO_AM_I 0x0F
#define CTRL1_XL 0x10
#define CTRL2_G 0x11
#define STATUS_REG 0x1E
#define ODR_XL_6660HZ 0b10100100
#define ODR_G_6660HZ 0b10100100
#define OUTX_L_A 0x28
#define OUTX_L_G 0x22
#define LSM6DSO_ID 0xC6

#define RESTART_BIT 0
#define END_BIT 1

// can use if statements to define these (or define these and then if statements control other related constants)
// for determining the scale factor for acceleration and rotation
#define ACCEL_RANGE 16.0 //16g's
#define GYRO_RANGE 500.0 // 500dps


/**
@brief Initializes the LSM6DSO IMU. Sets the sample rate to 6.66kHz for both the accelerometer and gyroscope.

@return Returns 1 if the requested chip ID is correct, 0 otherwise
*/
uint8_t initialize_LSM6DSO(void) {
    
    // initialize return value to 0
    int ret_val = 0;
    
    // begin Wire library if not done already
    Wire.begin();
    
    // request the device's ID
    Wire.beginTransmission(LSM6DSO_WRITE);
    Wire.write(WHO_AM_I);
    Wire.endTransmission(RESTART_BIT);
    Wire.requestFrom(LSM6DSO_READ, 1, END_BIT);
    
    // slave may send less than requested
    while(Wire.available()) {
        uint8_t c = Wire.read();    // receive a byte
        if (c == LSM6DSO_ID) {
            ret_val = 1; // set ret_val to 1 if the correct device ID is read
        }
    }
    
    // set the mode of the acclerometer and gyroscope to 6.66kHz 
    // accelerometer
    Wire.beginTransmission(LSM6DSO_WRITE);
    Wire.write(CTRL1_XL);
    Wire.write(ODR_XL_6660HZ);
    Wire.endTransmission(END_BIT);
    // gyroscope
    Wire.beginTransmission(LSM6DSO_WRITE);
    Wire.write(CTRL2_G);
    Wire.write(ODR_G_6660HZ);
    Wire.endTransmission(END_BIT);
    
    
    // return 1 if success, 0 otherwise
    return ret_val;
}

/**
@brief Returns the status register of the LSM6DSO. The three LSB's in the following order are: temperature measurement ready | gyroscope measurement ready | accelerometer measurement ready

@return The current value of the status register
*/
uint8_t check_status_reg(void) {
    // the status byte
    uint8_t status_byte;
    
    // requesting the status register byte
    Wire.beginTransmission(LSM6DSO_WRITE);
    Wire.write(STATUS_REG);
    Wire.endTransmission(RESTART_BIT);
    Wire.requestFrom(LSM6DSO_READ, 1, END_BIT);
    
     // slave may send less than requested
    while(Wire.available()) {
        status_byte = Wire.read();    // receive a byte
    }
    
    return status_byte;
}

/**
@brief Reads the accelerometer and gyroscope data and stores it in the given reference

@param IMU_data The IMU_data_t reference to write data to
*/
void read_IMU_data(IMU_data_t &IMU_data) {
    // writing the address of the gyroscope register, accelerometer comes right after
    Wire.beginTransmission(LSM6DSO_WRITE);
    Wire.write(OUTX_L_G);
    Wire.endTransmission(RESTART_BIT);
    Wire.requestFrom(LSM6DSO_READ, 12, END_BIT); // reading 12 bytes -> 2-bytes of x,y,z for gyro and accel
    
    // casting the IMU_data variable to an 8-bit pointer
    uint8_t* IMU_data_as_bytes = &IMU_data;
    
    for (int i = 0; i < 12; i++) {
        // the IMU register data is little endian, as are the 16-bit values in IMU_data_t, so we can just read the wire
        IMU_data_as_bytes[i] = Wire.read();
    }
}

/**
@brief Converts IMU data to float values with the scale factors given by ACCEL_RANGE and GYRO_RANGE

@param IMU_data The IMU_data_t reference to get IMU data from
@param accel The three_axis_float_t reference to populate with accelerometer data
@param gyro The three_axis_float_t reference to populate with gyroscope data
*/
void convert_IMU_data_to_float(IMU_data_t &IMU_data, three_axis_float_t &accel, three_axis_float_t &gyro) {
    // converting IMU data to float, multiplying the data by the scale factors, and storing them the in the float data variables
    accel.x = ((float) IMU_data.accel.x) * (ACCEL_RANGE / 32768.0);
    accel.y = ((float) IMU_data.accel.y) * (ACCEL_RANGE / 32768.0);
    accel.z = ((float) IMU_data.accel.z) * (ACCEL_RANGE / 32768.0);
    gyro.x = ((float) IMU_data.gyro.x) * (GYRO_RANGE / 32768.0);
    gyro.y = ((float) IMU_data.gyro.y) * (GYRO_RANGE / 32768.0);
    gyro.z = ((float) IMU_data.gyro.z) * (GYRO_RANGE / 32768.0);
}