//SKELETON CODE FOR ACTUATING AND TRANSMITTING STATE
//Thomas W. C. Carlson

#include <Wire.h>
#include <stdint.h>

// Union Declarations
typedef union FourBytes{
  uint32_t int_dat;
  unsigned char bytes[4];
};

typedef union OneByte{
  bool state;
  unsigned char bytes[1];
};

// Union instantiations in packet structure order
FourBytes Packet_Start;

FourBytes TimeStamp;

OneByte FUEL_Press;
OneByte LOX_Press;
OneByte FUEL_Vent;
OneByte LOX_Vent;
OneByte MAIN;
OneByte FUEL_Purge;
OneByte LOX_Purge;

FourBytes Terminator;

// Global variables for storing previous state values
bool FUEL_Press_Prev;
bool LOX_Press_Prev;
bool FUEL_Vent_Prev;
bool LOX_Vent_Prev;
bool MAIN_Prev;
bool FUEL_Purge_Prev;
bool LOX_Purge_Prev;
bool STATE_CHANGED;

int count = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize the constant union items
  Packet_Start.int_dat = 0;
  Terminator.int_dat = pow(2,32)-1;   //4294967295;

  // Initialize the normally <open/closed> behavior of the actuators
  FUEL_Press.state = false;           // NORMALLY CLOSED
  LOX_Press.state = false;            // NORMALLY CLOSED
  FUEL_Vent.state = true;             // NORMALLY OPEN
  LOX_Vent.state = true;              // NORMALLY OPEN
  MAIN.state = false;                 // NORMALLY CLOSED
  FUEL_Purge.state = false;           // NORMALLY CLOSED
  LOX_Purge.state = false;            // NORMALLY CLOSED

  // Initialize the previous <open/closed> state information
  FUEL_Press_Prev = FUEL_Press.state;
  LOX_Press_Prev = LOX_Press.state;
  FUEL_Vent_Prev = FUEL_Vent.state;
  LOX_Vent_Prev = LOX_Vent.state;
  MAIN_Prev = MAIN.state;
  FUEL_Purge_Prev = FUEL_Purge.state;
  LOX_Purge_Prev = LOX_Purge.state;
  STATE_CHANGED = false;

  
}

void loop() {
  // put your main code here, to run repeatedly:
  TimeStamp.int_dat = millis();

  
  if (count >= 10) {
    FUEL_Press.state = !FUEL_Press.state;
    count = 0;
    STATE_CHANGED = true;
  }

  // Only write to serial if the states have changed
  if (STATE_CHANGED) {
    // Serial Writes
    // Writing the Packet Start bytestring to the Serial Buffer
    //Serial.write(Packet_Start.bytes,4);

    // Writing the Timestamp to the Serial Buffer
    //Serial.write(TimeStamp.bytes, 4);
  
    // Writing the actuator states to the Serial Buffer
    Serial.write(FUEL_Press.bytes, 1);
    Serial.write(LOX_Press.bytes, 1);
    Serial.write(FUEL_Vent.bytes, 1);
    Serial.write(LOX_Vent.bytes, 1);
    Serial.write(MAIN.bytes, 1);
    Serial.write(FUEL_Purge.bytes, 1);
    Serial.write(LOX_Purge.bytes, 1);

    // Writing the Packet Stop bytestring to the Serial Buffer
    //Serial.write(Terminator.bytes, 4);
    STATE_CHANGED = false;
  }
  
  // Otherwise do nothing
  Serial.print(count);
  Serial.write('\n');
  count += 1;
  delay(1000);

}
