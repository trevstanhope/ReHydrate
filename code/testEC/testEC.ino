// testEC.ino
// Author: Trevor Stanhope
// Updated: 2013-04-14

/*
  Equations:
  EC slope 1.90% / deg C
  EC25 = temp compensate to 25C
  EC25 = ECf / (1 + x(t - 25)) 
  EC18 = temp compensate to 18C
  EC18 = ECf / (1+ x(t - 18)) 
  ECf = ec value
  x = coefficent (1.9%)
  t = temperature in C

  Calibration:
  1. Place your probe into the low salinity solution and wait 10-20 minutes.
  2. Record the frequency that the 555 generates.
  3. Place your probe into the high salinity solution and wait 10-20 minutes.
  4. Record the frequency that the 555 generates.

  Data Observations:
  Tap water: 1945Hz
  Unconnected: 1404 Hz
  5ml Fertilizer per Trev: 5524 Hz
  5ml Fertilizer per 1L: 5882 Hz
*/
 
/* --- Headers --- */
#define SAMPLES 4096
#define ENABLE_PIN 4
#define READ_PIN 7
 
/* --- Declarations --- */
float X1 = 500;
float X2 = 5000;
float Y1 = 0.16;
float Y2 = 2;
unsigned long freqhigh = 0;
unsigned long freqlow = 0;
unsigned long y = 0;
 
/* --- Setup --- */
void setup() {
  pinMode(READ_PIN, INPUT);
  pinMode(ENABLE_PIN, OUTPUT);
  Serial.begin(9600);
}

/* --- Loop --- */
void loop() {
  digitalWrite(ENABLE_PIN, HIGH);
  unsigned long freqhigh = 0;
  unsigned long freqlow = 0;
  unsigned long x = 0;
  for(unsigned int j=0; j < SAMPLES; j++) {
    freqhigh += pulseIn(7, HIGH, 250000);
    freqlow += pulseIn(7, LOW, 250000);
  }
  x = 1000000 / ( (freqhigh / SAMPLES) + (freqlow / SAMPLES) );
  Serial.print("Hz: ");
  Serial.println(x);
  double m = (Y2 - Y1) / (X2 - X1);
  double b = Y1 - (m*X1);
  double y = m*x + b;
  Serial.print("mS/Hz:");
  Serial.println(m,4);
  Serial.print("Minimum:");
  Serial.println(b,4);
  Serial.print("Salinity: ");
  Serial.println(y);
  digitalWrite(ENABLE_PIN, LOW);  
  delay(1000);
}
