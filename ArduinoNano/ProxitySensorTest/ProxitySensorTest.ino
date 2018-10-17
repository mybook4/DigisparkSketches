
// Globals (for convenience)
int trigPin = 11;    // Proximity sensor - Trig line
int echoPin = 12;    // Proximity sensor - Echo line

int pinFeather0 = 2;  // D2 ----/\/\/\/----|>|----Gnd.  All resistors 510 ohms.
int pinFeather1 = 3;  // D3 ----/\/\/\/----|>|----Gnd
int pinFeather2 = 4;  // D4 ----/\/\/\/----|>|----Gnd
int pinFeather3 = 5;  // D5 ----/\/\/\/----|>|----Gnd

// Variable to keep track of the previous number of feathers lit.
int prevFeathers;


// Setup function (executed once)
void setup() {
  
  // Serial port initialization (for debugging)
  Serial.begin (9600);
  
  // Initialize proximity sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initialize feather pins
  pinMode(pinFeather0, OUTPUT);
  pinMode(pinFeather1, OUTPUT);
  pinMode(pinFeather2, OUTPUT);
  pinMode(pinFeather3, OUTPUT);

  digitalWrite(pinFeather0, LOW);
  digitalWrite(pinFeather1, LOW);
  digitalWrite(pinFeather2, LOW);
  digitalWrite(pinFeather3, LOW);

  // Initialize other variables
  prevFeathers = 0;
}


// Function takes the distance in inches
// and returns the number of feathers to be lit
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



// Function takes the number of feathers desired to be lit,
// and lights the feather LEDs.
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



// Loop function (executed iteratively)
void loop() {
  
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
 
  // Convert the time into a distance (in inches)
  long currentInches = (duration / 2) / 74;

  // Display some debugging messages on the serial console
  Serial.print("currentInches=");
  Serial.print(currentInches);
  Serial.print(", prevFeathers=");
  Serial.print(prevFeathers);
  Serial.println();

  
  if(inchesToFeathers(currentInches) > prevFeathers) {
    // If, based on the current distance value, the number of feathers
    // that we need to light up is larger than the number of feathers
    // we previously lit up, then add another feather.
    int numFeathers = prevFeathers + 1;
    setFeathers(numFeathers);
    prevFeathers = numFeathers;
    
  } else if(inchesToFeathers(currentInches) < prevFeathers) {
    // If, based on the current distance value, the number of feathers
    // that we need to light up is fewer than the number of feathers
    // we previously lit up, then subtract a feather.
    int numFeathers = prevFeathers - 1;
    setFeathers(numFeathers);
    prevFeathers = numFeathers;
    
  } else {
    // Do nothing.  Keep the number of feathers the same.
  }

  // The loop is executed approximately 4 times per second.
  delay(250);
}
