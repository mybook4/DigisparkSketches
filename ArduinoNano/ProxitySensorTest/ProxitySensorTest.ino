
int trigPin = 11;    //Trig - green Jumper
int echoPin = 12;    //Echo - yellow Jumper


//int onboardLEDPin = 13; // D13 onboard Arduino Nano LED
//int offboardLEDPin = 3; // Offboard LED - D3 --/\/\/\----|>|----Gnd
                        //                     470 ohm

int pinFeather0 = 2;
int pinFeather1 = 3;
int pinFeather2 = 4;
int pinFeather3 = 5;

long currentInches;
int prevFeathers;

void setup() {
  //Serial Port begin
  Serial.begin (9600);
  //Define inputs and outputs
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  //pinMode(onboardLEDPin, OUTPUT);
  //pinMode(offboardLEDPin, OUTPUT);

  pinMode(pinFeather0, OUTPUT);
  pinMode(pinFeather1, OUTPUT);
  pinMode(pinFeather2, OUTPUT);
  pinMode(pinFeather3, OUTPUT);

  //digitalWrite(onboardLEDPin, LOW);
  //digitalWrite(offboardLEDPin, LOW);
  digitalWrite(pinFeather0, LOW);
  digitalWrite(pinFeather1, LOW);
  digitalWrite(pinFeather2, LOW);
  digitalWrite(pinFeather3, LOW);

  // initialize variables
  currentInches = 0;
  prevFeathers = 0;
}



int inchesToFeathers(long i) {

  if(i > 40) {
    return 0;
    
  } else if(i > 25) {
    return 1;
    
  } else if(i > 16) {
    return 2;
    
  } else if(i > 8) {
    return 3;
    
  } else {
    return 4;
  }
  
}


void setFeathers(int i) {

  int n = i;

  if(n > 4) {
    n = 4;
    
  } else if(n < 0) {
    n = 0;
    
  } else {
    // keep n as-is
  }



  switch(n) {
    case 0:
      digitalWrite(pinFeather0, LOW);
      digitalWrite(pinFeather1, LOW);
      digitalWrite(pinFeather2, LOW);
      digitalWrite(pinFeather3, LOW);
      break;
      
    case 1:
      digitalWrite(pinFeather0, HIGH);
      digitalWrite(pinFeather1, LOW);
      digitalWrite(pinFeather2, LOW);
      digitalWrite(pinFeather3, LOW);
      break;
      
    case 2:
      digitalWrite(pinFeather0, HIGH);
      digitalWrite(pinFeather1, HIGH);
      digitalWrite(pinFeather2, LOW);
      digitalWrite(pinFeather3, LOW);
      break;   

    case 3:
      digitalWrite(pinFeather0, HIGH);
      digitalWrite(pinFeather1, HIGH);
      digitalWrite(pinFeather2, HIGH);
      digitalWrite(pinFeather3, LOW);
      break; 

    case 4:
      digitalWrite(pinFeather0, HIGH);
      digitalWrite(pinFeather1, HIGH);
      digitalWrite(pinFeather2, HIGH);
      digitalWrite(pinFeather3, HIGH);
      break;
      
    default:
      digitalWrite(pinFeather0, LOW);
      digitalWrite(pinFeather1, LOW);
      digitalWrite(pinFeather2, LOW);
      digitalWrite(pinFeather3, LOW);
      break;
  }
  
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
  long duration = pulseIn(echoPin, HIGH);
 
  // convert the time into a distance (in inches)
  currentInches = (duration / 2) / 74;

  //Serial.print("current=");
  //Serial.print(currentInches);
  //Serial.print(", prevF=");
  //Serial.print(prevFeathers);
  //Serial.println();

  if(inchesToFeathers(currentInches) > prevFeathers) {
    int numFeathers = prevFeathers + 1;
    setFeathers(numFeathers);
    prevFeathers = numFeathers;
    
  } else if(inchesToFeathers(currentInches) < prevFeathers) {
    int numFeathers = prevFeathers - 1;
    setFeathers(numFeathers);
    prevFeathers = numFeathers;
    
  } else {
    // Do nothing.  Keep the number of feathers the same.
  }

  
  delay(250);
}
