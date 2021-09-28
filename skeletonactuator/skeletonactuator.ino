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

typedef union OneByte{
  bool state;
  unsigned char bytes[1];
};

// Union instantiations in packet structure order
// For sending status updates
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

// Global variables for receiving instructions over serial
const byte MESSAGE_LENGTH = 14;
char ReceivedChars[MESSAGE_LENGTH];
bool NewData;

// Comparison Packet for verifying instruction messages
// FUEL_Pres(S), LOX_Pres(s), FUEL_Ven(T), LOX_Ven(t), (M)ain, FUEL_Purg(E), FUEL_Purg(e)
// Interspersed with dummy request character holders
const char InstructionTemplate[14] = {'S','\0','s','\1','T','\0','t','\1','M','\0','E','\1','e','\0'};

// Global variables for storing desired states
// For receiving instructions
OneByte FUEL_Press_Desired;
OneByte LOX_Press_Desired;
OneByte FUEL_Vent_Desired;
OneByte LOX_Vent_Desired;
OneByte MAIN_Desired;
OneByte FUEL_Purge_Desired;
OneByte LOX_Purge_Desired;

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

  // Initialize output pins
  pinMode(FUEL_Press_ACTPIN, OUTPUT);
  pinMode(LOX_Press_ACTPIN, OUTPUT);
  pinMode(FUEL_Vent_ACTPIN, OUTPUT);
  pinMode(LOX_Vent_ACTPIN, OUTPUT);
  pinMode(MAIN_ACTPIN, OUTPUT);
  pinMode(FUEL_Purge_ACTPIN, OUTPUT);
  pinMode(LOX_Purge_ACTPIN, OUTPUT);

  // Initialize input pins for sensing actuator state
  pinMode(FUEL_Press_READPIN, INPUT);
  pinMode(LOX_Press_READPIN, INPUT);
  pinMode(FUEL_Vent_READPIN, INPUT);
  pinMode(LOX_Vent_READPIN, INPUT);
  pinMode(MAIN_READPIN, INPUT);
  pinMode(FUEL_Purge_READPIN, INPUT);
  pinMode(LOX_Purge_READPIN, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  // Pull the timestamp at the start of the loop
  TimeStamp.int_dat = millis();
  ReceiveData();
  ParseMessage();
  VerifyStates();
  SendUpdate();
  NewData = false;
}

void ReceiveData() {
  // Control variables
  static bool ReceiveInProgress = false;
  static byte MessageIndex = 0;
  char Starter = '<';
  char Terminator = '>';
  char ReceivedChar;

  while (Serial.available() > 0 && NewData == false) {
    // Read in a new character
    ReceivedChar = Serial.read();
    // If it belongs to a message being read
    if (ReceiveInProgress == true) {
      // and isn't a terminator character
      if (ReceivedChar != Terminator) {
          // Add it to the message to be parsed later
          ReceivedChars[MessageIndex] = ReceivedChar;
          // Increment the message index
          MessageIndex++;
          if (MessageIndex >= MESSAGE_LENGTH) {
            MessageIndex = MESSAGE_LENGTH - 1;
          }
        } else {
          // If it is a terminator character terminate the string
          Serial.println("TERMINATOR RECEIVED");
          // and update the control variables to intake a new message
          ReceiveInProgress = false;
          MessageIndex = 0;
          NewData = true;
        }
    } else if (ReceivedChar == Starter) {
      // If not currently receiving a new message, and the character is the starter character
      // Update the control variable to start reading a new message
      Serial.println("NEW MESSAGE");
      ReceiveInProgress = true;
    }
  }
}

void ParseMessage() {
  // Control vars
  static bool MESSAGE_GOOD = false;
  static int VerifiedCount = 0;
  
  // Pull the message apart into components
  // Verify message integrity
  for (int i = 0; i <= MESSAGE_LENGTH - 2; i += 2) {
    // For every 2 items in the instruction there should be an identifier character
    if (ReceivedChars[i] == InstructionTemplate[i]) {
      // Check that it matches the template
      Serial.println(ReceivedChars[i]);
      Serial.println("VERIFIED: " + String(VerifiedCount));
      VerifiedCount++;
    }
    if (VerifiedCount == (MESSAGE_LENGTH-2) / 2) {
      // If all characters are verified pair the instruction with the valve
      MESSAGE_GOOD = true;
    }
  }
  // Reset the verification counter
  Serial.println((MESSAGE_LENGTH-2) / 2);
  VerifiedCount = 0;
  Serial.println(MESSAGE_GOOD);
  // Pull the instructions out of the message
  if (MESSAGE_GOOD) {
    // Update the desired states list
    FUEL_Press_Desired.bytes[1] = ReceivedChars[1];
    LOX_Press_Desired.bytes[1] = ReceivedChars[3];
    FUEL_Vent_Desired.bytes[1] = ReceivedChars[5];
    LOX_Vent_Desired.bytes[1] = ReceivedChars[7];
    MAIN_Desired.bytes[1] = ReceivedChars[9];
    FUEL_Purge_Desired.bytes[1] = ReceivedChars[11];
    LOX_Purge_Desired.bytes[1] = ReceivedChars[13];
    // And prepare to read a new message
    MESSAGE_GOOD = false;
    for ( int a = 0; a < sizeof(ReceivedChars);  a++ ) {
      ReceivedChars[a] = (char)0;
    }
    Serial.println(ReceivedChars);
    Serial.println(FUEL_Press_Desired.bytes[1]);
  }
}

