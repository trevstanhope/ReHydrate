
/*
  ReHydrateBangB.ino
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
#include "DallasTemperature.h"
#include "OneWire.h"
#include "stdio.h"
#include <RunningMedian.h>


/* --- Constants --- */
const boolean VERBOSE = true;

// I/O Pins
const int N_PUMP_PIN = 2;
const int Ca_PUMP_PIN = 3;
const int K_PUMP_PIN = 4;
const int WATER_PUMP_PIN = 5;
const int HCL_PUMP_PIN = 6;

// Analog Sensors
const int N_SENSOR_PIN = A0;
const int Ca_SENSOR_PIN = A1;
const int K_SENSOR_PIN = A2;
const int EC_SENSOR_PIN = A3;
const int TEMP_SENSOR_PIN = A4; 
const int pH_SENSOR_PIN = A5;

// Command Chars
const char N_SET_CMD = 'N';
const char Ca_SET_CMD = 'C';
const char K_SET_CMD = 'K';
const char pH_SET_CMD = 'P';
const char EC_SET_CMD = 'E';
const char RESET_CMD = 'R';

// Constants
const int CHARS = 8;
const int BUFF_SIZE = 128;
const int READ_WAIT = 10;
const int INTERVAL = 5; // standardized delay between readings/adjustments
const int SAMPLES = 200;
const int BAUD = 9600;
const int PRECISION = 2; // number of decimal places
const int DIGITS = 6; // number of digits

// Default Setpoints
const int pH_DEFAULT = 512; // raw bit value at ideal pH
const int EC_DEFAULT = 512; // raw bit value at ideal EC
const int Ca_DEFAULT = 512; // raw bit value at ideal Ca
const int N_DEFAULT = 512; // raw bit value at ideal N
const int K_DEFAULT = 512; // raw bit value at ideal K

// PID Values
float N_P = 1.0, N_I = 0.0, N_D = 0.0;
float Ca_P = 1.0, Ca_I = 0.0, Ca_D = 0.0;
float K_P = 1.0, K_I = 0.0, K_D = 0.0;
float EC_P = 1.0, EC_I = 0.0, EC_D = 0.0;
float pH_P = 1.0, pH_I = 0.0, pH_D = 0.0;

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
double N_set, N_in, N_out;
double Ca_set, Ca_in, Ca_out;
double K_set, K_in, K_out;
double EC_set, EC_in, EC_out;
double pH_set, pH_in, pH_out;
RunningMedian N_samples = RunningMedian(SAMPLES);
RunningMedian Ca_samples = RunningMedian(SAMPLES);
RunningMedian K_samples = RunningMedian(SAMPLES);

