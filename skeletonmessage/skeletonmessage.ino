/*
 * SKELETON CODE FOR TESTING SENSOR MESSAGE WITH ZERO DATA
 * Thomas W. C. Carlson
 */

// Include necessary libraries
#include <Wire.h>           // Enables I2C
#include <stdint.h>         // Enables strict byte-size of variable types
#include <HardwareSerial.h> // Enables Serial.read() to grab 1 char from the buffer

// Union declarations
typedef union FourBytes{
    uint32_t int_dat;
    unsigned char bytes[4];
  };

typedef union TwoBytes {
    uint16_t int_dat;
    unsigned char bytes[2];
  };

// Union instantiations, in packet structure order
FourBytes Packet_Start;

TwoBytes PT_HE;
TwoBytes PT_Purge;
TwoBytes PT_Pneu;
TwoBytes PT_FUEL_PV;
TwoBytes PT_LOX_PV;
TwoBytes PT_FUEL_INJ;
TwoBytes PT_CHAM;

TwoBytes TC_FUEL_PV;
TwoBytes TC_LOX_PV;
TwoBytes TC_LOX_Valve_Main;
TwoBytes TC_WATER_In;
TwoBytes TC_WATER_Out;
TwoBytes TC_CHAM;

TwoBytes RC_LOX_Level;

TwoBytes FT_Thrust;

TwoBytes FL_WATER;

FourBytes Terminator;

const int SENSOR_MESSAGE_LENGTH = 36;
char SensorDataMessage[SENSOR_MESSAGE_LENGTH];

// Static Delay
// Needs to be larger than the byte-length of the message divided by the baud/10 for bytes/s
// Theoretically 3.125ms
// Practically has to be found empirically
static int BUFFER_DELAY = 50;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize 
  Packet_Start.int_dat = 0;
  Terminator.int_dat = pow(2,32)-1;  //4294967295;
  memcpy(&SensorDataMessage[0], Packet_Start.bytes, 4);
  memcpy(&SensorDataMessage[32], Terminator.bytes, 4);

}

void loop() { 
  // Values to be sent over serial
  PT_HE.int_dat = 10;//random(0,500);
  PT_Purge.int_dat = 20;//random(0,500);
  PT_Pneu.int_dat = 30;//random(0,500);
  PT_FUEL_PV.int_dat = 40;//random(0,500);
  PT_LOX_PV.int_dat = 50;//random(0,500);
  //PT_FUEL_INJ.int_dat = random(0,500);
  PT_CHAM.int_dat = 100;//random(0,500);
  TC_FUEL_PV.int_dat = 110;//random(0,500);
  TC_LOX_PV.int_dat = 120;//random(0,500);
  TC_LOX_Valve_Main.int_dat = 130;//random(0,500);
  TC_WATER_In.int_dat = 140;//random(0,500);
  TC_WATER_Out.int_dat = 150;//random(0,500);
  TC_CHAM.int_dat = 300;//random(0,500);
  //RC_LOX_Level.int_dat = random(0,500);
  FT_Thrust.int_dat = 500;//random(0,500);
  FL_WATER.int_dat = 250;//random(0,500);

  // Serial writes
  // Assign each data point to its spot in the message array
  memcpy(&SensorDataMessage[4],PT_HE.bytes, 2);
  memcpy(&SensorDataMessage[6],PT_Purge.bytes, 2);
  memcpy(&SensorDataMessage[8],PT_Pneu.bytes, 2);
  memcpy(&SensorDataMessage[10],PT_FUEL_PV.bytes, 2);
  memcpy(&SensorDataMessage[12],PT_LOX_PV.bytes, 2);
  memcpy(&SensorDataMessage[14],PT_CHAM.bytes, 2);
  memcpy(&SensorDataMessage[16],TC_FUEL_PV.bytes, 2);
  memcpy(&SensorDataMessage[18],TC_LOX_PV.bytes, 2);
  memcpy(&SensorDataMessage[20],TC_LOX_Valve_Main.bytes, 2);
  memcpy(&SensorDataMessage[22],TC_WATER_In.bytes, 2);
  memcpy(&SensorDataMessage[24],TC_WATER_Out.bytes, 2);
  memcpy(&SensorDataMessage[26],TC_CHAM.bytes, 2);
  memcpy(&SensorDataMessage[28],FT_Thrust.bytes, 2);
  memcpy(&SensorDataMessage[30],FL_WATER.bytes, 2);

  // Send the array
  Serial.write(SensorDataMessage, SENSOR_MESSAGE_LENGTH);

  // Configurable delay
  delay(BUFFER_DELAY);
}
