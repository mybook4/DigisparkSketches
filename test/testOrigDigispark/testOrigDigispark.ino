void setup() {
  // put your setup code here, to run once:
  pinMode(1, OUTPUT); //on board LED 
}

void loop() {
  // put your main code here, to run repeatedly:
  
  digitalWrite(0, HIGH);
  digitalWrite(1, HIGH);
  digitalWrite(2, HIGH);
  digitalWrite(5, HIGH);
  
  delay(2000);
  
  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
  digitalWrite(2, LOW);
  digitalWrite(5, LOW);
  
  delay(2000);
  
  
}
