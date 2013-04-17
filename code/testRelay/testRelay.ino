/*
  testRelay.ino
  Trevor Stanhope
  2013-04-17
  Controls relay switch.
*/

/* --- Headers --- */
#define RELAY_PIN 12
#define INTERVAL 1000

/* --- Declarations --- */
/* --- Setup --- */
void setup() { 
  // Initialize the digital pin as a relay controller.
  pinMode(RELAY_PIN, OUTPUT); 
}
/* --- Loop --- */
void loop() {
  digitalWrite(RELAY_PIN, HIGH); // set the RELAY on
  delay(INTERVAL); // wait for a second
  digitalWrite(RELAY_PIN, LOW); // set the RELAY off
  delay(INTERVAL); // wait for a second
}