void VerifyStates() {
  // Read the sense wire states
  FUEL_Press.state = digitalRead(FUEL_Press_READPIN);
  LOX_Press.state = digitalRead(LOX_Press_READPIN);
  FUEL_Vent.state = digitalRead(FUEL_Vent_READPIN);
  LOX_Vent.state = digitalRead(LOX_Vent_READPIN);
  MAIN.state = digitalRead(MAIN_READPIN);
  FUEL_Purge.state = digitalRead(FUEL_Purge_READPIN);
  LOX_Purge.state = digitalRead(LOX_Purge_READPIN);

  // Compare the read states to the desired states and set them to be equal to the desired
  // If FUEL_Press does not match the ordered state
  if (FUEL_Press.state != FUEL_Press_Desired.state) {
    // Store current state into previous state
    FUEL_Press_Prev = FUEL_Press.state;
    // Write the new state to the pin
    digitalWrite(FUEL_Press_ACTPIN, FUEL_Press_Desired.state);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
  // If LOX_Press does not match the ordered state
  if (LOX_Press.state != LOX_Press_Desired.state) {
    // Store current state into previous state
    LOX_Press_Prev = LOX_Press.state;
    // Write the new state to the pin
    digitalWrite(LOX_Press_ACTPIN, LOX_Press_Desired.state);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
  // If FUEL_Vent does not match the ordered state
  if (FUEL_Vent.state != FUEL_Vent_Desired.state) {
    // Store current state into previous state
    FUEL_Vent_Prev = FUEL_Vent.state;
    // Write the new state to the pin
    digitalWrite(FUEL_Vent_ACTPIN, FUEL_Vent_Desired.state);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
  // If LOX_Vent does not match the ordered state
  if (LOX_Vent.state != LOX_Vent_Desired.state) {
    // Store current state into previous state
    LOX_Vent_Prev = LOX_Vent.state;
    // Write the new state to the pin
    digitalWrite(LOX_Vent_ACTPIN, LOX_Vent_Desired.state);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
  // If MAIN does not match the ordered state
  if (MAIN.state != MAIN_Desired.state) {
    // Store current state into previous state
    MAIN_Prev = MAIN.state;
    // Write the new state to the pin
    digitalWrite(MAIN_ACTPIN, MAIN_Desired.state);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
  // If FUEL_Purge does not match the ordered state
  if (FUEL_Purge.state != FUEL_Purge_Desired.state) {
    // Store current state into previous state
    FUEL_Purge_Prev = FUEL_Purge.state;
    // Write the new state to the pin
    digitalWrite(FUEL_Purge_ACTPIN, FUEL_Purge_Desired.state);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
  // If FUEL_Purge does not match the ordered state
  if (LOX_Purge.state != LOX_Purge_Desired.state) {
    // Store current state into previous state
    LOX_Purge_Prev = LOX_Purge.state;
    // Write the new state to the pin
    digitalWrite(LOX_Purge_ACTPIN, LOX_Purge_Desired.state);
    // Track the state change to send a notification to Pi
    STATE_CHANGED = true;
  }
}

void SendUpdate() {
  if (STATE_CHANGED) {
    // Serial Writes
    // Writing the Packet Start bytestring to the Serial Buffer
    Serial.write(Packet_Start.bytes,4);

    // Writing the Timestamp to the Serial Buffer
    Serial.write(TimeStamp.bytes, 4);
  
    // Writing the actuator states to the Serial Buffer
    Serial.write(FUEL_Press.bytes, 1);
    Serial.write(LOX_Press.bytes, 1);
    Serial.write(FUEL_Vent.bytes, 1);
    Serial.write(LOX_Vent.bytes, 1);
    Serial.write(MAIN.bytes, 1);
    Serial.write(FUEL_Purge.bytes, 1);
    Serial.write(LOX_Purge.bytes, 1);

    // Writing the Packet Stop bytestring to the Serial Buffer
    Serial.write(Terminator.bytes, 4);
    STATE_CHANGED = false;
  }
}
