/*
  getDATA.ino
  License: Creative Commons 2013, Trevor Stanhope.
  Updated: 2013-04-16
  Summary: Monitors EC, PH and Temp on interval
*/

/* --- Headers --- */
// Libraries
#include "SoftwareSerial.h"
#include "DallasTemperature.h"
#include "OneWire.h"
#include "stdio.h"
// I/O Pins
#define EC_ENABLE_PIN 4
#define EC_READ_PIN 7
#define TEMP_PIN 2
#define PH_PIN 0
// Constants
#define GAIN 9.65414
#define VOLTAGE 5.05 // arduino voltage
#define SLOPE 59.19 // mV per pH
#define CHARS 8
#define MAX 128
#define INTERVAL 1000 // standardized delay between readings/adjustments
#define SAMPLES 4096
#define BAUD 9600
#define PRECISION 2 // number of decimal places
#define DIGITS 4 // number of digits

/* --- Declarations --- */
OneWire oneWire(TEMP_PIN);
DallasTemperature temperature(&oneWire);
char PH[CHARS];
char EC[CHARS];
char TEMP[CHARS];
char DATA[MAX];
float X1 = 500;
float Y1 = 0.16;
float X2 = 5000;
float Y2 = 2;

/* --- Setup --- */
void setup() {
  pinMode(EC_READ_PIN, INPUT);
  pinMode(EC_ENABLE_PIN, OUTPUT);
  Serial.begin(BAUD);
  temperature.begin();
}

/* --- Loop --- */
void loop() {
  dtostrf(testPH(),DIGITS,PRECISION,PH);
  dtostrf(testEC(),DIGITS,PRECISION,EC);
  dtostrf(testTEMP(),DIGITS,PRECISION,TEMP);
  sprintf(DATA, "{'PH':%s,'EC':%s,'TEMP':%s}",PH,EC,TEMP); // concatenate message string
  Serial.println(DATA);
  wait();
}

/* --- Test Temperature --- */
float testTEMP() {
  float val = 0;
  temperature.requestTemperatures();
  val = temperature.getTempCByIndex(0);
  return val;
}

/* --- Test pH --- */
float testPH() {
  float val = 0;
  int reading = 0; 
  for(unsigned int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading += analogRead(PH_PIN); 
  } 
  reading /= 100;
  val = 7 - (((((reading * VOLTAGE) / 1024) * 1000) / GAIN) / SLOPE);
  return val;
}

/* --- Test EC --- */
float testEC() {
  float val = 0;
  unsigned long freqhigh = 0;
  unsigned long freqlow = 0;
  unsigned long x = 0;
  digitalWrite(EC_ENABLE_PIN, HIGH);
  for (unsigned int j=0; j < SAMPLES; j++) {
    freqhigh += pulseIn(EC_READ_PIN, HIGH, 250000);
    freqlow += pulseIn(EC_READ_PIN, LOW, 250000);
  }
  digitalWrite(EC_ENABLE_PIN, LOW);  
  x = 1000000 / ( (freqhigh / SAMPLES) + (freqlow / SAMPLES) );
  double m = (Y2 - Y1) / (X2 - X1);
  double b = Y1 - (m*X1);
  val = m*x + b;
  return val;
}

void wait() {
  delay(INTERVAL);
}
