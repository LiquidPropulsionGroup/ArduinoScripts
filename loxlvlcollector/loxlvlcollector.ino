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
static double time_constant = log(2);                   // ln(2)
static int resistor_one = 500;                          // 500 Ohm resistor
static long resistor_two = 10000000;                    // 10 MOhm resistor
double capacitance;                                     // measured capacitance
static double EMPTY_CAPACITOR_CAPACITANCE = 5.02e-11;   // baseline air in capacitor
double rel_capacitance;                                 // relative capacitance, fractional
static double CAPACITOR_RANGE = 3.97e-9;
float frac_capacitance;
char str[255];

// For profiling the code
long runTime;
long baseTime;

// Union definitions for serialization
typedef union FourBytes{
  double floatDat;
  int32_t numDat;
  unsigned char bytes[4];
};

typedef union TwoBytes{
  uint16_t numDat;
  unsigned char bytes[2];
};

// Union instantiations
FourBytes Packet_Start;     // Byte marker for packet start
TwoBytes LOXLVL;           // LOXLVL sensor data
FourBytes Packet_End;       // Byte marker for packet start

// Packet definition
const int SENSOR_MESSAGE_LENGTH = 10;
char SensorDataMessage[SENSOR_MESSAGE_LENGTH];
const int SERIAL_DELAY = 100;

bool WATER = 1;

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
  time_high = pulseIn(pulse_pin, HIGH, 2000000);  // usec
  time_low = pulseIn(pulse_pin, LOW, 2000000);    // usec
  time_total = time_high + time_low;            // usec
  capacitance = (time_total/1e6)/(time_constant*(resistor_one + 2*resistor_two));
  // Data interpretation
  rel_capacitance = capacitance - EMPTY_CAPACITOR_CAPACITANCE;
  // Capacity as a relative proportion to a fully water-filled capacitor
  LOXLVL.numDat = rel_capacitance/CAPACITOR_RANGE*100;
  Serial.print("HIGH duration: ");
  Serial.println(time_high);
  Serial.print("LOW duration: ");
  Serial.println(time_low);
  Serial.print("Capacitance: ");
  Serial.print(capacitance*1000000000000, 4);
//  dtostre(capacitance, str, 5, 0);
//  Serial.print(str);
  Serial.println("pF");
  Serial.println("=================");

  ParseWrite_Data();
//  Debug_Runtime_Print();

  // Delay to avoid flooding
  interrupts();
  delay(SERIAL_DELAY);
  noInterrupts();
}

void Debug_Runtime_Print() {
  // Check the end time
  interrupts();
  runTime = micros();
  Serial.print("Loop time: ");
  Serial.println((runTime - baseTime)*0.001);

  // Check the time
  baseTime = micros();
  noInterrupts();
}

void ParseWrite_Data() {
  // Copy the data to the writing array as bytes
  memcpy(&SensorDataMessage[4],LOXLVL.bytes, 2);

  // Debug prints
//  Serial.println("============================");
//  Serial.print("Packet Start Mark: ");
//  Serial.println(Packet_Start.numDat);
//  Serial.print("Percent Filled: ");
//  Serial.println(LOXLVL.numDat);
//  Serial.print("Packet End Mark: ");
//  Serial.println(Packet_End.numDat);
//  Serial.write(Packet_End.bytes,4);

  // Write the data out to serial
  Serial.write(SensorDataMessage, SENSOR_MESSAGE_LENGTH);
}
