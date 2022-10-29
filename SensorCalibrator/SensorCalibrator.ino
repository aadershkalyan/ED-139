int sensorPins[] = {A0, A1, A2, A3, A4, A5, A6, A7, A8};

void setup() {
  Serial.begin(9600);
}

void loop() {
  String vals = "";
  int sum = 0;
  for (int i = 0; i < 9; i++) {
    int val = analogRead(sensorPins[i]);
    vals = vals + "Pin " + i + ": " + val +"\n";
    if (val >= 200) {
        sum += pow(2, i);
    }

  }
  Serial.println(vals);
  Serial.println(sum);
  delay(2000);
}
