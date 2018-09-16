
int trigPin = 11;    //Trig - green Jumper
int echoPin = 12;    //Echo - yellow Jumper
int onboardLEDPin = 13; // D13 onboard Arduino Nano LED
int offboardLEDPin = 3; // Offboard LED - D3 --/\/\/\----|>|----Gnd
//////////////////////                         470 ohm
long duration, cm, inches;

void setup() {
  //Serial Port begin
  Serial.begin (9600);
  //Define inputs and outputs
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(onboardLEDPin, OUTPUT);
  pinMode(offboardLEDPin, OUTPUT);
  digitalWrite(onboardLEDPin, LOW);
  digitalWrite(offboardLEDPin, LOW);
}

void loop()
{
  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
 
  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
 
  // convert the time into a distance
  cm = (duration/2) / 29.1;
  inches = (duration/2) / 74;

  if(inches > 30) {
    digitalWrite(onboardLEDPin, HIGH);
    digitalWrite(offboardLEDPin, HIGH);
  } else if(inches > 20) {
    digitalWrite(onboardLEDPin, HIGH);
    digitalWrite(offboardLEDPin, LOW);
  } else if(inches > 10) {
    digitalWrite(onboardLEDPin, LOW);
    digitalWrite(offboardLEDPin, HIGH);
  } else {
    digitalWrite(onboardLEDPin, LOW);
    digitalWrite(offboardLEDPin, LOW);
  }
  
  Serial.print(inches);
  Serial.print("in, ");
  Serial.print(cm);
  Serial.print("cm");
  Serial.println();
  
  delay(1000);
}
