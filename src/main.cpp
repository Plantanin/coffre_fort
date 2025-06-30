#include <Arduino.h>

void setup() {
  Serial.begin(57600);
}

void loop() {
  Serial.println("Loop");
  delay(1000);
}

// void main(void) {
//   setup();
//   while(1) {
//     loop();
//   }
// }
