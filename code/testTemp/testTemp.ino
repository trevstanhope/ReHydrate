// testTemp.ino
// Author: Trevor Stanhope
// Updated: 2013-04-04
// Gets temperature from DS18B20 probe

/* --- Headers --- */
#include <DallasTemperature.h>
#include <OneWire.h>
#define TEMP 2 // Digital 2 or 3 on EC Shield

/* --- Declarations --- */
OneWire oneWire(TEMP);
DallasTemperature sensors(&oneWire);

/* --- Setup --- */
void setup() {
  Serial.begin(9600);
  sensors.begin();
}

/* --- Loop --- */
void loop() {
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println(sensors.getTempCByIndex(0)); // You can have more than one IC on the same bus.
  
}

