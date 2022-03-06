/*
 * SKELETON CODE FOR TESTING SENSOR MESSAGE WITH ZERO DATA
 * Thomas W. C. Carlson
 */

int temp_TC1 = A0;
int temp_TC2 = A2;


// Include necessary libraries
#include <Wire.h>           // Enables I2C
#include <stdint.h>         // Enables strict byte-size of variable types
#include <HardwareSerial.h> // Enables Serial.read() to grab 1 char from the buffer
#include <Adafruit_ADS1X15.h> // Enables Adafruit I2C library for communicating with ADS1115

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

//TwoBytes Label;

FourBytes Terminator;

// Debug
//int loop_start;
//int loop_end;
//int loop_dur;

const int SENSOR_MESSAGE_LENGTH = 34;
char SensorDataMessage[SENSOR_MESSAGE_LENGTH];

// Static Delay
// Needs to be larger than the byte-length of the message divided by the baud/10 for bytes/s
// Theoretically 3.125ms
// Practically has to be found empirically
// Primarily limited by Adafruit I2C libraries, not sure if they can be improved upon
static int BUFFER_DELAY = 4;

Adafruit_ADS1115 adc48;
Adafruit_ADS1115 adc49;
Adafruit_ADS1115 adc4A;
Adafruit_ADS1115 adc4B;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
//  Serial.println("START");
  // Initialize packet structure
  Packet_Start.int_dat = 0;
  Terminator.int_dat = pow(2,32)-1;  //4294967295;
//  Label.int_dat = 0;
  memcpy(&SensorDataMessage[0], Packet_Start.bytes, 4);
  memcpy(&SensorDataMessage[SENSOR_MESSAGE_LENGTH  - 4], Terminator.bytes, 4);

//  Serial.println("PACKET INIT");
  // Initialize ADS1115
  adc48.begin(0x48);
  adc48.setGain(GAIN_ONE);
//  Serial.println("ADC 48 UP");
  adc49.begin(0x49);
  adc49.setGain(GAIN_ONE);
//  Serial.println("ADC 49 UP");
  adc4A.begin(0x4A);
  adc4A.setGain(GAIN_ONE);
//  Serial.println("ADC 4A UP");
  adc4B.begin(0x4B);
  adc4B.setGain(GAIN_ONE);
//  Serial.println("ADC INIT");
}

