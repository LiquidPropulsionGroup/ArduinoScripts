/*
 * SKELETON CODE FOR TESTING SENSOR MESSAGE / VALVE UPDATES ON A SINGLE ARDUINO
 * Thomas W. C. Carlson
 * 9/30/21
 */

// Include necessary libraries
#include <Wire.h>           // Enables I2C
#include <stdint.h>         // Enables strict byte-size of variable types
#include <HardwareSerial.h> // Enables Serial.read() to grab 1 char from the buffer
#include <Regexp.h>         // Enables regex

// Union declarations
typedef union FourBytes{
    uint32_t int_dat;
    unsigned char bytes[4];
  };
typedef union TwoBytes {
    uint16_t int_dat;
    unsigned char bytes[2];
  };

// Union instantiations, these are shared between both SensorMessage() and ValveControl()
FourBytes Packet_Start;
FourBytes TimeStamp;
FourBytes Terminator;

// Actuation pins: HIGH activates the solenoid tied to this pin
  int FUEL_Press_ACTPIN = 13;     // DO NOT USE IN LIVE ENVIRONMENT
  int LOX_Press_ACTPIN = 12;
  int FUEL_Vent_ACTPIN = 11;      // DO NOT USE IN LIVE ENVIRONMENT
  int LOX_Vent_ACTPIN = 10;       // DO NOT USE IN LIVE ENVIRONMENT
  int MAIN_ACTPIN = 9;
  int FUEL_Purge_ACTPIN = 8;
  int LOX_Purge_ACTPIN = 7;

// Read pins: HIGH indicates that a solenoid is activated
  int FUEL_Press_READPIN = 2;
  int LOX_Press_READPIN = A5;
  int FUEL_Vent_READPIN = A4;
  int LOX_Vent_READPIN = A3;
  int MAIN_READPIN = A2;
  int FUEL_Purge_READPIN = A1;
  int LOX_Purge_READPIN = A0;

// Send value holders, to be used in Serial.write()
  char FUEL_Press_Send;
  char LOX_Press_Send;
  char FUEL_Vent_Send;
  char LOX_Vent_Send;
  char MAIN_Send;
  char FUEL_Purge_Send;
  char LOX_Purge_Send;

// Current state value holders, updated via digitalRead()
  bool FUEL_Press;
  bool LOX_Press;
  bool FUEL_Vent;
  bool LOX_Vent;
  bool MAIN;
  bool FUEL_Purge;
  bool LOX_Purge;

// Defining the received instruction message length
  const int INSTRUCTION_LENGTH = 14;
  char ReceivedChars[INSTRUCTION_LENGTH+2]; // +2 to include the start and terminator chars

// Instruction template definition for verification that the packet includes instructions
  // FUEL_Pres(S), LOX_Pres(s), FUEL_Ven(T), LOX_Ven(t), (M)ain, FUEL_Purg(E), FUEL_Purg(e)
  // Interspersed with dummy request character holders
  const char InstructionTemplate[INSTRUCTION_LENGTH] = {'S','\0','s','\0','T','\1','t','\1','M','\0','E','\0','e','\0'};

// Desired states value holders, updated by receiving instruction from Pi
  bool FUEL_Press_Desired;
  bool LOX_Press_Desired;
  bool FUEL_Vent_Desired;
  bool LOX_Vent_Desired;
  bool MAIN_Desired;
  bool FUEL_Purge_Desired;
  bool LOX_Purge_Desired;

// Gatekeeping variable, allows updating ACTPINs and sending a return message to the Pi
  bool MESSAGE_GOOD = false;

  char SensorDataMessage[40];
  char ValveDataMessage[19];

