//SKELETON CODE FOR ACTUATING AND TRANSMITTING STATE
//Thomas W. C. Carlson

#include <Wire.h>               // Enables I2C
#include <stdint.h>             // Enables strict byte-size of variable types
#include <HardwareSerial.h>     // Enables Serial.read
#include <Regexp.h>             // Enables regex

// Pin Declarations
int FUEL_Press_ACTPIN = A12;
int LOX_Press_ACTPIN = A10;
int FUEL_Vent_ACTPIN = A9;
int LOX_Vent_ACTPIN = A11;
int MAIN_ACTPIN = A15;
int FUEL_Purge_ACTPIN = A13;
int LOX_Purge_ACTPIN = A14;

int FUEL_Press_READPIN = 25;
int LOX_Press_READPIN = 23;
int FUEL_Vent_READPIN = 35;
int LOX_Vent_READPIN = 33;
int MAIN_READPIN = 27;
int FUEL_Purge_READPIN = 31;
int LOX_Purge_READPIN = 29;

int IGNITE_ACTPIN = 12;

// Union Declarations
typedef union FourBytes{
  uint32_t int_dat;
  unsigned char bytes[4];
};

// Union instantiations in packet structure order
// For sending status updates 
FourBytes Packet_Start;

char FUEL_Press_Send;
char LOX_Press_Send;
char FUEL_Vent_Send;
char LOX_Vent_Send;
char MAIN_Send;
char FUEL_Purge_Send;
char LOX_Purge_Send;
char IGNITE_Send;

FourBytes Terminator;

const int VALVE_MESSAGE_LENGTH = 16;
char ValveDataMessage[VALVE_MESSAGE_LENGTH];

// Thereotically 1.3ms
// Practically has to be found empirically
static int BUFFER_DELAY = 5;

// Global variables for storing current state values
bool FUEL_Press;
bool LOX_Press;
bool FUEL_Vent;
bool LOX_Vent;
bool MAIN;
bool FUEL_Purge;
bool LOX_Purge;
bool IGNITE;

// Global variables for receiving instructions over serial
const int MESSAGE_LENGTH = 16;
char ReceivedChars[MESSAGE_LENGTH+2];

// Comparison Packet for verifying instruction messages
// FUEL_Pres(S), LOX_Pres(s), FUEL_Ven(T), LOX_Ven(t), (M)ain, FUEL_Purg(E), FUEL_Purg(e), IGNITE(I)
// Interspersed with dummy request character holders
const char InstructionTemplate[16] = {'S','\0','s','\0','T','\1','t','\1','M','\0','E','\0','e','\0','I','\0'};

// Global variables for storing desired states
// For receiving instructions
bool FUEL_Press_Desired;
bool LOX_Press_Desired;
bool FUEL_Vent_Desired;
bool LOX_Vent_Desired;
bool MAIN_Desired;
bool FUEL_Purge_Desired;
bool LOX_Purge_Desired;
bool IGNITE_Desired;

bool MESSAGE_GOOD = false;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Initialize the constant union items
  Packet_Start.int_dat = 0;
  Terminator.int_dat = pow(2,32)-1;   //4294967295;

  // Initialize the unchanging parts of the valve message
  memcpy(&ValveDataMessage[0], Packet_Start.bytes, 4);
  memcpy(&ValveDataMessage[11], Terminator.bytes, 4);

  // Initialize the normally <open/closed> behavior of the actuators
  FUEL_Press_Desired = false;           // NORMALLY CLOSED
  LOX_Press_Desired = false;            // NORMALLY CLOSED
  FUEL_Vent_Desired = false;            // NORMALLY OPEN
  LOX_Vent_Desired = false;             // NORMALLY OPEN
  MAIN_Desired = false;                 // NORMALLY CLOSED
  FUEL_Purge_Desired = false;           // NORMALLY CLOSED
  LOX_Purge_Desired = false;            // NORMALLY CLOSED
  IGNITE_Desired = false;               // NORMALLY OFF

  // Initialize output pins
  pinMode(FUEL_Press_ACTPIN, OUTPUT);
  pinMode(LOX_Press_ACTPIN, OUTPUT);
  pinMode(FUEL_Vent_ACTPIN, OUTPUT);
  pinMode(LOX_Vent_ACTPIN, OUTPUT);
  pinMode(MAIN_ACTPIN, OUTPUT);
  pinMode(FUEL_Purge_ACTPIN, OUTPUT);
  pinMode(LOX_Purge_ACTPIN, OUTPUT);
  pinMode(IGNITE_ACTPIN, OUTPUT);
  
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
  ReceiveData();
  if (MESSAGE_GOOD == true) {
    ParseMessage();
    VerifyStates();
    SendUpdate();
  }
}

