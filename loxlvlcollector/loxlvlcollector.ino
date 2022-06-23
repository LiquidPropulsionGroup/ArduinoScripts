/*
 * CODE FOR AUXILIARY DATA COLLECTION AND PACKAGING
 * Thomas W. C. Carlson
 * 6/22/2022
 */

/*
 * This code relies on a direct pinout connection to the loxlvl
 * astable oscillator circuit whose pulse period is related
 * to the current dielectric composition of the capacitor.
 * Capacitance = (t_high + t_low)/(ln(2)*(R_1+2*R_2))
 */
int pulse_pin = 8;
uint32_t time_high;
uint32_t time_low;
uint32_t time_total;
static double time_constant = log(2);       // ln(2)
static int resistor_one = 500;              // 500 Ohm resistor
static long resistor_two = 10000000;        // 10 MOhm resistor
float capacitance;
char str[255];

// For profiling the code
long runTime;
long baseTime;

// Union definitions for serialization
typedef union FourBytes{
  float floatDat;
  int32_t numDat;
  unsigned char bytes[4];
};

// Union instantiations
FourBytes Packet_Start;     // Byte marker for packet start
FourBytes LOXLVL;           // LOXLVL sensor data
FourBytes Packet_End;       // Byte marker for packet start

// Packet definition
const int SENSOR_MESSAGE_LENGTH = 12;
char SensorDataMessage[SENSOR_MESSAGE_LENGTH];


void setup() {
  Serial.begin(115200);

  // Packet endings, never changes
  Packet_Start.numDat = 0;
  Packet_End.numDat   = 4294967295;
  memcpy(&SensorDataMessage[0], Packet_Start.bytes, 4);
  memcpy(&SensorDataMessage[SENSOR_MESSAGE_LENGTH - 4], Packet_End.bytes, 4);

  // Pinmodes
  pinMode(pulse_pin, INPUT);

  // Interrupts mess up the pulseIn() function accuracy
  noInterrupts();
}

void loop() {
  time_high = pulseIn(pulse_pin, HIGH, 10000);  // usec
  time_low = pulseIn(pulse_pin, LOW, 10000);    // usec
  time_total = time_high + time_low;            // usec
  LOXLVL.floatDat = (time_total/1e6)/(time_constant*(resistor_one + 2*resistor_two));
//  Serial.print("HIGH duration: ");
//  Serial.println(time_high);
//  Serial.print("LOW duration: ");
//  Serial.println(time_low);
//  Serial.print("Capacitance: ");
//  dtostre(LOXLVL.floatDat, str, 5, 0);
//  Serial.print(str);
//  Serial.println("F");
//  Serial.println("=================");

  ParseWrite_Data();
//  interrupts();
//  Debug_Runtime_Print();
//  noInterrupts();
}

void Debug_Runtime_Print() {
  // Check the end time
  runTime = micros();
  Serial.print("Loop time: ");
  Serial.println((runTime - baseTime)*0.001);

  // Check the time
  baseTime = micros();
}

void ParseWrite_Data() {
  // Data interpretation
  LOXLVL.floatDat = (time_total/1e6)/(time_constant*(resistor_one + 2*resistor_two));
  
  // Copy the data to the writing array as bytes
  memcpy(&SensorDataMessage[4],LOXLVL.bytes, 4);

  // Debug prints
//  Serial.println("============================");
//  Serial.print("Packet Start Mark: ");
//  Serial.println(Packet_Start.numDat);
//  Serial.print("LOX_LVL: ");
//  Serial.println(LOXLVL.floatDat, 20);
//  Serial.print("Packet End Mark: ");
//  Serial.println(Packet_End.numDat);
//  Serial.write(Packet_End.bytes,4);

  // Write the data out to serial
  Serial.write(SensorDataMessage, SENSOR_MESSAGE_LENGTH);
}
