//SKELETON CODE FOR ACTUATING AND TRANSMITTING STATE
//Thomas W. C. Carlson

#include <Wire.h>
#include <stdint.h>
#include <HardwareSerial.h>

// Pin Declarations
int FUEL_Press_ACTPIN = 13;
int LOX_Press_ACTPIN = 12;
int FUEL_Vent_ACTPIN = 11;
int LOX_Vent_ACTPIN = 10;
int MAIN_ACTPIN = 9;
int FUEL_Purge_ACTPIN = 8;
int LOX_Purge_ACTPIN = 7;

int FUEL_Press_READPIN = 2;
int LOX_Press_READPIN = A5;
int FUEL_Vent_READPIN = A4;
int LOX_Vent_READPIN = A3;
int MAIN_READPIN = A2;
int FUEL_Purge_READPIN = A1;
int LOX_Purge_READPIN = A0;

// Union Declarations
typedef union FourBytes{
  uint32_t int_dat;
  unsigned char bytes[4];
};

// Union instantiations in packet structure order
// For sending status updates
FourBytes Packet_Start;

FourBytes TimeStamp;

char FUEL_Press_Send;
char LOX_Press_Send;
char FUEL_Vent_Send;
char LOX_Vent_Send;
char MAIN_Send;
char FUEL_Purge_Send;
char LOX_Purge_Send;

FourBytes Terminator;

static int BUFFER_DELAY = 5;

// Global variables for storing previous state values
bool FUEL_Press_Prev;
bool LOX_Press_Prev;
bool FUEL_Vent_Prev;
bool LOX_Vent_Prev;
bool MAIN_Prev;
bool FUEL_Purge_Prev;
bool LOX_Purge_Prev;
bool STATE_CHANGED;

// Global variables for storing current state values
bool FUEL_Press;
bool LOX_Press;
bool FUEL_Vent;
bool LOX_Vent;
bool MAIN;
bool FUEL_Purge;
bool LOX_Purge;

// Global variables for receiving instructions over serial
const int MESSAGE_LENGTH = 14;
char ReceivedChars[MESSAGE_LENGTH+2];
bool NewData;

// Comparison Packet for verifying instruction messages
// FUEL_Pres(S), LOX_Pres(s), FUEL_Ven(T), LOX_Ven(t), (M)ain, FUEL_Purg(E), FUEL_Purg(e)
// Interspersed with dummy request character holders
const char InstructionTemplate[14] = {'S','\0','s','\0','T','\1','t','\1','M','\0','E','\0','e','\0'};

// Global variables for storing desired states
// For receiving instructions
bool FUEL_Press_Desired;
bool LOX_Press_Desired;
bool FUEL_Vent_Desired;
bool LOX_Vent_Desired;
bool MAIN_Desired;
bool FUEL_Purge_Desired;
bool LOX_Purge_Desired;

bool ReceiveInProgress;
bool MESSAGE_GOOD = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize the constant union items
  Packet_Start.int_dat = 0;
  Terminator.int_dat = pow(2,32)-1;   //4294967295;

  // Initialize the normally <open/closed> behavior of the actuators
  FUEL_Press_Desired = false;           // NORMALLY CLOSED
  LOX_Press_Desired = false;            // NORMALLY CLOSED
  FUEL_Vent_Desired = true;             // NORMALLY OPEN
  LOX_Vent_Desired = true;              // NORMALLY OPEN
  MAIN_Desired = false;                 // NORMALLY CLOSED
  FUEL_Purge_Desired = false;           // NORMALLY CLOSED
  LOX_Purge_Desired = false;            // NORMALLY CLOSED

  // Initialize the previous <open/closed> state information
  FUEL_Press_Prev = FUEL_Press;
  LOX_Press_Prev = LOX_Press;
  FUEL_Vent_Prev = FUEL_Vent;
  LOX_Vent_Prev = LOX_Vent;
  MAIN_Prev = MAIN;
  FUEL_Purge_Prev = FUEL_Purge;
  LOX_Purge_Prev = LOX_Purge;
  STATE_CHANGED = false;

  // Initialize output pins
  pinMode(FUEL_Press_ACTPIN, OUTPUT);
  pinMode(LOX_Press_ACTPIN, OUTPUT);
  pinMode(FUEL_Vent_ACTPIN, OUTPUT);
  pinMode(LOX_Vent_ACTPIN, OUTPUT);
  pinMode(MAIN_ACTPIN, OUTPUT);
  pinMode(FUEL_Purge_ACTPIN, OUTPUT);
  pinMode(LOX_Purge_ACTPIN, OUTPUT);

//  LOX_Purge_Desired.state = true;
//  ReceiveInProgress = false;
  

  // Initialize input pins for sensing actuator state
  pinMode(FUEL_Press_READPIN, INPUT);
  pinMode(LOX_Press_READPIN, INPUT);
  pinMode(FUEL_Vent_READPIN, INPUT);
  pinMode(LOX_Vent_READPIN, INPUT);
  pinMode(MAIN_READPIN, INPUT);
  pinMode(FUEL_Purge_READPIN, INPUT);
  pinMode(LOX_Purge_READPIN, INPUT);

  // Execute Initializations
  for ( int a = 0; a < sizeof(ReceivedChars);  a++ ) {
    ReceivedChars[a] = '0';
  }
  VerifyStates();
}

