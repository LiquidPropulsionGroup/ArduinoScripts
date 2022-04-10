/*
 * CODE FOR DATA COLLECTION AND PACKAGING
 * Thomas W. C. Carlson
 * 4/9/2022
 */

/*
 * This code relies on I2C communication with 4 ADS1115 boards. 
 */

////////////////////////BEGIN FILE////////////////////////

/*
 * LIBRARY DECLARATIONS
 */
#include <ADS1X15.h>  // Library for interaction with the ADS1115
                      // https://github.com/RobTillaart/ADS1X15
                      // Relies on Wire.h as well

/*
 * UNION DECLARATIONS
 * Enables fast type conversion from float to byte
 */
typedef union FourBytes{
  int32_t numDat;
  unsigned char bytes[4];
  };

typedef union TwoBytes{
  int16_t numDat;
  unsigned char bytes[2];
  };

/*
 * UNION INSTANTIATIONS
 * Create instances of the unions for holding data
 */
FourBytes Packet_Start;     // Byte marker for packet start
//////////////////////////////
TwoBytes  PT_HE;            // PT_HE sensor data                (0-65535)
TwoBytes  PT_Pneu;          // PT_Pneu sensor data              (0-65535)
TwoBytes  PT_FUEL_PV;       // PT_FUEL_PV sensor data           (0-65535)
TwoBytes  PT_LOX_PV;        // PT_LOX_PV sensor data            (0-65535)
TwoBytes  PT_FUEL_INJ;      // PT_FUEL_INJ sensor data          (0-65535)
TwoBytes  PT_CHAM;          // PT_CHAM sensor data              (0-65535)
//////////////////////////////
TwoBytes  TC_FUEL_PV;       // TC_FUEL_PV sensor data           (0-65535)
TwoBytes  TC_LOX_PV;        // TC_LOX_PV sensor data            (0-65535)
TwoBytes  TC_LOX_Valve_Main;// TC_LOX_Valve_Main sensor data    (0-65535)
TwoBytes  TC_WATER_In;      // TC_WATER_In sensor data          (0-65535)
TwoBytes  TC_WATER_Out;     // TC_WATER_Out sensor data         (0-65535)
TwoBytes  TC_CHAM;          // TC_CHAM sensor data              (0-65535)
//////////////////////////////
TwoBytes  FT_Thrust;        // FT_Thrust sensor data            (0-65535)
//////////////////////////////
FourBytes Packet_End;       // Byte marker for packet start

/*
 * PACKET DEFINITIONS
 * Define the size of and declare the array of data to be sent over Serial.write()
 */
const int SENSOR_MESSAGE_LENGTH = 34;             // Total Byte length of the data packet
char SensorDataMessage[SENSOR_MESSAGE_LENGTH];    // Declare the byte array of length
const int BUFFER_DELAY = 4;                       // Static delay
                                                  // This is the time it takes for the buffer to clear
                                                  // after Serial.write()
                                                  // For 34 bytes @ 115200 baud, theoretically 3.125ms
                                                  // However, the loop execution time is longer than this
                                                  // and Serial.write() is an asynchronous process. Thus,
                                                  // it may be unnecessary.

/*
 * ADS OBJECT INSTANTIATION
 */
ADS1115 ADS[4];             // Create an array to contain the ADS objects
uint16_t ADCBits[16];       // Create an array to contain the number of bits on the ADS input channels
int ChannelIndex;           // Int holding which channel is being sampled for the current loop

/*
 * DEBUG VARIABLES
 */
long baseTime;
long runTime;

