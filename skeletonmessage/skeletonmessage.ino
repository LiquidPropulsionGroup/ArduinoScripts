/*
 * SKELETON CODE FOR TESTING SENSOR MESSAGE WITH ZERO DATA
 * Thomas W. C. Carlson
 * 8/25/2021
 */

#include <Wire.h>
#include <SensorMessage.h>
#include <stdint.h>
SensorMessage sensorMessage(0);

typedef union FourBytes{
    uint32_t int_dat;
    unsigned char bytes[4];
  };

typedef union TwoBytes {
    uint16_t int_dat;
    unsigned char bytes[2];
  };

FourBytes Terminator;
FourBytes Packet_Start;

bool toggle = true;

static int BUFFER_DELAY = 100;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  Terminator.int_dat = pow(2,32)-1;
  Packet_Start.int_dat = 0;
  //4294967295;
  //Serial.println(Terminator.int_dat);
}

void loop() {
  // put your main code here, to run repeatedly:
  /*if(toggle == true){
    sensorMessage.PT_HE = 1;                   ///< 16 bit unsigned integer representing the Helium presurrant supply
    sensorMessage.PT_Purge = random(0,500);                ///< 16 bit unsigned integer representing the Nitrogen purge supply
    sensorMessage.PT_Pneu = random(0,500);                 ///< 16 bit unsigned integer representing the Nitrogen pneumatic supply
    sensorMessage.PT_FUEL_PV = random(0,500);              ///< 16 bit unsigned integer representing the Gas above fuel in pressure vessel
    sensorMessage.PT_LOX_PV = random(0,500);               ///< 16 bit unsigned integer representing the Gas above LOX in pressure vessel
    sensorMessage.PT_FUEL_INJ = random(0,500);             ///< 16 bit unsigned integer representing the Fuel pressure before entering chamber
    sensorMessage.PT_CHAM = random(0,500);                 ///< 16 bit unsigned integer representing the Chamber pressure
    sensorMessage.TC_FUEL_PV = random(0,500);              ///< 16 bit unsigned integer representing the Gas above fuel in pressure vessel (possible surface mount)
    sensorMessage.TC_LOX_PV = random(0,500);               ///< 16 bit unsigned integer representing the Gas above LOX in pressure vessel (possible surface mount)
    sensorMessage.TC_LOX_Valve_Main = random(0,500);       ///< 16 bit unsigned integer representing the External sensor for valve chill in and force
    sensorMessage.RC_LOX_Level = random(0,500);            ///< 16 bit unsigned integer representing the Capacitive level sensor of LOX in pressure vessel
    sensorMessage.FT_Thrust = random(0,500);               ///< 16 bit unsigned integer representing the Load cell that measures thrust of engine
    sensorMessage.Send();
    Serial.println(millis());
    //delay(10);
    //toggle = false;
  }*/

  
  FourBytes TimeStamp;
  TimeStamp.int_dat = millis();//+16777217;

  TwoBytes PT_HE;
  PT_HE.int_dat = 65535; 
  
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

  // Writing the Packet Stop bytestring to the Serial Buffer
  Serial.write(Terminator.bytes, 4);

  // Configurable delay
  delay(SERIAL_DELAY);
  
}
