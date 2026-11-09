#include <Wire.h>
#include <VL53L1X.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define SDA_PIN     6
#define SCL_PIN     7
#define XSHUT_TOF   10  // ToF Shutdown / Enable pin

VL53L1X tof;
Adafruit_MPU6050 mpu;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  Serial.println("\n--- Initializing VL53L1X + MPU-6050 ---");

  // Step 1: Boot hardware enable for ToF
  pinMode(XSHUT_TOF, OUTPUT);
  digitalWrite(XSHUT_TOF, LOW);
  delay(50);
  digitalWrite(XSHUT_TOF, HIGH);
  delay(100);

  // Step 2: Initialize I2C bus on GPIO6 & GPIO7
  Wire.setPins(SDA_PIN, SCL_PIN);
  Wire.begin();
  delay(20);

  // Step 3: Initialize VL53L1X Distance Sensor
  tof.setTimeout(500);
  if (!tof.init()) {
    Serial.println("ERROR: VL53L1X failed to initialize!");
    while (1);
  }
  tof.setDistanceMode(VL53L1X::Long);
  tof.setMeasurementTimingBudget(50000); // 50ms per measurement
  tof.startContinuous(50);
  Serial.println("VL53L1X online at default I2C address.");

  // Step 4: Initialize MPU-6050 IMU
  if (!mpu.begin(0x68, &Wire)) {
    Serial.println("ERROR: MPU-6050 failed to initialize at 0x68!");
    while (1);
  }
  Serial.println("MPU-6050 online at 0x68.");

  // Configure MPU-6050
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("\nBoth sensors online and streaming!\n");
}

void loop() {
  // Read distance (mm)
  uint16_t dist = tof.read();

  // Read IMU data
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Output readings
  Serial.print("ToF Dist: ");
  if (tof.timeoutOccurred()) {
    Serial.print("TIMEOUT");
  } else {
    Serial.print(dist);
    Serial.print(" mm");
  }

  Serial.print("   |   Accel X: ");
  Serial.print(a.acceleration.x, 2);
  Serial.print(" m/s^2   |   Gyro Z: ");
  Serial.print(g.gyro.z, 2);
  Serial.print(" rad/s");

  Serial.println();
  delay(100);
}