void setup() {
  // put your setup code here, to run once:

  // Initialize the Serial port
  Serial.begin(115200);
//  Serial.println("Serial start...");

  // Define the packet ends
  Packet_Start.numDat = 0;
  Packet_End.numDat   = pow(2,32)-1;                      // The maximum value in 4 bytes (2^32 - 1)
  memcpy(&SensorDataMessage[0], Packet_Start.bytes, 4);   // Copy the packet_start data in byte form to the
                                                          // &MEMORY_LOCATION of the package array
  memcpy(&SensorDataMessage[SENSOR_MESSAGE_LENGTH - 4], Packet_End.bytes, 4);

  // Instantiate the ADS objects
  for (uint8_t n = 0; n < 4; n++) {
    uint8_t address = 0x48 + n;           // The ADS addresses are 0x48, 0x49, 0x4A, 0x4B
                                          // These are sequential in HEX, maps to 72, 73, 74, 75 in DEC
    ADS[n] = ADS1115(address);            // Save the objects into the array
    ADS[n].begin();                       // Initialize the ADS boards
    ADS[n].setDataRate(6);                // Data rates in this library are mapped like so:
                                          // Speed  0 : 8   SPS ("cleanest")
                                          //        1 : 16  SPS
                                          //        2 : 32  SPS
                                          //        3 : 64  SPS
                                          //        4 : 128 SPS (default)
                                          //        5 : 250 SPS
                                          //        6 : 475 SPS
                                          //        7 : 860 SPS (noisy)

//    Serial.print(address, HEX);
//    Serial.print("  ");
//    Serial.println(ADS[n].begin() ? "connected" : "not connected");
                                          
  }
  ADS[0].setGain(0);                      // Set the ADS gain for the 0x48 (PT: 0.6-3.0V)
  ADS[1].setGain(0);                      // Set the ADS gain for the 0x49 (PT: 0.6-3.0V)
  ADS[2].setGain(0);                      // Set the ADS gain for the 0x4A (TC: 0-5V)
  ADS[3].setGain(0);                      // Set the ADS gain for the 0x4B (TC: 0-5V)
                                          // ADS gains in this library are mapped like so:
                                          // GAIN   0 : 2/3 (FSR: ±6.144V Resolution: 187.5uV)
                                          //        1 : 1   (FSR: ±4.096V Resolution: 125uV)
                                          //        2 : 2   (FSR: ±2.048V Resolution: 62.5uV)
                                          //        4 : 4   (FSR: ±1.024V Resolution: 31.25uV)
                                          //        8 : 8   (FSR: ±0.512V Resolution: 15.625uV)
                                          //        16: 16  (FSR: ±0.256V Resolution: 7.8125uV)

  // Set the I2C clock speed
  // This is responsible for a huge performance gain. ARM Processor standard values are 100kHz and 400kHz
  // The ADS1115 can support 3.4Mhz, but this results in a lot of jitter. Alternatives are 100kHz and 400kHz
  // The MEGA2560 seems to not support I2C faster than 1Mhz regardless
  Wire.setClock(400000);

  // Start the loop by requesting a sample
  ADS_Request_Data();
}

void loop() {
  // Try to read data until all lanes are sampled and recorded
  while (ADS_Read_AIN()) {
    // Do nothing until all lanes are sampled
  }

  // Print the values if debugging
  Debug_Runtime_Print();
  Debug_Delay();
  Debug_ADS_Print();

  // Request new reads
  ADS_Request_Data();
}

bool ADS_Read_AIN() {
  // Step through each ADS
  for (int i = 0; i < 4; i++) {
    // Check if it is both connected and busy
    if (ADS[i].isConnected() && ADS[i].isBusy()) {
      // If it is, restart the loop by returning true
      // This behavior should continue until the ADS are done reading (not busy)
      return true;
    }
  }
  // If the ADS are connected and not busy, then read the AIN corresponding to the current value
  // of ChannelIndex
  for (int j = 0; j < 4; j++) {
    // 0x48 AIN0: ADS[0], 0x49 AIN0: ADS[4], 0x4A AIN0: ADS[8], 0x4B AIN0: ADS[12]
    //      AIN1: ADS[1],      AIN1: ADS[5],      AIN1: ADS[9],      AIN1: ADS[13]
    //      AIN2: ADS[2],      AIN2: ADS[6],      AIN2: ADS[10],     AIN2: ADS[14]
    //      AIN3: ADS[3],      AIN3: ADS[7],      AIN3: ADS[11],     AIN3: ADS[15]
    ADCBits[j * 4 + ChannelIndex] = ADS[j].getValue();
  }
  // Increment the ChannelIndex to prep reading the next AIN
  ChannelIndex++;
  // So long as ChannelIndex is less than 4, there are still AIN lanes to read
  if (ChannelIndex < 4) {
    // Therefore request sampling the next lane
    ADS_Request_Data();
    // And restart the loop
    return true;
  }
  ChannelIndex = 0;
  return false;
}

void ADS_Request_Data() {
  // Ping the ADS and start a conversion
  for (int k = 0; k < 4; k++) {
    if (ADS[k].isConnected()) {
      ADS[k].requestADC(ChannelIndex);
    }
  }
}

void Debug_ADS_Print() { 
  // Print ADC Results in columns
  Serial.println("\t 0x48 \t 0x49 \t 0x4A \t 0x4B");
  for (int l = 0; l < 4; l++) {
    Serial.print("AIN");
    Serial.print(l);
    Serial.print(": \t");
    for (int m = 0; m < 4; m++) {
      Serial.print(ADCBits[m * 4 + l]*0.0001875, 3);
      Serial.print("\t");
    }
    Serial.println();
  }
  Serial.println();
}

void Debug_Runtime_Print() {
  // Check the end time
  runTime = micros();
  Serial.print("Loop time: ");
  Serial.println((runTime - baseTime)*0.001);

  // Check the time
  baseTime = micros();
}

void Debug_Delay() {
  delay(1000);
}

////////////////////////END OF FILE////////////////////////
