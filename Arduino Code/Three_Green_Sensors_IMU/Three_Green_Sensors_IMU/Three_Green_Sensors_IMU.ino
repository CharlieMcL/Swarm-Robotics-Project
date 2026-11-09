#include <Wire.h>
#include <VL53L1X.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

// ESP32 pin definitions
#define SDA_PIN       6
#define SCL_PIN       7
#define XSHUT_TOF1    10
#define XSHUT_TOF2    11
#define XSHUT_TOF3    15

// Hardware I2C Addresses (must all be unique)
#define TOF1_ADDR     0x30  
#define TOF2_ADDR     0x31  
#define TOF3_ADDR     0x29 
#define MPU_ADDR      0x68

VL53L1X tof1;
VL53L1X tof2;
VL53L1X tof3;
Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  Serial.println("\n--- Initializing Triple VL53L1X + MPU-6050 ---");

  // Force all three ToF sensors into hardware shutdown to reset addresses
  pinMode(XSHUT_TOF1, OUTPUT);
  pinMode(XSHUT_TOF2, OUTPUT);
  pinMode(XSHUT_TOF3, OUTPUT);
  digitalWrite(XSHUT_TOF1, LOW);
  digitalWrite(XSHUT_TOF2, LOW);
  digitalWrite(XSHUT_TOF3, LOW);
  delay(10);

  // Initialise I2C bus before sending commands
  Wire.setPins(SDA_PIN, SCL_PIN);
  Wire.begin();
  delay(20);

  // Boot and configure Sensor 1
  digitalWrite(XSHUT_TOF1, HIGH);
  delay(10);
  tof1.setTimeout(500);
  if (!tof1.init()) {
    Serial.println("ERROR: VL53L1X #1 failed to initialize!");
    while (1);
  }
  tof1.setAddress(TOF1_ADDR);
  tof1.setDistanceMode(VL53L1X::Long);
  tof1.setMeasurementTimingBudget(50000);
  tof1.startContinuous(50);
  Serial.println("VL53L1X #1 online at I2C address 0x30.");

  // Boot and configure Sensor 2
  digitalWrite(XSHUT_TOF2, HIGH);
  delay(10);
  tof2.setTimeout(500);
  if (!tof2.init()) {
    Serial.println("ERROR: VL53L1X #2 failed to initialize!");
    while (1);
  }
  tof2.setAddress(TOF2_ADDR); // Reassign address to 0x31
  tof2.setDistanceMode(VL53L1X::Long);
  tof2.setMeasurementTimingBudget(50000);
  tof2.startContinuous(50);
  Serial.println("VL53L1X #2 online at I2C address 0x31.");

  // Boot and configure Sensor 3
  digitalWrite(XSHUT_TOF3, HIGH);
  delay(10);
  tof3.setTimeout(500);
  if (!tof3.init()) {
    Serial.println("ERROR: VL53L1X #3 failed to initialize!");
    while (1);
  }
  tof3.setAddress(TOF3_ADDR); // Retains 0x29
  tof3.setDistanceMode(VL53L1X::Long);
  tof3.setMeasurementTimingBudget(50000);
  tof3.startContinuous(50);
  Serial.println("VL53L1X #3 online at I2C address 0x29.");

  // Initialise MPU-6050 IMU
  if (!mpu.begin(MPU_ADDR, &Wire)) {
    Serial.println("ERROR: MPU-6050 failed to initialize at 0x68!");
    while (1);
  }
  Serial.println("MPU-6050 online at 0x68.");

  // Configure MPU-6050
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("\nAll sensors online and streaming\n");
}

void loop() {
  // Read distance (mm) for VL53L1X sensors
  uint16_t dist1 = tof1.read();
  bool timeout1 = tof1.timeoutOccurred();

  uint16_t dist2 = tof2.read();
  bool timeout2 = tof2.timeoutOccurred();

  uint16_t dist3 = tof3.read();
  bool timeout3 = tof3.timeoutOccurred();

  // Read IMU data
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Print ToF Sensor 1
  Serial.print("ToF 1: ");
  if (timeout1) Serial.print("TIMEOUT");
  else Serial.print(dist1), Serial.print(" mm");

  // Print ToF Sensor 2
  Serial.print("  |  ToF 2: ");
  if (timeout2) Serial.print("TIMEOUT");
  else Serial.print(dist2), Serial.print(" mm");

  // Print ToF Sensor 3
  Serial.print("  |  ToF 3: ");
  if (timeout3) Serial.print("TIMEOUT");
  else Serial.print(dist3), Serial.print(" mm");

  Serial.print("  |  Accel (X, Y, Z): ");
  Serial.print(a.acceleration.x, 2);
  Serial.print(", ");
  Serial.print(a.acceleration.y, 2);
  Serial.print(", ");
  Serial.print(a.acceleration.z, 2);
  Serial.print(" m/s^2  |  Gyro (X, Y, Z): ");
  Serial.print(g.gyro.x, 2);
  Serial.print(", ");
  Serial.print(g.gyro.y, 2);
  Serial.print(", ");
  Serial.print(g.gyro.z, 2);
  Serial.print(" rad/s");

  Serial.println();
  delay(100);
}