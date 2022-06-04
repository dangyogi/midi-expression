
#define AREF  1

void setup() {
  // put your setup code here, to run once:
  pinMode(AREF, OUTPUT);
  Serial.begin(9600);
  Serial.print("A5: ");
  Serial.println(A5);
  Serial.print("A7: ");
  Serial.println(A7);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.println("HIGH");
  digitalWrite(AREF, HIGH);
  delay(1000);
  Serial.println("LOW");
  digitalWrite(AREF, LOW);
  delay(1000);
}
