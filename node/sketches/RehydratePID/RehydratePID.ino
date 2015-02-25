/*
  ReHydratePID.ino
  Version: 0.15.2
  License: Creative Commons 2015, Trevor Stanhope
  
  Control system for a recirculating hydroponic system.
  
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
#include "PID_v1.h"

/* --- Constants --- */
const boolean VERBOSE = true;

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
const int READ_WAIT = 20;
const int INTERVAL = 5000; // standardized delay between readings/adjustments
const int SAMPLES = 20;
const int BAUD = 9600;
const int PRECISION = 2; // number of decimal places
const int DIGITS = 6; // number of digits
const int PH_DEFAULT = 512; // raw bit value at ideal pH
const int EC_DEFAULT = 512; // raw bit value at ideal EC
const int CA_DEFAULT = 512; // raw bit value at ideal Ca
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
PID N_pid(&N_in, &N_out, &N_set, N_P, N_I, N_D, DIRECT);
double Ca_set, Ca_in, Ca_out;
PID Ca_pid(&Ca_in, &Ca_out, &Ca_set, Ca_P, Ca_I, Ca_D, DIRECT);
double K_set, K_in, K_out;
PID K_pid(&K_in, &K_out, &K_set, K_P, K_I, K_D, DIRECT);
double EC_set, EC_in, EC_out;
PID EC_pid(&EC_in, &EC_out, &EC_set, EC_P, EC_I, EC_D, DIRECT);
double pH_set, pH_in, pH_out;
PID pH_pid(&pH_in, &pH_out, &pH_set, pH_P, pH_I, pH_D, DIRECT);
RunningMedian N_samples = RunningMedian(SAMPLES);
RunningMedian Ca_samples = RunningMedian(SAMPLES);
RunningMedian K_samples = RunningMedian(SAMPLES);

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

  // Turn the PID on
  N_set = N_DEFAULT;
  Ca_set = CA_DEFAULT;
  K_set = K_DEFAULT;
  EC_set = EC_DEFAULT;
  pH_set = PH_DEFAULT;
  N_pid.SetMode(AUTOMATIC);
  Ca_pid.SetMode(AUTOMATIC);
  K_pid.SetMode(AUTOMATIC);
  EC_pid.SetMode(AUTOMATIC);
  pH_pid.SetMode(AUTOMATIC);
  
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
  dtostrf(test_acidity(), DIGITS, PRECISION, pH);
  dtostrf(test_conductivity(), DIGITS, PRECISION, EC);
  dtostrf(test_nitrogen(), DIGITS, PRECISION, N);
  dtostrf(test_calcium(), DIGITS,PRECISION, Ca);
  dtostrf(test_potassium(), DIGITS, PRECISION, K);
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
    if (VERBOSE) {
      Serial.println(set_point);
      Serial.println(command);
    }
    
    // Switch structure for setpoint of which nutrient
    switch(command) {
      case 'N':
        N_set = set_point;
        break;
      case 'C':
        Ca_set = set_point;
        break;
      case 'K':
        K_set = set_point;
        break;
      case 'E':
        EC_set = set_point;
        break;
      case 'P':
        pH_set = set_point;
        break;
      case 'R':
        N_set = N_DEFAULT;
        Ca_set = CA_DEFAULT;
        K_set = K_DEFAULT;
        EC_set = EC_DEFAULT;
        pH_set = PH_DEFAULT;
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
  
  // Compute PID
  N_pid.Compute();
  Ca_pid.Compute();
  K_pid.Compute();
  pH_pid.Compute();
  EC_pid.Compute();
  
  if (VERBOSE) {
    Serial.println(N_out);
    Serial.println(Ca_out);
    Serial.println(K_out);
    Serial.println(pH_out);
    Serial.println(EC_out);
  }
  
  /* --- Nutrient Application Decision Tree --- */
  if (N_out > N_set) {
    digitalWrite(N_PUMP_PIN, HIGH);   // Add Nitrogen Solution  
  }
  else if (N_out < N_set) {
    digitalWrite(N_PUMP_PIN, LOW);
  }
  
  if (Ca_out > Ca_set) {
    digitalWrite(CA_PUMP_PIN, LOW);
  }
  else if (Ca_out < Ca_set) {
    digitalWrite(CA_PUMP_PIN, HIGH);  // Add Calcium Solution
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
float test_acidity() {
  long reading = 0; 
  for(int i = 0; i < SAMPLES; i++) {
    reading += analogRead(PH_SENSOR_PIN); 
  } 
  float val = reading / SAMPLES;
  pH_in = double(val); // #! Side effect
  return val;
}

/* --- Test EC --- */
float test_conductivity() {
  long reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading += analogRead(EC_SENSOR_PIN); 
  } 
  float val = 0; // reading / SAMPLES;
  EC_in = double(val); // #! Side effect
  return val;
}

/* --- Test (N) Nitrogen --- */
float test_nitrogen() {
  long reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading = analogRead(N_SENSOR_PIN); 
    N_samples.add(reading);
    delay(READ_WAIT);
  } 
  float val = N_samples.getMedian();
  N_in = double(val); // #! Side effect
  return val;
}

/* --- Test (Ca) Calcium --- */
float test_calcium() {
  long reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading = analogRead(CA_SENSOR_PIN); 
    Ca_samples.add(reading);
  } 
  float val = Ca_samples.getMedian();
  Ca_in = double(val); // #! Side effect
  return val;
}

/* --- Test (K) Potassium --- */
float test_potassium() {
  long reading = 0; 
  for(int i = 0; i < SAMPLES; i++) { // sample 100 times
    reading += analogRead(K_SENSOR_PIN); 
  } 
  float val = reading / SAMPLES;
  K_in = double(val); // #! Side effect
  return val;
}

/* --- Wait --- */
void wait() {
  delay(INTERVAL);
}