void loop() {
  // put your main code here, to run repeatedly:
  // Pull the timestamp at the start of the loop
  TimeStamp.int_dat = millis();
//  Serial.println("READ DATA");
  ReceiveData();
  if (MESSAGE_GOOD == true) {
//  Serial.println("PARSE DATA");
  ParseMessage();
//  Serial.println("VERIFY STATES");
  VerifyStates();
//  Serial.println("SEND UPDATES");
  SendUpdate();
  NewData = false;
  FUEL_Press = digitalRead(FUEL_Press_READPIN);
  LOX_Press = digitalRead(LOX_Press_READPIN);
  FUEL_Vent = digitalRead(FUEL_Vent_READPIN);
  LOX_Vent = digitalRead(LOX_Vent_READPIN);
  MAIN = digitalRead(MAIN_READPIN);
  FUEL_Purge = digitalRead(FUEL_Purge_READPIN);
  LOX_Purge = digitalRead(LOX_Purge_READPIN);
  }
}

void ReceiveData() {
  // Control variables
//  static bool ReceiveInProgress = false;
  static byte MessageIndex = 0;
  char Starter = '<';
  char Terminator = '>';
  char ReceivedChar;
  MESSAGE_GOOD = false;
//  Serial.println(ReceivedChars);
//  Serial.println("Receive Data:");
//  Serial.println(Serial.available());
//  Serial.println(MessageIndex);
  if (Serial.available() > 0) {

//move everything back by 1 index and read the next byte into the last index
      for(int i = 0; i < MESSAGE_LENGTH+1 /*All but the last character*/;i++){
          ReceivedChars[i] = ReceivedChars[i+1];
      }
      ReceivedChars[MESSAGE_LENGTH+1] = Serial.read();
//      Serial.println(ReceivedChars);
      //validate
      if (ReceivedChars[0] != Starter) {
//        Serial.println("IMPROPER START CHAR, DISCARD");
        return;
      }else if (ReceivedChars[MESSAGE_LENGTH+1] != Terminator) {
//        Serial.println("IMPROPER END CHAR, DISCARD");
        return;
      } else {
        for (int i = 0; i < MESSAGE_LENGTH; i += 2) {
          if (ReceivedChars[i+1] != InstructionTemplate[i]) {
//            Serial.println("Identifiers malformed");
            return;
          }
          if (ReceivedChars[i+2] != '1' && ReceivedChars[i+2] != '0') {
//            Serial.println("States malformed");
            return;
          }
        }
      }
//      Serial.println("MESSAGE GOOD");
      MESSAGE_GOOD = true;
  }

//  Serial.println(ReceivedChars);
}

void ParseMessage() {
  // Control vars
  static int VerifiedCount = 0;
  
  // Pull the message apart into components
  
  // Pull the instructions out of the message
  // Update the desired states list
//    Serial.println("MESSAGE GOOD");
  // Map FUEL_Press request to boolean
  FUEL_Press_Desired = ByteToBool(ReceivedChars[2]);
  LOX_Press_Desired = ByteToBool(ReceivedChars[4]);
  FUEL_Vent_Desired = ByteToBool(ReceivedChars[6]);
  LOX_Vent_Desired = ByteToBool(ReceivedChars[8]);
  MAIN_Desired = ByteToBool(ReceivedChars[10]);
  FUEL_Purge_Desired = ByteToBool(ReceivedChars[12]);
  LOX_Purge_Desired = ByteToBool(ReceivedChars[14]);
//    Serial.print(FUEL_Press_Desired);
//    Serial.print(LOX_Press_Desired);
//    Serial.print(FUEL_Vent_Desired);
//    Serial.print(LOX_Vent_Desired);
//    Serial.print(MAIN_Desired);
//    Serial.print(FUEL_Purge_Desired);
//    Serial.print(LOX_Purge_Desired);
    // And prepare to read a new message
  MESSAGE_GOOD = false;
}