void setup() {
  // put your setup code here, to run once:
  { // Initialize serial communications
  Serial.begin(115200);
  }
  
  { // Initialize the packet structure endings
  Packet_Start.int_dat = 0;
  Terminator.int_dat = pow(2,32)-1;   //4294967295;
  }

  { // Initialize the normally <closed/open> behavior of the actuators
  FUEL_Press_Desired = false;           // NORMALLY CLOSED
  LOX_Press_Desired = false;            // NORMALLY CLOSED
  FUEL_Vent_Desired = true;             // NORMALLY OPEN
  LOX_Vent_Desired = true;              // NORMALLY OPEN
  MAIN_Desired = false;                 // NORMALLY CLOSED
  FUEL_Purge_Desired = false;           // NORMALLY CLOSED
  LOX_Purge_Desired = false;            // NORMALLY CLOSED   
  }

  { // Initialize the output pins for actuators
  pinMode(FUEL_Press_ACTPIN, OUTPUT);
  pinMode(LOX_Press_ACTPIN, OUTPUT);
  pinMode(FUEL_Vent_ACTPIN, OUTPUT);
  pinMode(LOX_Vent_ACTPIN, OUTPUT);
  pinMode(MAIN_ACTPIN, OUTPUT);
  pinMode(FUEL_Purge_ACTPIN, OUTPUT);
  pinMode(LOX_Purge_ACTPIN, OUTPUT);
  }

  { // Initialize the input pins for actuators
  pinMode(FUEL_Press_READPIN, INPUT);
  pinMode(LOX_Press_READPIN, INPUT);
  pinMode(FUEL_Vent_READPIN, INPUT);
  pinMode(LOX_Vent_READPIN, INPUT);
  pinMode(MAIN_READPIN, INPUT);
  pinMode(FUEL_Purge_READPIN, INPUT);
  pinMode(LOX_Purge_READPIN, INPUT);
  }

  { // Execute the default states to ensure correctness
    // Initialize the instruction array
  for ( int a = 0; a < sizeof(ReceivedChars);  a++ ) {
    ReceivedChars[a] = '0';
  }
    // Force all states to the defaults
    VerifyStates();
  }
  memcpy(&SensorDataMessage[0], Packet_Start.bytes, 4);
  memcpy(&SensorDataMessage[36], Terminator.bytes, 4);
  memcpy(&ValveDataMessage[0], Packet_Start.bytes, 4);
  memcpy(&ValveDataMessage[15], Terminator.bytes, 4);
}

void loop() {
  // Sensor reading, packaging, and sending
  SensorMessage();

  // Valve control and status update sending
  ValveControl();
}


