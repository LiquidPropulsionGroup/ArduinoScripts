#include <avr/interrupt.h>
#include <avr/io.h>
#include <HardwareSerial.h>

// this can be used to turn profiling on and off
#define PROFILING 1
// this needs to be true in at least ONE .c, .cpp, or .ino file in your sketch
#define PROFILING_MAIN 1
// override the number of bins
#define MAXPROF 8
#include "profiling.h"

// some handy macros for printing debugging values
#define DL(x) Serial.print(x)
#define DLn(x) Serial.println(x)
#define DV(m, v) do{Serial.print(m);Serial.print(v);Serial.print(" ");}while(0)
#define DVn(m, v) do{Serial.print(m);Serial.println(v);}while(0)

// more handy macros but unused in this example
#define InterruptOff  do{TIMSK2 &= ~(1<<TOIE2)}while(0)
#define InterruptOn  do{TIMSK2 |= (1<<TOIE2)}while(0)

// stuff used for time keeping in our ISR
volatile unsigned int int_counter;
volatile unsigned char seconds, minutes;
unsigned int tcnt2; // used to store timer value

// Arduino runs at 16 Mhz, so we have 1000 overflows per second...
// this ISR will get hit once a millisecond
ISR(TIMER2_OVF_vect) {

    int_counter++;
    if (int_counter == 1000) {
  seconds++;
  int_counter = 0;
  if(seconds == 60) {
      seconds = 0;
      minutes++;
  }
    }
#if PROFILING
    prof_array[prof_line]++;
#endif
    TCNT2 = tcnt2;  // reset the timer for next time
}

// Timer setup code borrowed from Sebastian Wallin
// http://popdevelop.com/2010/04/mastering-timer-interrupts-on-the-arduino/
// further borrowed from: http://www.desert-home.com/p/super-thermostat.html
void setupTimer (void) {
  //Timer2 Settings:  Timer Prescaler /1024
  // First disable the timer overflow interrupt while we're configuring
  TIMSK2 &= ~(1<<TOIE2);
  // Configure timer2 in normal mode (pure counting, no PWM etc.)
  TCCR2A &= ~((1<<WGM21) | (1<<WGM20));
  // Select clock source: internal I/O clock
  ASSR &= ~(1<<AS2);
  // Disable Compare Match A interrupt enable (only want overflow)
  TIMSK2 &= ~(1<<OCIE2A);

  // Now configure the prescaler to CPU clock divided by 128
  TCCR2B |= (1<<CS22)  | (1<<CS20); // Set bits
  TCCR2B &= ~(1<<CS21);             // Clear bit

  /* We need to calculate a proper value to load the timer counter.
   * The following loads the value 131 into the Timer 2 counter register
   * The math behind this is:
   * (CPU frequency) / (prescaler value) = 125000 Hz = 8us.
   * (desired period) / 8us = 125.
   * MAX(uint8) - 125 = 131;
   */
  /* Save value globally for later reload in ISR */
  tcnt2 = 131;

  /* Finally load end enable the timer */
  TCNT2 = tcnt2;
  TIMSK2 |= (1<<TOIE2);
  sei();
}

int baseTime;
int runTime;



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
    int32_t int_dat;
    unsigned char bytes[4];
  };

typedef union TwoBytes {
    int16_t int_dat;
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
  #if PROFILING
    PF(0);
    prof_has_dumped = 0;
    clear_profiling_data();
  #endif
    Serial.begin(9600);
    Serial.println("setup()");

    int_counter = 0;
    seconds = 0;
    minutes = 0;

    Serial.println("setupTimer()");
    setupTimer();
    pinMode(1, OUTPUT);







  
  // put your setup code here, to run once:
  Serial.begin(115200);
//  Serial.println("START");
  // Initialize packet structure
  Packet_Start.int_dat = 0;
  Terminator.int_dat = pow(2,32)-1;  //4294967295;
//  Label.int_dat = 0;
  memcpy(&SensorDataMessage[0], Packet_Start.bytes, 4);
  memcpy(&SensorDataMessage[SENSOR_MESSAGE_LENGTH - 4], Terminator.bytes, 4);

//  Serial.println("PACKET INIT");
  // Initialize ADS1115
  adc48.begin(0x48);
  adc48.setGain(GAIN_ONE);
  adc48.setDataRate(8);
//  Serial.println("ADC 48 UP");
  adc49.begin(0x49);
  adc49.setGain(GAIN_ONE);
//  Serial.println("ADC 49 UP");
  adc4A.begin(0x4A);
  adc4A.setGain(GAIN_TWOTHIRDS);
//  Serial.println("ADC 4A UP");
  adc4B.begin(0x4B);
  adc4B.setGain(GAIN_TWOTHIRDS);
//  Serial.println("ADC INIT");
}

