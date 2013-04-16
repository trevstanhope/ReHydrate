// testPH.ino
// Author: Trevor Stanhope
// Updated: 2013-04-14
// Takes samples from pH probe.

/*
  Calibration:
  In 7pH solution, adjust gain s.t. dV = 0 across meter
  In 4pH solution, adjust slope s.t. pH = 4
  In 7pH solution, adjust slope s.t. pH = 7
*/

/* --- Headers --- */
// 4000mv is max output and 59.2 * 7 is the maximum range in mV of probe
#define PH_GAIN 9.65414 // PH_GAIN is (4000mv / (59.19 * 7))  
#define ARDUINO_VOLTAGE 5.05
#define PH 0

/* --- Declarations --- */
int samples = 100; // number of samples to take per calculation
int reading; // int for averaged pH reading
int millivolts; // int for conversion to millivolts 
float pHValue; // float for the pH value 
int i; // sampling iterator

/* --- Setup --- */
void setup() { 
  Serial.begin(9600); 
}

/* --- Loop --- */
void loop() { 
  reading = 0; 
  for(i = 0; i < samples; i++) { 
    reading += analogRead(PH); 
    delay(10);
  } 
  reading /= samples; // average readings 
  millivolts = ((reading * ARDUINO_VOLTAGE) / 1024) * 1000;  // convert reading into mV
  pHValue = 7 - ((millivolts / PH_GAIN) / 59.19); // convert mV into pH
  Serial.print("reading = "); 
  Serial.println(reading);
  Serial.print("mV = "); 
  Serial.println(millivolts);
  Serial.print("pH = "); 
  Serial.println(pHValue);
  delay(500); 
}
