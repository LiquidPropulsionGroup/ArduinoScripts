/*
 * SKELETON CODE FOR TESTING SENSOR MESSAGE WITH ZERO DATA
 * Thomas W. C. Carlson
 */

#include <Wire.h>
#include <stdint.h>

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

FourBytes TimeStamp;

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

// Static Delay
// Needs to be larger than the byte-length of the message divided by the baud/10 for bytes/s
static int BUFFER_DELAY = 5;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize 
  Packet_Start.int_dat = 0;
  Terminator.int_dat = pow(2,32)-1;  //4294967295;
}

void loop() { 
  // Values to be sent over serial
  TimeStamp.int_dat = millis();   //+16777217; //for testing
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
  FL_WATER.int_dat = 250;//radnom(0,500);

  // Serial Writes
  // Writing the Packet Start bytestring to the Serial Buffer
  Serial.write(Packet_Start.bytes,4);

  // Writing the Timestamp to the Serial Buffer
  Serial.write(TimeStamp.bytes, 4);

  // Writing PT Data to the Serial Buffer
  Serial.write(PT_HE.bytes, 2);
  Serial.write(PT_Purge.bytes, 2);
  Serial.write(PT_Pneu.bytes, 2);
  Serial.write(PT_FUEL_PV.bytes, 2);
  Serial.write(PT_LOX_PV.bytes, 2);
  //Serial.write(PT_FUEL_INJ.bytes, 2);
  Serial.write(PT_CHAM.bytes, 2);

  // Writing TC Data to the Serial Buffer
  Serial.write(TC_FUEL_PV.bytes, 2);
  Serial.write(TC_LOX_PV.bytes, 2);
  Serial.write(TC_LOX_Valve_Main.bytes, 2);
  Serial.write(TC_WATER_In.bytes, 2);
  Serial.write(TC_WATER_Out.bytes, 2);
  Serial.write(TC_CHAM.bytes, 2);

  // Writing RC Data to the Serial Buffer
  //Serial.write(RC_LOX_Level.bytes, 2);

  // Writing FT Data to the Serial Buffer
  Serial.write(FT_Thrust.bytes, 2);

  // Writing FL Data to the Serial Buffer
  Serial.write(FL_WATER.bytes, 2);

  // Writing the Packet Stop bytestring to the Serial Buffer
  Serial.write(Terminator.bytes, 4);

  // Configurable delay
  delay(BUFFER_DELAY);
  
}
