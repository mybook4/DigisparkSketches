#include <DigiUSB.h>

void setup() {
  DigiUSB.begin();
}

void loop() {
  // print output
  DigiUSB.println("Hello");
  
  DigiUSB.delay(1000);

}
