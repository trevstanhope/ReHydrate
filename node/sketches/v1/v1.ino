/*
  ReHydrate.ino
  Version: 0.13.4
  License: Creative Commons 2013, Trevor Stanhope
  Updated: 2015-01-31
  Summary: Control system for a recirculating hydroponic system.
  
  Todo:
  - test sensor functions
  - test response timing
*/

/* --- Headers --- */
// Libraries
#include "DallasTemperature.h"
#include "OneWire.h"
#include "stdio.h"

// I/O Pins
const int N_PUMP_PIN = 2;
const int CA_PUMP_PIN = 3;
const int K_PUMP_PIN = 4;
const int WATER_PUMP_PIN = 5;
const int HCL_PUMP_PIN = 6;

// Analog Sensors
const int N_SENSOR_PIN = A0;
const int CA_SENSOR_PIN = A1;
const int K_SENSOR_PIN = A2;
const int EC_SENSOR_PIN = A3;
const int TEMP_SENSOR_PIN = A4; 
const int PH_SENSOR_PIN = A5;

// Constants
const int CHARS = 8;
const int BUFF_SIZE = 128;
const int INTERVAL = 1000; // standardized delay between readings/adjustments
const int SAMPLES = 4096;
const int BAUD = 9600;
const int PRECISION =2; // number of decimal places
const int DIGITS = 4; // number of digits

/* --- Declarations --- */
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature temperature(&oneWire);
char PH[CHARS];
char EC[CHARS];
char N[CHARS];
char CA[CHARS];
char K[CHARS];
char TEMP[CHARS];
char OUTPUT_BUFFER[BUFF_SIZE];

/* --- Setup --- */
void setup() {
  
  // Initialize Pump Control Pins (Digital)
  pinMode(N_PUMP_PIN, OUTPUT);
  pinMode(CA_PUMP_PIN, OUTPUT);
  pinMode(K_PUMP_PIN, OUTPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  pinMode(HCL_PUMP_PIN, OUTPUT);
  
  // Initialize Sensor Input Pins (Analog)
  pinMode(N_SENSOR_PIN, INPUT);
  pinMode(CA_SENSOR_PIN, INPUT);
  pinMode(PH_SENSOR_PIN, INPUT);
  pinMode(K_SENSOR_PIN, INPUT);
  pinMode(EC_SENSOR_PIN, INPUT);
  pinMode(TEMP_SENSOR_PIN, INPUT);

  Serial.begin(BAUD);
  temperature.begin();
}

/* --- Loop --- */
void loop() {
  send_data(); // reads sensors, then sends data
  check_queue(); // gets command, then executes actions
  wait();
}

/* --- Send Data --- */
// Collects data and sends it to node.
void send_data() {
  dtostrf(test_temperature(), DIGITS, PRECISION, TEMP);
  dtostrf(test_acidity(), DIGITS, PRECISION, PH);
  dtostrf(test_conductivity(), DIGITS, PRECISION, EC);
  dtostrf(test_nitrogen(), DIGITS, PRECISION, N);
  dtostrf(test_calcium(), DIGITS,PRECISION, CA);
  dtostrf(test_potassium(), DIGITS, PRECISION, K);
  sprintf(OUTPUT_BUFFER, "{'PH':%s,'EC':%s,'TEMP':%s,'N':%s,'CA':%s,'K':%s}",PH,EC,TEMP,N,CA,K); // concatenate message string
  Serial.println(OUTPUT_BUFFER);
  Serial.flush();
}

/* --- Check Queue --- */
// Receives command from node and executes response. 
void check_queue() {
  if (Serial.available()) {
    char command = Serial.read();
    switch(command) {
    case 1:
      break;
    default:
      break;
    }
  Serial.flush();
  }
}

/* --- Test Temperature --- */
float test_temperature() {
  float val = 0;
  temperature.requestTemperatures();
  val = temperature.getTempCByIndex(0);
  return val;
}

/* --- Test pH --- */
float test_acidity() {
  float val = 0;
  int reading = 0; 
  for(int i = 0; i < SAMPLES; i++) {
    reading += analogRead(PH_SENSOR_PIN); 
  } 
  reading /= SAMPLES;
  return val;
}

/* --- Test EC --- */
float test_conductivity() {
  float val = 0;
  int reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading += analogRead(EC_SENSOR_PIN); 
  } 
  reading /= SAMPLES;
  return val;
}

/* --- Test Nitrogen --- */
float test_nitrogen() {
  float val = 0;
  int reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading += analogRead(N_SENSOR_PIN); 
  } 
  reading /= SAMPLES;
  return val;
}

/* --- Test Calcium --- */
float test_calcium() {
  float val = 0;
  int reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading += analogRead(CA_SENSOR_PIN); 
  } 
  reading /= SAMPLES;
  return val;
}

/* --- Test Potassium --- */
float test_potassium() {
  float val = 0;
  int reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading += analogRead(K_SENSOR_PIN); 
  } 
  reading /= SAMPLES;
  return val;
}

/* --- Wait --- */
void wait() {
  delay(INTERVAL);
}
