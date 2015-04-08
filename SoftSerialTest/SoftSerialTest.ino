#include <TinyPinChange.h>
#include <SoftSerial.h>

SoftSerial mySerial(8, 9, true); // RX, TX

void setup()  
{
  // Open serial communications and wait for port to open:
  // set the data rate for the SoftwareSerial port
  mySerial.begin(9600);
  mySerial.println("In setup function");
}

void loop() // run over and over
{
  delay(1000);
  
  if (mySerial.available())
    mySerial.println("in loop");

}
