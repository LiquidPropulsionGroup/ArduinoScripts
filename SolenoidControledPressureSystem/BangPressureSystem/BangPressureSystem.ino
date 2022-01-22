int solenoidV = 3;
int solenoidP = 2;
int analogpin = A6;

int pressuresetting = 40; // psi
float operatingvarrance = 0.02;
float pressureupperbound;
float pressurelowerbound;
float analoginput;
float pressure;


void setup() 
{
  pinMode(solenoidP, OUTPUT);
  pinMode(solenoidV, OUTPUT);
  Serial.begin(9600);
  pressureupperbound = pressuresetting+(pressuresetting*operatingvarrance);
  Serial.println(pressureupperbound);
  pressurelowerbound = pressuresetting-(pressuresetting*operatingvarrance);
  Serial.println(pressurelowerbound);
  delay(3000);
}

void loop() 
{
  analoginput = analogRead(analogpin);
  Serial.println(analoginput);
  float voltage = analoginput * 5.0/1023;
  pressure = (analoginput - 97)*(300-0)/(1023-97)+0;
  Serial.print(voltage);
  Serial.print(" volts    ");
  Serial.print(pressure);
  Serial.println(" psi");
  
  if (pressure > pressureupperbound)
  {
    VENT();  
  }
  else if (pressure < pressurelowerbound)
  {
    PRESSURIZE();
  }
  else
  {
    digitalWrite(solenoidP,LOW);
    digitalWrite(solenoidV,LOW);
  }
  delay(10);
}

int PRESSURIZE()
{
  digitalWrite(solenoidP,HIGH);
  digitalWrite(solenoidV,LOW);
  Serial.println("PRES");
}

int VENT()
{
  digitalWrite(solenoidP,LOW);
  digitalWrite(solenoidV,HIGH);
  Serial.println("VENT");
}
