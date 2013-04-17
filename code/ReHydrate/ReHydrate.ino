/*
  ReHydrate.ino
  Version: 0.13.4
  License: Creative Commons 2013, Trevor Stanhope
  Updated: 2013-04-15
  Summary: Control system for a recirculating hydroponic system.
  Todo:
  - test sensor functions
  - test response timing
*/

/* --- Headers --- */
// Libraries
#include "SoftwareSerial.h"
#include "DallasTemperature.h"
#include "OneWire.h"
#include "stdio.h"
// I/O Pins
#define LED_PIN 13
#define OUTLET_PIN 12
#define INLET_PIN 11
#define HEAT_PIN 10
#define FLOW_PIN 9
#define PUMP_PIN 8
#define EC_READ_PIN 7 // EC_READ_PIN must be 7, 8, or 9
#define BASE_PIN 6
#define ACID_PIN 5
#define EC_ENABLE_PIN 4 // EC_ENABLE_PIN must be 4, 5, or 6
#define TEMP_PIN 3 
#define LEVEL_PIN 2
#define PH_PIN 0 // analog 0
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
#define X1 500
#define Y1 0.16
#define X2 5000
#define Y2 2

/* --- Declarations --- */
OneWire oneWire(TEMP_PIN);
DallasTemperature temperature(&oneWire);
char PH[CHARS];
char EC[CHARS];
char TEMP[CHARS];
char FLOW[CHARS];
char LEVEL[CHARS];
char DATA[MAX];

/* --- Setup --- */
void setup() {
  pinMode(HEAT_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(EC_READ_PIN, INPUT);
  pinMode(EC_ENABLE_PIN, OUTPUT);
  Serial.begin(BAUD);
  temperature.begin();
}

/* --- Loop --- */
void loop() {
  sendDATA(); // reads sensors, then sends data
  wait();
  receiveCOMMAND(); // gets command, then executes actions
  wait();
}

/* --- Send Data --- */
// Collects data and sends it to server.
void sendDATA() {
  digitalWrite(LED_PIN, HIGH);
  dtostrf(testPH(),DIGITS,PRECISION,PH);
  dtostrf(testEC(),DIGITS,PRECISION,EC);
  dtostrf(testTEMP(),DIGITS,PRECISION,TEMP);
  dtostrf(testFLOW(),DIGITS,PRECISION,FLOW);
  dtostrf(testLEVEL(),DIGITS,PRECISION,LEVEL);
  sprintf(DATA, "{'PH':%s,'EC':%s,'TEMP':%s,'FLOW':%s,'LEVEL':%s}",PH,EC,TEMP,FLOW,LEVEL); // concatenate message string
  Serial.println(DATA);
  Serial.flush();
  digitalWrite(LED_PIN, LOW);
}

/* --- Receive Command --- */
// Receives command from server and executes response. 
void receiveCOMMAND() {
  digitalWrite(LED_PIN, HIGH);
  if (Serial.available()) {
    char command = Serial.read();
    switch(command) {
    case 't': // topup
      topup(); break;
    case 'c': // cool
      cool(); break;
    case 'h': // heat
      heat(); break;
    case 'i': // inject
      inject(); break;
    case 'd': // dilute
      dilute(); break;
    case 'b': // base
      base(); break;
    case 'a': // acid
      acid(); break;
    case 's': // standby
      standby(); break;
    case 'e': // empty
      empty(); break;
    default:
      break;
    }
  Serial.flush();
  }
  digitalWrite(LED_PIN,LOW);
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
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading += analogRead(PH_PIN); 
  } 
  reading /= 100;
  val = 7 - (((((reading * VOLTAGE) / 1024) * 1000) / GAIN) / SLOPE);
  return val;
}

/* --- Test EC --- */
float testEC() {
  float val = 0;
  digitalWrite(EC_ENABLE_PIN, HIGH);
  unsigned long freqhigh = 0;
  unsigned long freqlow = 0;
  unsigned long x = 0;
  for(unsigned int j=0; j < SAMPLES; j++) {
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

/* --- Test Flow --- */
float testFLOW() {
  float val = 0;
  val = digitalRead(FLOW_PIN);
  return val;
}

/* --- Test Level --- */
int testLEVEL() {
  int val;
  val = digitalRead(LEVEL_PIN);
  return val;
}

/* --- Top Up --- */
void topup() {
  digitalWrite(INLET_PIN, HIGH);
  wait();
  digitalWrite(INLET_PIN, LOW);
}

/* --- Heat --- */
void heat() {
  digitalWrite(HEAT_PIN, HIGH);
}

/* --- Cool --- */
void cool() {
  digitalWrite(HEAT_PIN, LOW);
}

/* --- Inject --- */
void inject() {
  digitalWrite(ACID_PIN, HIGH);
  digitalWrite(BASE_PIN, HIGH);
  wait();
  digitalWrite(ACID_PIN, LOW);
  digitalWrite(BASE_PIN, LOW);
}

/* --- Dilute --- */
void dilute() {
  digitalWrite(INLET_PIN, LOW);
  wait();
  digitalWrite(INLET_PIN, HIGH);
}

/* --- Base --- */
void base() {
  digitalWrite(BASE_PIN, HIGH);
  wait();
  digitalWrite(BASE_PIN, LOW);
}

/* --- Acid --- */
void acid() {
  digitalWrite(ACID_PIN, HIGH);
  wait();
  digitalWrite(ACID_PIN, LOW);
}

/* --- Standby --- */
void standby() {
  digitalWrite(BASE_PIN, LOW);
  digitalWrite(ACID_PIN, LOW);
  digitalWrite(INLET_PIN, HIGH);
  digitalWrite(OUTLET_PIN,HIGH);
}

/* --- Empty --- */
void empty() { // flush contents
  digitalWrite(OUTLET_PIN, LOW);
  digitalWrite(INLET_PIN, HIGH);
}

/* --- Wait --- */
void wait() {
  delay(INTERVAL);
}