void VerifyStates() {
  // Read the sense wire states
  FUEL_Press = digitalRead(FUEL_Press_READPIN);
  LOX_Press = digitalRead(LOX_Press_READPIN);
  FUEL_Vent = digitalRead(FUEL_Vent_READPIN);
  LOX_Vent = digitalRead(LOX_Vent_READPIN);
  MAIN = digitalRead(MAIN_READPIN);
  FUEL_Purge = digitalRead(FUEL_Purge_READPIN);
  LOX_Purge = digitalRead(LOX_Purge_READPIN); 

//  Serial.println("=====DESIRED STATES=====");
//  Serial.println(FUEL_Press_Desired);
//  Serial.println(LOX_Press_Desired);
//  Serial.println(FUEL_Vent_Desired);
//  Serial.println(LOX_Vent_Desired);
//  Serial.println(MAIN_Desired);
//  Serial.println(FUEL_Purge_Desired);
//  Serial.println(LOX_Purge_Desired);

  // Compare the read states to the desired states and set them to be equal to the desired
  // If FUEL_Press does not match the ordered state
  if (FUEL_Press != FUEL_Press_Desired) {
//    Serial.println("CHANGE FUEL PRESS");
    // Store current state into previous state
    FUEL_Press_Prev = FUEL_Press;
    // Write the new state to the pin
    digitalWrite(FUEL_Press_ACTPIN, FUEL_Press_Desired);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
  // If LOX_Press does not match the ordered state
  if (LOX_Press != LOX_Press_Desired) {
//    Serial.println("CHANGE LOX PRESS");
    // Store current state into previous state
    LOX_Press_Prev = LOX_Press;
    // Write the new state to the pin
    digitalWrite(LOX_Press_ACTPIN, LOX_Press_Desired);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
  // If FUEL_Vent does not match the ordered state
  if (FUEL_Vent != FUEL_Vent_Desired) {
//    Serial.println("CHANGE FUEL VENT");
    // Store current state into previous state
    FUEL_Vent_Prev = FUEL_Vent;
    // Write the new state to the pin
    digitalWrite(FUEL_Vent_ACTPIN, FUEL_Vent_Desired);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
  // If LOX_Vent does not match the ordered state
  if (LOX_Vent != LOX_Vent_Desired) {
//    Serial.println("CHANGE LOX VENT");
    // Store current state into previous state
    LOX_Vent_Prev = LOX_Vent;
    // Write the new state to the pin
    digitalWrite(LOX_Vent_ACTPIN, LOX_Vent_Desired);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
  // If MAIN does not match the ordered state
  if (MAIN != MAIN_Desired) {
//    Serial.println("CHANGE MAIN");
    // Store current state into previous state
    MAIN_Prev = MAIN;
    // Write the new state to the pin
    digitalWrite(MAIN_ACTPIN, MAIN_Desired);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
  // If FUEL_Purge does not match the ordered state
  if (FUEL_Purge != FUEL_Purge_Desired) {
//    Serial.println("CHANGE FUEL PURGE");
    // Store current state into previous state
    FUEL_Purge_Prev = FUEL_Purge;
    // Write the new state to the pin
    digitalWrite(FUEL_Purge_ACTPIN, FUEL_Purge_Desired);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
  // If FUEL_Purge does not match the ordered state
  if (LOX_Purge != LOX_Purge_Desired) {
//    Serial.println("CHANGE LOX PURGE");
    // Store current state into previous state
    LOX_Purge_Prev = LOX_Purge;
    // Write the new state to the pin
    digitalWrite(LOX_Purge_ACTPIN, LOX_Purge_Desired);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
}

void SendUpdate() {
  if (STATE_CHANGED) {
//    Serial.println("STATE CHANGED");
    // Check the current status
    FUEL_Press = digitalRead(FUEL_Press_READPIN);
    LOX_Press = digitalRead(LOX_Press_READPIN);
    FUEL_Vent = digitalRead(FUEL_Vent_READPIN);
    LOX_Vent = digitalRead(LOX_Vent_READPIN);
    MAIN = digitalRead(MAIN_READPIN);
    FUEL_Purge = digitalRead(FUEL_Purge_READPIN);
    LOX_Purge = digitalRead(LOX_Purge_READPIN); 

    // Map the boolean values to the corresponding bytes
    FUEL_Press_Send = BoolToByte(FUEL_Press);
    LOX_Press_Send = BoolToByte(LOX_Press);
    FUEL_Vent_Send = BoolToByte(FUEL_Vent);
    LOX_Vent_Send = BoolToByte(LOX_Vent);
    MAIN_Send = BoolToByte(MAIN);
    FUEL_Purge_Send = BoolToByte(FUEL_Purge);
    LOX_Purge_Send = BoolToByte(LOX_Purge);
    
    // Serial Writes
    // Writing the Packet Start bytestring to the Serial Buffer
    Serial.write(Packet_Start.bytes,4);

    // Writing the Timestamp to the Serial Buffer
    Serial.write(TimeStamp.bytes, 4);
  
    // Writing the actuator states to the Serial Buffer
    Serial.write(FUEL_Press_Send);
    Serial.write(LOX_Press_Send);
    Serial.write(FUEL_Vent_Send);
    Serial.write(LOX_Vent_Send);
    Serial.write(MAIN_Send);
    Serial.write(FUEL_Purge_Send);
    Serial.write(LOX_Purge_Send);

    // Writing the Packet Stop bytestring to the Serial Buffer
    Serial.write(Terminator.bytes, 4);
    STATE_CHANGED = false;
  }
}

char BoolToByte(const bool Boolean) {
  char Byte;
  if (Boolean == false) {
    return Byte = '0';
  } else if (Boolean == true) {
    return Byte = '1';
  }
}

bool ByteToBool(const char Byte) {
  bool Boolean;
  if (Byte == '0') {
    return Boolean = false;
  } else if (Byte == '1') {
    return Boolean = true;
  }
}