void loop() {
  baseTime = millis();


   
  // Debug
//  loop_start = micros();
  
  // Read from ADS48
  PF(1);
  static int16_t adc48p0, adc48p1, adc48p2, adc48p3;
  adc48p0 = adc48.readADC_SingleEnded(0);
  adc48p1 = adc48.readADC_SingleEnded(1);
  adc48p2 = adc48.readADC_SingleEnded(2);
  adc48p3 = adc48.readADC_SingleEnded(3);
//  Serial.println("ADC 48 UP");

  PF(2);
  // Read from ADS49
  static int16_t adc49p0, adc49p1, adc49p2, adc49p3;
  adc49p0 = adc49.readADC_SingleEnded(0);
  adc49p1 = adc49.readADC_SingleEnded(1);
  adc49p2 = adc49.readADC_SingleEnded(2);
  adc49p3 = adc49.readADC_SingleEnded(3);
//  Serial.println("ADC 49 UP");

  PF(3);
  // Read from ADS4A
  static int16_t adc4Ap0, adc4Ap1, adc4Ap2, adc4Ap3;
  adc4Ap0 = adc4A.readADC_SingleEnded(0);
  adc4Ap1 = adc4A.readADC_SingleEnded(1);
  adc4Ap2 = adc4A.readADC_SingleEnded(2);
  adc4Ap3 = adc4A.readADC_SingleEnded(3);
//  Serial.println("ADC 4A UP");

  PF(4);
  // Read from ADS4B
  static int16_t adc4Bp0, adc4Bp1, adc4Bp2, adc4Bp3;
  adc4Bp0 = adc4B.readADC_SingleEnded(0);
  adc4Bp1 = adc4B.readADC_SingleEnded(1);
  adc4Bp2 = adc4B.readADC_SingleEnded(2);
  adc4Bp3 = adc4B.readADC_SingleEnded(3);
//  Serial.println("ADC 4B UP");

  PF(5);
  float volts = 0;
  float current = 0;
//  delay(500);
  // Values to be sent over serial
  // wow this is terrible

  // Collect calibrated data from PT_HE
  PT_HE.int_dat = (adc48p2*0.000125)*1180.0-671;
//  Serial.print("HE: ");
//  Serial.print(PT_HE.int_dat, 4);

  // Uncalibrated, static for testing
  // PT_Purge.int_dat = 0;//random(0,500);
  PT_Pneu.int_dat = 0;//random(0,500);

  // Collect calibrated data from PT_FUEL_PV
  PT_FUEL_PV.int_dat = (adc48p1*0.000125)*442.0-258;
//  Serial.print("   FUEL: ");
//  Serial.print(PT_FUEL_PV.int_dat, 4);

  // Collect calibrated data from PT_LOX_PV
  PT_LOX_PV.int_dat = (adc49p2*0.000125)*427.0-245;

  // Collect calibrated data from PT_FUEL_INJ
  PT_FUEL_INJ.int_dat = 300;

//  Serial.print("   LOX: ");
//  Serial.println(PT_LOX_PV.int_dat, 4);

//// Sample debug code, averages 5 samples and displays intermediate values
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

  // Uncalibrated data, static for testing
  PT_CHAM.int_dat = 0;//random(0,500);

  // Collect uncalibrated data from TC_FUEL_PV
  TC_FUEL_PV.int_dat = ((adc4Ap2*0.0001875)-1.25) * 200.0;

  // Collect uncalibrated data from TC_LOX_PV
  TC_LOX_PV.int_dat = ((adc4Ap3*0.0001875)-1.25) * 200.0;

  // Uncalibrated data, static for testing
  TC_LOX_Valve_Main.int_dat = 0;//random(0,500);
  TC_WATER_In.int_dat = 0;//random(0,500);
  TC_WATER_Out.int_dat = 0;//random(0,500);
  TC_CHAM.int_dat = 0;//random(0,500);
  //RC_LOX_Level.int_dat = random(0,500);
  FT_Thrust.int_dat = 0;//random(0,500);

// Debug
//  Label.int_dat = Label.int_dat + 1;
//  Serial.print("TC_FUEL_PV: ");
//  Serial.print(TC_FUEL_PV.int_dat);
//  Serial.print("  Steps: ");
//  Serial.print(adc4Ap0);
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

  PF(6);
  // Serial writes
  // Assign each data point to its spot in the message array
//  memcpy(&SensorDataMessage[4],PT_HE.bytes, 2);
//  memcpy(&SensorDataMessage[6],PT_Pneu.bytes, 2);
//  memcpy(&SensorDataMessage[8],PT_FUEL_PV.bytes, 2);
//  memcpy(&SensorDataMessage[10],PT_LOX_PV.bytes, 2);
//  memcpy(&SensorDataMessage[12],PT_FUEL_INJ.bytes, 2);
//  memcpy(&SensorDataMessage[14],PT_CHAM.bytes, 2);
//  memcpy(&SensorDataMessage[16],TC_FUEL_PV.bytes, 2);
//  memcpy(&SensorDataMessage[18],TC_LOX_PV.bytes, 2);
//  memcpy(&SensorDataMessage[20],TC_LOX_Valve_Main.bytes, 2);
//  memcpy(&SensorDataMessage[22],TC_WATER_In.bytes, 2);
//  memcpy(&SensorDataMessage[24],TC_WATER_Out.bytes, 2);
//  memcpy(&SensorDataMessage[26],TC_CHAM.bytes, 2);
//  memcpy(&SensorDataMessage[28],FT_Thrust.bytes, 2);

  Serial.println("===================");

//  Serial.println(adc48p2);
  Serial.println((adc48p3*0.000125));
  Serial.println(PT_HE.int_dat);
//  Serial.println(PT_Pneu.int_dat);
//  Serial.println(PT_FUEL_PV.int_dat);
//  Serial.println(PT_LOX_PV.int_dat);
//  Serial.println(PT_FUEL_INJ.int_dat);
//  Serial.println(PT_CHAM.int_dat);
//  Serial.println(TC_FUEL_PV.int_dat);
//  Serial.println(TC_LOX_PV.int_dat);
//  Serial.println(TC_LOX_Valve_Main.int_dat);
//  Serial.println(TC_WATER_In.int_dat);
//  Serial.println(TC_WATER_Out.int_dat);
//  Serial.println(TC_CHAM.int_dat);
//  Serial.println(FT_Thrust.int_dat);
//  Serial.println(adc4Ap2*0.0001875);
//  Serial.println("===================");
  // Send the array
//  Serial.write(SensorDataMessage, SENSOR_MESSAGE_LENGTH);

  // Debug
//  loop_end = micros();
//  loop_dur = loop_end - loop_start;
//  Serial.println(" ");
//  Serial.println("=============LOOP ENDED============");
//  Serial.println(loop_dur);

  // Configurable delay
  delay(BUFFER_DELAY);







  #if PROFILING
    if(seconds % 60 == 3 && !prof_has_dumped) {
  dump_profiling_data();  // also clears profiling data
    }
    if(seconds % 60 == 4 && prof_has_dumped) {
  prof_has_dumped = 0;
    }
  #endif

  
  runTime = millis();
//  Serial.print("Loop time:");
//  Serial.println(runTime-baseTime);
  delay(10);
  
}