void loop() { 
  // Debug
//  loop_start = micros();
  
  // Read from ADS48
  static int16_t adc48p0, adc48p1, adc48p2, adc48p3;
  adc48p0 = adc48.readADC_SingleEnded(0);
  adc48p1 = adc48.readADC_SingleEnded(1);
  adc48p2 = adc48.readADC_SingleEnded(2);
  adc48p3 = adc48.readADC_SingleEnded(3);
//  Serial.println("ADC 48 UP");
  
  // Read from ADS49
  static int16_t adc49p0, adc49p1, adc49p2, adc49p3;
  adc49p0 = adc49.readADC_SingleEnded(0);
  adc49p1 = adc49.readADC_SingleEnded(1);
  adc49p2 = adc49.readADC_SingleEnded(2);
  adc49p3 = adc49.readADC_SingleEnded(3);
//  Serial.println("ADC 49 UP");

  // Read from ADS4A
  static int16_t adc4Ap0, adc4Ap1, adc4Ap2, adc4Ap3;
  adc4Ap0 = adc4A.readADC_SingleEnded(0);
  adc4Ap1 = adc4A.readADC_SingleEnded(1);
  adc4Ap2 = adc4A.readADC_SingleEnded(2);
//  Serial.println("ADC 4A UP");

  // Read from ADS4B
  static int16_t adc4Bp0, adc4Bp1, adc4Bp2, adc4Bp3;
  adc4Bp0 = adc4B.readADC_SingleEnded(0);
  adc4Bp1 = adc4B.readADC_SingleEnded(1);
  adc4Bp2 = adc4B.readADC_SingleEnded(2);
//  Serial.println("ADC 4B UP");
  
  float volts = 0;
  float current = 0;
//  delay(500);
  // Values to be sent over serial
  // wow this is terrible
  PT_HE.int_dat = (adc48p2*0.000125)*1180-671;
//  Serial.print("HE: ");
//  Serial.print(PT_HE.int_dat, 4);
  PT_Purge.int_dat = 20;//random(0,500);
  PT_Pneu.int_dat = 30;//random(0,500);
  PT_FUEL_PV.int_dat = (adc48p1*0.000125)*442-258;
//  Serial.print("   FUEL: ");
//  Serial.print(PT_FUEL_PV.int_dat, 4);
  PT_LOX_PV.int_dat = (adc49p2*0.000125)*427-245;
//  Serial.print("   LOX: ");
//  Serial.println(PT_LOX_PV.int_dat, 4);
//  delay(1000);
//  PT_HE.int_dat = 0;
//  for (int i=0; i<=4; i++) {
//    adc48p2 = adc49.readADC_SingleEnded(3);
////    Serial.println(" Steps: ");
////    Serial.print(adc48p2);
//    PT_HE.int_dat = PT_HE.int_dat + ((adc48p2*0.000125/150.0000*1000)-4)*1000/16;
////    Serial.print("    PSI: ");
////    Serial.print(PT_HE.int_dat);
//    volts = volts + adc48p2*0.000125;
////    Serial.print("    Volts: ");
////    Serial.print(volts);
//    current =  current + adc48p2*0.000125/150.0000*1000;
////    Serial.print("    Current: ");
//  }
//  Serial.print("PSI: ");
//    Serial.print(PT_HE.int_dat/5.0000, 4);
//  Serial.print("    VOLTS: ");
//    Serial.print(volts/5.0000, 4);
//  Serial.print("    mA: ");
//    Serial.println(current/5.0000, 4);
//    volts = adc49p2*0.000125;
//    Serial.print("CALIBRATED SUPPLY PSI: ");
//    Serial.println(volts/5*427-245, 4);
//    Serial.println(volts*1180-671, 4);
//    Serial.println("================="); 
//  Serial.print("   FUEL PV: ");
//  Serial.print(PT_FUEL_PV.int_dat);   
//  Serial.print("   LOX PV: ");
//  Serial.println(PT_LOX_PV.int_dat);
  //PT_FUEL_INJ.int_dat = random(0,500);
  PT_CHAM.int_dat = 100;//random(0,500);
  TC_FUEL_PV.int_dat = ((adc4Ap0*0.000125)-1.25) * 200;
  TC_LOX_PV.int_dat = ((adc4Ap1*0.000125)-1.25) * 200;
  TC_LOX_Valve_Main.int_dat = 130;//random(0,500);
  TC_WATER_In.int_dat = 140;//random(0,500);
  TC_WATER_Out.int_dat = 150;//random(0,500);
  TC_CHAM.int_dat = 300;//random(0,500);
  //RC_LOX_Level.int_dat = random(0,500);
  FT_Thrust.int_dat = 500;//random(0,500);
//  Label.int_dat = Label.int_dat + 1;
//  Serial.print("TC_FUEL_PV: ");
//  Serial.print(TC_FUEL_PV.int_dat);
//  Serial.print("  Steps: ");
//  Serial.print(adc4Ap0);emp2;
//  temp1 = analogRead(A0);
//  Serial.print("TC1: ");
//  Serial.print(temp1/1023.000*5, 4);
//  temp2 = analogRead(A2);
//  Serial.print("   TC2: ");
//  Serial.println(temp2/1023.000*5, 4);
//  Serial.print("TEMP 1: ");
//  Serial.print((((temp1/1023.000*5)-1.2500)*200),4);
//  Serial.print("   TEMP 2: ");
//  Serial.println((((temp2
//  Serial.print("     TC_LOX_PV: ");
//  Serial.print(TC_LOX_PV.int_dat);
//  Serial.print("  Steps: ");
//  Serial.println(adc4Ap1);
//  float temp1, t/1023.000*5)-1.2500)*200),4);
  // Serial writes
  // Assign each data point to its spot in the message array
  memcpy(&SensorDataMessage[4],PT_HE.bytes, 2);
  memcpy(&SensorDataMessage[6],PT_Purge.bytes, 2);
  memcpy(&SensorDataMessage[8],PT_Pneu.bytes, 2);
  memcpy(&SensorDataMessage[10],PT_FUEL_PV.bytes, 2);
  memcpy(&SensorDataMessage[12],PT_LOX_PV.bytes, 2);
  memcpy(&SensorDataMessage[14],PT_CHAM.bytes, 2);
  memcpy(&SensorDataMessage[16],TC_FUEL_PV.bytes, 2);
  memcpy(&SensorDataMessage[18],TC_LOX_PV.bytes, 2);
  memcpy(&SensorDataMessage[20],TC_LOX_Valve_Main.bytes, 2);
  memcpy(&SensorDataMessage[22],TC_WATER_In.bytes, 2);
  memcpy(&SensorDataMessage[24],TC_WATER_Out.bytes, 2);
  memcpy(&SensorDataMessage[26],TC_CHAM.bytes, 2);
  memcpy(&SensorDataMessage[28],FT_Thrust.bytes, 2);

//  memcpy(&SensorDataMessage[32],Label.bytes, 2);
  // Send the array
  Serial.write(SensorDataMessage, SENSOR_MESSAGE_LENGTH);

  // Debug
//  loop_end = micros();
//  loop_dur = loop_end - loop_start;
//  Serial.println(" ");
//  Serial.println("=============LOOP ENDED============");
//  Serial.println(loop_dur);


  // Configurable delay
  delay(BUFFER_DELAY);
}