void SensorMessage() {
  // Union instantiations, in packet structure order
//  FourBytes Packet_Start;
//  FourBytes TimeStamp;
  static TwoBytes PT_HE;
  static TwoBytes PT_Purge;
  static TwoBytes PT_Pneu;
  static TwoBytes PT_FUEL_PV;
  static TwoBytes PT_LOX_PV;
//  TwoBytes PT_FUEL_INJ;
  static TwoBytes PT_CHAM;
  static TwoBytes TC_FUEL_PV;
  static TwoBytes TC_LOX_PV;
  static TwoBytes TC_LOX_Valve_Main;
  static TwoBytes TC_WATER_In;
  static TwoBytes TC_WATER_Out;
  static TwoBytes TC_CHAM;
//  TwoBytes RC_LOX_Level;
  static TwoBytes FT_Thrust;
  static TwoBytes FL_WATER;
//  FourBytes Terminator;

  
  

  // Static Delay
  // Needs to be larger than the byte-length of the message divided by the baud/10 for bytes/s
  static int BUFFER_DELAY = 10;
  
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

    // Assign each data point to its spot in the message array
//  SensorMessage[4] = TimeStamp.bytes[0];
//  SensorMessage[5] = TimeStamp.bytes[1];
//  SensorMessage[6] = TimeStamp.bytes[2];
//  SensorMessage[7] = TimeStamp.bytes[3];
    memcpy(&SensorDataMessage[4],TimeStamp.bytes, 4);
//  SensorMessage[8] = PT_HE.bytes[0];
//  SensorMessage[9] = PT_HE.bytes[1];
    memcpy(&SensorDataMessage[8],PT_HE.bytes, 2);
//  SensorMessage[10] = PT_Purge.bytes[0];
//  SensorMessage[11] = PT_Purge.bytes[1];
    memcpy(&SensorDataMessage[10],PT_Purge.bytes, 2);
//  SensorMessage[12] = PT_Pneu.bytes[0];
//  SensorMessage[13] = PT_Pneu.bytes[1];
    memcpy(&SensorDataMessage[12],PT_Pneu.bytes, 2);
//  SensorMessage[14] = PT_FUEL_PV.bytes[0];
//  SensorMessage[15] = PT_FUEL_PV.bytes[1];
    memcpy(&SensorDataMessage[14],PT_FUEL_PV.bytes, 2);
//  SensorMessage[16] = PT_LOX_PV.bytes[0];
//  SensorMessage[17] = PT_LOX_PV.bytes[1];
    memcpy(&SensorDataMessage[16],PT_LOX_PV.bytes, 2);
//  SensorMessage[18] = PT_CHAM.bytes[0];
//  SensorMessage[19] = PT_CHAM.bytes[1];
    memcpy(&SensorDataMessage[18],PT_CHAM.bytes, 2);
//  SensorMessage[20] = TC_FUEL_PV.bytes[0];
//  SensorMessage[21] = TC_FUEL_PV.bytes[1];
    memcpy(&SensorDataMessage[20],TC_FUEL_PV.bytes, 2);
//  SensorMessage[22] = TC_LOX_PV.bytes[0];
//  SensorMessage[23] = TC_LOX_PV.bytes[1];
    memcpy(&SensorDataMessage[22],TC_LOX_PV.bytes, 2);
//  SensorMessage[24] = TC_LOX_Valve_Main.bytes[0];
//  SensorMessage[25] = TC_LOX_Valve_Main.bytes[1];
    memcpy(&SensorDataMessage[24],TC_LOX_Valve_Main.bytes, 2);
//  SensorMessage[26] = TC_WATER_In.bytes[0];
//  SensorMessage[27] = TC_WATER_In.bytes[1];
    memcpy(&SensorDataMessage[26],TC_WATER_In.bytes, 2);
//  SensorMessage[28] = TC_WATER_Out.bytes[0];
//  SensorMessage[29] = TC_WATER_Out.bytes[1];
    memcpy(&SensorDataMessage[28],TC_WATER_Out.bytes, 2);
//  SensorMessage[30] = TC_CHAM.bytes[0];
//  SensorMessage[31] = TC_CHAM.bytes[1];
    memcpy(&SensorDataMessage[30],TC_CHAM.bytes, 2);
//  SensorMessage[32] = FT_Thrust.bytes[0];
//  SensorMessage[33] = FT_Thrust.bytes[1];
    memcpy(&SensorDataMessage[32],FT_Thrust.bytes, 2);
//  SensorMessage[34] = FL_WATER.bytes[0];
//  SensorMessage[35] = FL_WATER.bytes[1];
    memcpy(&SensorDataMessage[34],FL_WATER.bytes, 2);

  // Send the array
  Serial.write(SensorDataMessage, 40);

  // Configurable delay
  delay(BUFFER_DELAY);
}

