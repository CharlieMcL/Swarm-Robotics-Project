// This file contains the necessary code for:
// - Testing our TT motors;
// - Learning about the L298N's logical control signals;
// - Experimenting with speed control using the ESP32 and PWM.

// Begin by assigning ESP32 pins:
const int IN1 = 21; 
const int IN2 = 22;
const int ENA = 23;
// (Logical combinations of) IN1, IN2 tell the motor driver to turn forward, in reverse, or not at all (see below).
// The ENA pin allows the ESP32 to send a PWM pulse signal to the driver. Passing 'N' creates a N/255 duty cycle.
// That is, the motor power-line is triggered on and off at high-speed to implement (N/255)-effective speed.

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
}

// The loop() function  cycles through the four steps outlined - move forward, stop, reverse, stop, then back to forward, and so on.
// Delays are given in milliseconds (ms); a delay of 1000ms = 1s.

void loop() {
  // Drive forward:
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 180); // For N = 180, we get (180/255) = 70.6% effective speed.
  delay(2000);

  // Pause:
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
  delay(1000);


  // Drive in reverse:
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, 180);
  delay(2000);

  // Pause again:
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, 0);
  delay(1000);
}
