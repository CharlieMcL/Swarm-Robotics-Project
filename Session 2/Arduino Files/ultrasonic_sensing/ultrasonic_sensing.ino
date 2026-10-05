// This file contains the code to:
// - Trigger our ultrasonic HC-SR04 sensors
// - Calculating and printing measured distances

// First assign the ESP32 pins:
const int TRIG_PIN = 22;
const int ECHO_PIN = 23;

void setup() {
  Serial.begin(115200); // We initialise serial communication at a rate of 115200 baud (symbols/second), for debugging.
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

void loop() {
  // First we clear the trigger-line, followed by a 2us pause:
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Then send a 10us pulse to trigger the sensor:
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Now measure the return-pulse's duration; wait for ECHO_PIN to go high, start timing, then stop when it goes low.
  // We also set a timeout of 3,000,000us = 1s, which corresponds to a max distance of 5m.
  long duration = pulseIn(ECHO_PIN, HIGH, 3000000);

  if(duration == 0) {
    Serial.println("Target out of range / sensor has been disconnected!"); // Error message for being out of ~5m range, or sensor being disconnected
  } else {
    float dist_cm = (duration*0.0343) / 2.0; // Calculate distance based on duration 
    Serial.print("Distance: ");
    Serial.print(dist_cm);
    Serial.println(" cm");
  }

  // Lastly, add a delay between measurements; we get 5 measurements per second.
  delay(200);
}