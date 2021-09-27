//SKELETON CODE FOR ACTUATING AND TRANSMITTING STATE
//Thomas W. C. Carlson

#include <Wire.h>
#include <stdint.h>

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

int count = 0;

// Global variables for receiving instructions over serial
const byte MESSAGE_LENGTH = 14;
char ReceivedChars[MESSAGE_LENGTH];
bool NewData;

// Comparison Packet for verifying instruction messages
// FUEL_Pres(S), LOX_Pres(s), FUEL_Ven(T), LOX_Ven(t), (M)ain, FUEL_Purg(E), FUEL_Purg(e)
// Interspersed with dummy request character holders
const char InstructionTemplate[14] = {'S','O','s','O','T','O','t','O','M','O','E','O','e','O'};

// Global variables for storing desired states
// For receiving instructions
bool FUEL_Press_Desired;
bool LOX_Press_Desired;
bool FUEL_Vent_Desired;
bool LOX_Vent_Desired;
bool MAIN_Desired;
bool FUEL_Purge_Desired;
bool LOX_Purge_Desired;


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

  // Debug
  if (count >= 10) {
    FUEL_Press.state = !FUEL_Press.state;
    count = 0;
    STATE_CHANGED = true;
  }

  // Only write to serial if the states have changed
  if (STATE_CHANGED) {
    // Serial Writes
    // Writing the Packet Start bytestring to the Serial Buffer
//    Serial.write(Packet_Start.bytes,4);

    // Writing the Timestamp to the Serial Buffer
//    Serial.write(TimeStamp.bytes, 4);
  
    // Writing the actuator states to the Serial Buffer
//    Serial.write(FUEL_Press.bytes, 1);
//    Serial.write(LOX_Press.bytes, 1);
//    Serial.write(FUEL_Vent.bytes, 1);
//    Serial.write(LOX_Vent.bytes, 1);
//    Serial.write(MAIN.bytes, 1);
//    Serial.write(FUEL_Purge.bytes, 1);
//    Serial.write(LOX_Purge.bytes, 1);

    // Writing the Packet Stop bytestring to the Serial Buffer
//    Serial.write(Terminator.bytes, 4);
    STATE_CHANGED = false;
  }
  
  // Otherwise do nothing
  // Debug
//  Serial.print(count);
//  Serial.write('\n');
  count += 1;
  delay(1000);


  ReceiveData();
  Serial.println(ReceivedChars);
  ParseMessage();
  VerifyStates();
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
  Serial.println("Do something");
  // Verify message integrity
  for (int i = 0; i <= MESSAGE_LENGTH - 2; i += 2) {
    // For every 2 items in the instruction there should be an identifier character
    if (ReceivedChars[i] == InstructionTemplate[i]) {
      // Check that it matches the template
      Serial.println(ReceivedChars[i]);
      Serial.println(InstructionTemplate[i]);
      Serial.println("VERIFIED: " + String(i));
      VerifiedCount++;
    } else if (VerifiedCount == MESSAGE_LENGTH / 2) {
      // If all characters are verified pair the instruction with the valve
      MESSAGE_GOOD = true;
      // and reset the verification counter
      VerifiedCount = 0;
    }
  }
  // Pull the instructions out of the message
  if (MESSAGE_GOOD) {
    // Update the desired states list
    FUEL_Press_Next = ReceivedChars[1];
    LOX_Press_Next = ReceivedChars[3];
    FUEL_Vent_Next = ReceivedChars[5];
    LOX_Vent_Next = ReceivedChars[7];
    MAIN_Next = ReceivedChars[9];
    FUEL_Purge_Next = ReceivedChars[11];
    LOX_Purge_Next = ReceivedChars[13];
    // And prepare to read a new message
    MESSAGE_GOOD = false;
  }
}

void VerifyStates() {
  // Check that the sense wires show a valve is what it is meant to be
  /*
    TODO
  */

  
}