void ValveControl() {
  // Pull the timestamp before doing anything
  TimeStamp.int_dat = millis();

  // Receive an instruction if there is one
  ReceiveData();

  // If the instruction is valid,
  if (MESSAGE_GOOD == true) {
    // Parse the message to extract instructions
    ParseMessage();
    
    // Update and verify the states against the instruction
    VerifyStates();
    
    // Return an update message to the pi for validation and UI updates
    SendUpdate();

    // maybe useless
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
  READ_RESET:
  // Control Variables
  const char Starter = '<';
  const char Terminator = '>';
  const char STAT_UPDATE = '?';

  // For new messages, assume it is incomplete
  MESSAGE_GOOD = false;

  // If there is data on the serial input buffer
  if ( Serial.available() > 0 ) {
    // Move every item in the instruction forward once to create a rolling buffer
    for ( int i = 0; i < INSTRUCTION_LENGTH+1; i++ ) {
      ReceivedChars[i] = ReceivedChars[i+1];
    }
    ReceivedChars[INSTRUCTION_LENGTH+1] = Serial.read();
    // Validate the entirety of the rolling buffer sequentially...
    if ( ReceivedChars[0] != Starter ) {
      // Check that the first item is the starter character '<'
      // If it isn't, return out of the function without setting MESSAGE_GOOD = true
      goto READ_RESET;
      return;
    } else if ( ReceivedChars[INSTRUCTION_LENGTH+1] != Terminator ) {
      // Check that the last item is the terminator character '>'
      // It it isn't, return out of the function without setting MESSAGE_GOOD = true
      goto READ_RESET;
      return;
    } else {
      // Perform regex on the received message
      static MatchState ms;
      ms.Target( ReceivedChars );
      // Check if the message is a status update request
      char result = ms.Match ("(%?%?%?%?%?%?%?%?%?%?%?%?%?%?)");
      if (result == REGEXP_MATCHED) {
        // Status update request
        SendUpdate();
        goto READ_RESET;
        return;
      }
      // Then check if the message is an instruction
      result = ms.Match ("(S[01]s[01]T[01]t[01]M[01]E[01]e[01])");
      if (result == REGEXP_MATCHED) {
        // Matches template, allow MESSAGE_GOOD = true
      }
      else if (result == REGEXP_NOMATCH) {
        // Does not match template, reject
        goto READ_RESET;
        return;
      }
      else {
        // shit pant
        goto READ_RESET;
        return;
      }
    }
    // If the program gets past this point, it has passed all verification without returning
    // Therefore the message is good
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
  
  // With the data extracted, prepare to read a new instruction
  MESSAGE_GOOD = false;
}

void VerifyStates() {
  // Read the current sense wire states
  // This might need a delay
  FUEL_Press = digitalRead(FUEL_Press_READPIN);
  LOX_Press = digitalRead(LOX_Press_READPIN);
  FUEL_Vent = digitalRead(FUEL_Vent_READPIN);
  LOX_Vent = digitalRead(LOX_Vent_READPIN);
  MAIN = digitalRead(MAIN_READPIN);
  FUEL_Purge = digitalRead(FUEL_Purge_READPIN);
  LOX_Purge = digitalRead(LOX_Purge_READPIN); 

  // Check that the sensed state matches the commanded state...
  // If FUEL_Press does NOT match the desired state
  if ( FUEL_Press != FUEL_Press_Desired ) {
    // then update the state of ACTPIN to match
    digitalWrite(FUEL_Press_ACTPIN, FUEL_Press_Desired);
  }

  // If LOX_Press does NOT match the desired state
  if ( LOX_Press != LOX_Press_Desired ) {
    // then update the state of ACTPIN to match
    digitalWrite(LOX_Press_ACTPIN, LOX_Press_Desired);
  }

  // If FUEL_Vent does NOT match the desired state
  if ( FUEL_Vent != FUEL_Vent_Desired ) {
    // then update the state of ACTPIN to match
    digitalWrite(FUEL_Vent_ACTPIN, FUEL_Vent_Desired);
  }

  // If LOX_Vent does NOT match the desired state
  if ( LOX_Vent != LOX_Vent_Desired ) {
    // then update the state of ACTPIN to match
    digitalWrite(LOX_Vent_ACTPIN, LOX_Vent_Desired);
  }

  // If MAIN does NOT match the desired state
  if ( MAIN != MAIN_Desired ) {
    // then update the state of ACTPIN to match
    digitalWrite(MAIN_ACTPIN, MAIN_Desired);
  }

  // If FUEL_Purge does NOT match the desired state
  if ( FUEL_Purge != FUEL_Purge_Desired ) {
    // then update the state of ACTPIN to match
    digitalWrite(FUEL_Purge_ACTPIN, FUEL_Purge_Desired);
  }

  // If LOX_Purge does NOT match the desired state
  if ( LOX_Purge != LOX_Purge_Desired ) {
    // then update the state of ACTPIN to match
    digitalWrite(LOX_Purge_ACTPIN, LOX_Purge_Desired);
  }
}

void SendUpdate() {
  // Check the current status of the actuators
  // THIS COULD CAUSE A TRAINWRECK
  // MAYBE USE A CONFIGURABLE DELAY?
  FUEL_Press = digitalRead(FUEL_Press_READPIN);
  LOX_Press = digitalRead(LOX_Press_READPIN);
  FUEL_Vent = digitalRead(FUEL_Vent_READPIN);
  LOX_Vent = digitalRead(LOX_Vent_READPIN);
  MAIN = digitalRead(MAIN_READPIN);
  FUEL_Purge = digitalRead(FUEL_Purge_READPIN);
  LOX_Purge = digitalRead(LOX_Purge_READPIN); 

  // 'True' needs to be mapped to '1' for sending as byte
  // 'False' needs to be mapped to '0' for sending as byte
  FUEL_Press_Send = BoolToByte(FUEL_Press);
  LOX_Press_Send = BoolToByte(LOX_Press);
  FUEL_Vent_Send = BoolToByte(FUEL_Vent);
  LOX_Vent_Send = BoolToByte(LOX_Vent);
  MAIN_Send = BoolToByte(MAIN);
  FUEL_Purge_Send = BoolToByte(FUEL_Purge);
  LOX_Purge_Send = BoolToByte(LOX_Purge);

  // Write the full timestamped states to the buffer
  // Writing the Packet Start bytestring to the Serial Buffer


//  // Writing the Timestamp to the Serial Buffer
//  Serial.write(TimeStamp.bytes, 4);
//  
//  // Writing the actuator states to the Serial Buffer
//  Serial.write(FUEL_Press_Send);
//  Serial.write(LOX_Press_Send);
//  Serial.write(FUEL_Vent_Send);
//  Serial.write(LOX_Vent_Send);
//  Serial.write(MAIN_Send);
//  Serial.write(FUEL_Purge_Send);
//  Serial.write(LOX_Purge_Send);
//
//  // Writing the Packet Stop bytestring to the Serial Buffer
//  Serial.write(Terminator.bytes, 4);
//  Serial.write(Packet_Start.bytes,4);

//  delay(2);

  memcpy(&ValveDataMessage[4], TimeStamp.bytes, 4);
  memcpy(&ValveDataMessage[8], &FUEL_Press_Send, 1);
  memcpy(&ValveDataMessage[9], &LOX_Press_Send, 1);
  memcpy(&ValveDataMessage[10], &FUEL_Vent_Send, 1);
  memcpy(&ValveDataMessage[11], &LOX_Vent_Send, 1);
  memcpy(&ValveDataMessage[12], &MAIN_Send, 1);
  memcpy(&ValveDataMessage[13], &FUEL_Purge_Send, 1);
  memcpy(&ValveDataMessage[14], &LOX_Purge_Send, 1);

  Serial.write(ValveDataMessage, 19);

  delay(5);
}

// Map boolean value to byte
char BoolToByte(const bool Boolean) {
  char Byte;
  if (Boolean == false) {
    return Byte = '0';
  } else if (Boolean == true) {
    return Byte = '1';
  }
}

// Map byte to boolean value
bool ByteToBool(const char Byte) {
  bool Boolean;
  if (Byte == '0') {
    return Boolean = false;
  } else if (Byte == '1') {
    return Boolean = true;
  }
}
