// testRelay.ino
// Trevor Stanhope
// December 8th, 2012
// Controls relay switch.

/*
http://quarkstream.wordpress.com/2009/12/11/arduino-8-relays/
*/

/* --- Headers --- */
#define RELAY_PIN 2
#define INTERVAL 1000

/* --- Declarations --- */
/* --- Functions --- */
void setup() { 
  // Initialize the digital pin as a relay controller.
  pinMode(RELAY_PIN, OUTPUT); 
}
void loop() {
  digitalWrite(RELAY_PIN, HIGH); // set the RELAY on
  delay(INTERVAL); // wait for a second
  digitalWrite(RELAY_PIN, LOW); // set the RELAY off
  delay(INTERVAL); // wait for a second
}
