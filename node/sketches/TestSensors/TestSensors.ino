/*
  TestSensors.ino
  Version: 0.15.2
  License: Creative Commons 2015, Trevor Stanhope
  
  Control system for a recirculating hydroponic system.
  This program is designed for Bang-Bang control, i.e. simple thresholding control of the pump relays.
  
  Todo:
  - test sensor functions
  - test response timing
  - test buffer memory
  - test set commands
*/

/* --- Libraries --- */
#include "RunningMedian.h"
#include "DallasTemperature.h"
#include "OneWire.h"
#include "stdio.h"

/* --- Constants --- */
const boolean VERBOSE = true;

// Analog Sensors
const int N_SENSOR_PIN = A0;
const int Ca_SENSOR_PIN = A1;
const int K_SENSOR_PIN = A2;
const int EC_SENSOR_PIN = A3;
const int TEMP_SENSOR_PIN = A4; 
const int pH_SENSOR_PIN = A5;

// Constants
const int CHARS = 8;
const int BUFF_SIZE = 128;
const int READ_WAIT = 2;
const int INTERVAL = 10; // standardized delay between readings/adjustments
const int SAMPLES = 500;
const int BAUD = 9600;
const int PRECISION = 2; // number of decimal places
const int DIGITS = 6; // number of digits

/* --- Variables --- */
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature temperature(&oneWire);
char pH[CHARS];
char EC[CHARS];
char N[CHARS];
char Ca[CHARS];
char K[CHARS];
char temp[CHARS];
char output_buffer[BUFF_SIZE];

/* --- Setup --- */
void setup() {
  
  // Initialize Sensor Input Pins (Analog)
  pinMode(N_SENSOR_PIN, INPUT);
  pinMode(Ca_SENSOR_PIN, INPUT);
  pinMode(pH_SENSOR_PIN, INPUT);
  pinMode(K_SENSOR_PIN, INPUT);
  pinMode(EC_SENSOR_PIN, INPUT);
  pinMode(TEMP_SENSOR_PIN, INPUT);
  
  // Start Temperature Sensors
  temperature.begin();
  
  // Start Serial
  Serial.begin(BAUD);
}

/* --- Loop --- */
void loop() {
  read_sensors();
  wait();
}

/* --- Read Sensors --- */
// Collects data and sends it to node.
void read_sensors() {
  dtostrf(test_temperature(), DIGITS, PRECISION, temp);
  dtostrf(test_pH(), DIGITS, PRECISION, pH);
  dtostrf(test_EC(), DIGITS, PRECISION, EC);
  dtostrf(test_N(), DIGITS, PRECISION, N);
  dtostrf(test_Ca(), DIGITS,PRECISION, Ca);
  dtostrf(test_K(), DIGITS, PRECISION, K);
  sprintf(output_buffer, "{'pH':%s,'EC':%s,'temp':%s,'N':%s,'Ca':%s,'K':%s}",pH,EC,temp,N,Ca,K); // concatenate message string
  Serial.println(output_buffer);
  Serial.flush();
}

/* --- Test Temperature --- */
float test_temperature() {
  float val = 0;
  temperature.requestTemperatures();
  val = temperature.getTempCByIndex(0);
  return val;
}

/* --- Test pH --- */
float test_pH() {
  long reading = 0; 
  for(int i = 0; i < SAMPLES; i++) {
    reading += analogRead(pH_SENSOR_PIN); 
  } 
  float val = reading / SAMPLES;
  return val;
}

/* --- Test EC --- */
float test_EC() {
  long reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading += analogRead(EC_SENSOR_PIN); 
  } 
  float val = 0; // reading / SAMPLES;
  return val;
}

/* --- Test (N) Nitrogen --- */
float test_N() {
  long reading = 0; 
  long sum = 0;
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading = analogRead(N_SENSOR_PIN); 
    sum = sum + reading;
    delay(READ_WAIT);
  } 
  float val = sum / SAMPLES;
  return val;
}

/* --- Test (Ca) Calcium --- */
float test_Ca() {
  long reading = 0; 
  long sum = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading = analogRead(Ca_SENSOR_PIN); 
    sum = sum + reading;
    delay(READ_WAIT);
  } 
  float val = sum / SAMPLES;
  return val;
}

/* --- Test (K) Potassium --- */
float test_K() {
  long reading = 0; 
  long sum = 0;
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading = analogRead(K_SENSOR_PIN); 
    sum = sum + reading;
    delay(READ_WAIT);
  } 
  float val = sum / SAMPLES;
  return val;
}

/* --- Wait --- */
void wait() {
  delay(INTERVAL);
}
