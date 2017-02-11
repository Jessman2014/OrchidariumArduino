int pin = 8;
int pwr = A0;

void setup() {
  // put your setup code here, to run once:
  pinMode(pin, OUTPUT);
  pinMode(pwr, OUTPUT);
  digitalWrite(pwr, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(pin, HIGH);
  delay(3000);
  digitalWrite(pin, LOW);
  delay(3000);
}