void ReceiveData() {
  // Label for resetting the reads
  READ_RESET: 
  // Control variables
  static byte MessageIndex = 0;
  char Starter = '<';
  char Terminator = '>';
  char ReceivedChar;
  MESSAGE_GOOD = false;
  
  if (Serial.available() > 0) {
    // move everything back by 1 index and read the next byte into the last index
      for(int i = 0; i < MESSAGE_LENGTH+1 /*All but the last character*/;i++){
          ReceivedChars[i] = ReceivedChars[i+1];
      }
      ReceivedChars[MESSAGE_LENGTH+1] = Serial.read();
      // Validate
      if (ReceivedChars[0] != Starter) {
        // First check that the first character is a message starter
        goto READ_RESET;
      }else if (ReceivedChars[MESSAGE_LENGTH+1] != Terminator) {
        // Check that the last character in the buffer is a message terminator
        goto READ_RESET;
      } else {
        // Perform regex on the received message
        static MatchState ms;
        ms.Target( ReceivedChars );
        // Check if the message is a status update request
        char result = ms.Match ("(%?%?%?%?%?%?%?%?%?%?%?%?%?%?%?)");
        if (result == REGEXP_MATCHED) {
          // Status update request
          SendUpdate();
          goto READ_RESET;
        }
        // Then check if the message is an instruction
        result = ms.Match ("(S[01]s[01]T[01]t[01]M[01]E[01]e[01]I[01])");
        if (result == REGEXP_MATCHED) {
          // Matches template, allow MESSAGE_GOOD = true
        }
        else if (result == REGEXP_NOMATCH) {
          // Does not match template, reject
          goto READ_RESET;
        }
        else {
          // shit pant
          goto READ_RESET;
        }
      }
      // If no gotos are executed, the message is correct and can be used
      MESSAGE_GOOD = true;
  }
}

void ParseMessage() {
  // Pull the instructions out of the message
  // '1' needs to be mapped to True
  // '0' needs to be mapped to False
  FUEL_Press_Desired = ByteToBool(ReceivedChars[2]);
  LOX_Press_Desired = ByteToBool(ReceivedChars[4]);
  FUEL_Vent_Desired = ByteToBool(ReceivedChars[6]);
  LOX_Vent_Desired = ByteToBool(ReceivedChars[8]);
  MAIN_Desired = ByteToBool(ReceivedChars[10]);
  FUEL_Purge_Desired = ByteToBool(ReceivedChars[12]);
  LOX_Purge_Desired = ByteToBool(ReceivedChars[14]);
  IGNITE_Desired = ByteToBool(ReceivedChars[16]);
  
  // With the data extracted, prepare to read a new instruction
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

  // Compare the read states to the desired states and set them to be equal to the desired
  
  // If FUEL_Press does not match the ordered state
  if (!FUEL_Press != FUEL_Press_Desired) {
    // Write the new state to the pin
    digitalWrite(FUEL_Press_ACTPIN, FUEL_Press_Desired);
  }
  
  // If LOX_Press does not match the ordered state
  if (!LOX_Press != LOX_Press_Desired) {
    // Write the new state to the pin
    digitalWrite(LOX_Press_ACTPIN, LOX_Press_Desired);
  }
  
  // If FUEL_Vent does not match the ordered state
  if (!FUEL_Vent != FUEL_Vent_Desired) {
    // Write the new state to the pin
    digitalWrite(FUEL_Vent_ACTPIN, FUEL_Vent_Desired);
  }
  
  // If LOX_Vent does not match the ordered state
  if (!LOX_Vent != LOX_Vent_Desired) {
    // Write the new state to the pin
    digitalWrite(LOX_Vent_ACTPIN, LOX_Vent_Desired);
  }
  
  // If MAIN does not match the ordered state
  if (!MAIN != MAIN_Desired) {
    // Write the new state to the pin
    digitalWrite(MAIN_ACTPIN, MAIN_Desired);
  }
  
  // If FUEL_Purge does not match the ordered state
  if (!FUEL_Purge != FUEL_Purge_Desired) {
    // Write the new state to the pin
    digitalWrite(FUEL_Purge_ACTPIN, FUEL_Purge_Desired);
  }
  
  // If FUEL_Purge does not match the ordered state
  if (!LOX_Purge != LOX_Purge_Desired) {
    // Write the new state to the pin
    digitalWrite(LOX_Purge_ACTPIN, LOX_Purge_Desired);
  }

  // If IGNITE does not match the ordered state
  if (IGNITE != IGNITE_Desired) {
    // Write the new state to the pin
    digitalWrite(IGNITE_ACTPIN, IGNITE_Desired);
    // Update the state since we have no detection circuit currently
    IGNITE = IGNITE_Desired;
  }
}

void SendUpdate() {
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
    IGNITE_Send = BoolToByte(IGNITE);
    
    // Serial Writes
    // Writing the actuator states to the Serial Buffer
    memcpy(&ValveDataMessage[4], &FUEL_Press_Send, 1);
    memcpy(&ValveDataMessage[5], &LOX_Press_Send, 1);
    memcpy(&ValveDataMessage[6], &FUEL_Vent_Send, 1);
    memcpy(&ValveDataMessage[7], &LOX_Vent_Send, 1);
    memcpy(&ValveDataMessage[8], &MAIN_Send, 1);
    memcpy(&ValveDataMessage[9], &FUEL_Purge_Send, 1);
    memcpy(&ValveDataMessage[10], &LOX_Purge_Send, 1);
    memcpy(&ValveDataMessage[11], &IGNITE_Send, 1);

    // Write the array to the serial buffer
    Serial.write(ValveDataMessage, VALVE_MESSAGE_LENGTH);

    // Wait for the buffer to clear
    delay(BUFFER_DELAY);
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