/* --- Setup --- */
void setup() {
  // Initial
  // Initialize Pump Control Pins (Digital)
  pinMode(N_PUMP_PIN, OUTPUT);
  pinMode(Ca_PUMP_PIN, OUTPUT);
  pinMode(K_PUMP_PIN, OUTPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  pinMode(HCL_PUMP_PIN, OUTPUT);
  
  // Initialize Sensor Input Pins (Analog)
  pinMode(N_SENSOR_PIN, INPUT);
  pinMode(Ca_SENSOR_PIN, INPUT);
  pinMode(pH_SENSOR_PIN, INPUT);
  pinMode(K_SENSOR_PIN, INPUT);
  pinMode(EC_SENSOR_PIN, INPUT);
  pinMode(TEMP_SENSOR_PIN, INPUT);
  
  // Turn the PID on
  N_set = N_DEFAULT;
  Ca_set = Ca_DEFAULT;
  K_set = K_DEFAULT;
  EC_set = EC_DEFAULT;
  pH_set = pH_DEFAULT;
  
  // Start Temperature Sensors
  temperature.begin();
  
  // Start Serial
  Serial.begin(BAUD);
}

/* --- Loop --- */
void loop() {
  read_sensors();
  check_queue();
  control_pumps();
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

/* --- Check Queue --- */
// Receives command from node and executes response. 
void check_queue() {
  if (Serial.available()) {
    
    // TODO: This section should include parse failure handling
    char command = Serial.read();
    int set_point = Serial.parseInt();
    
    // Switch structure for setpoint of which nutrient
    switch(command) {
      case N_SET_CMD:
        N_set = set_point;
        break;
      case Ca_SET_CMD:
        Ca_set = set_point;
        break;
      case K_SET_CMD:
        K_set = set_point;
        break;
      case EC_SET_CMD:
        EC_set = set_point;
        break;
      case pH_SET_CMD:
        pH_set = set_point;
        break;
      case RESET_CMD:
        N_set = N_DEFAULT;
        Ca_set = Ca_DEFAULT;
        K_set = K_DEFAULT;
        EC_set = EC_DEFAULT;
        pH_set = pH_DEFAULT;
        break;
      default:
        break;
    }
  Serial.flush();
  }
}

/* --- Control Pumps --- */
// Calculate PID values and control pumps
void control_pumps() {
  
  // Compute output value
  N_out = N_in;
  K_out = K_in;
  Ca_out = Ca_in;
  pH_out = pH_in;
  EC_out = EC_in;
  
  /* --- Nutrient Application Decision Tree --- */
  if (N_out > N_set) {
    digitalWrite(N_PUMP_PIN, HIGH);   // Add Nitrogen Solution  
  }
  else if (N_out < N_set) {
    digitalWrite(N_PUMP_PIN, LOW);
  }
  
  if (Ca_out > Ca_set) {
    digitalWrite(Ca_PUMP_PIN, LOW);
  }
  else if (Ca_out < Ca_set) {
    digitalWrite(Ca_PUMP_PIN, HIGH);  // Add Calcium Solution
  }
  
  if (K_out > K_set) {
    digitalWrite(K_PUMP_PIN, LOW);    
  }
  else if (K_out < K_set) {
    digitalWrite(K_PUMP_PIN, HIGH); // Potassium Solution
  }
  
  if (EC_out > EC_set) {
    digitalWrite(WATER_PUMP_PIN, HIGH);  // Add Water
  }
  else if (EC_out < EC_set) {
    digitalWrite(WATER_PUMP_PIN, LOW);
  }
  
  if (pH_out > pH_set) {
    digitalWrite(HCL_PUMP_PIN, HIGH);  // Add HCL Solution
  }
  else if (pH_out < pH_set) {
    digitalWrite(HCL_PUMP_PIN, LOW);
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
float test_pH() {
  long reading = 0; 
  for(int i = 0; i < SAMPLES; i++) {
    reading += analogRead(pH_SENSOR_PIN); 
  } 
  float val = reading / SAMPLES;
  pH_in = double(val); // #! Side effect
  return val;
}

/* --- Test EC --- */
float test_EC() {
  long reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading += analogRead(EC_SENSOR_PIN); 
  } 
  float val = 0; // reading / SAMPLES;
  EC_in = double(val); // #! Side effect
  return val;
}

/* --- Test (N) Nitrogen --- */
float test_N() {
  long reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading = analogRead(N_SENSOR_PIN); 
    N_samples.add(reading);
    delay(READ_WAIT);
  } 
  float val = N_samples.getAverage();
  N_in = double(val); // #! Side effect
  return val;
}

/* --- Test (Ca) Calcium --- */
float test_Ca() {
  long reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading = analogRead(Ca_SENSOR_PIN); 
    Ca_samples.add(reading);
    delay(READ_WAIT);
  } 
  float val = Ca_samples.getAverage();
  Ca_in = double(val); // #! Side effect
  return val;
}

/* --- Test (K) Potassium --- */
float test_K() {
  long reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading = analogRead(K_SENSOR_PIN); 
    K_samples.add(reading);
    delay(READ_WAIT);
  } 
  float val = K_samples.getAverage();
  K_in = double(val); // #! Side effect
  return val;
}

/* --- Wait --- */
void wait() {
  delay(INTERVAL);
}
