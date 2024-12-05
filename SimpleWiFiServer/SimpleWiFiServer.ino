#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
const char *ssid = "OPN";
const char *password = "lebhikhari";
const char* serverURL = "http://192.168.84.110:5000/sensor";

Adafruit_MPU6050 mpu;

float distance = 0.0;           // Total distance traveled
float velocity = 0.0;           // Current velocity
float accelXOffset = 0.0;       // Offset for X-axis acceleration
unsigned long lastTime = 0;
const float ACCEL_THRESHOLD = 0.05;  // Threshold for acceleration noise

// Complementary filter constants
const float alpha = 0.98;       // Weight for the gyroscope data
float compAngleX = 0.0, compAngleY = 0.0; // Complementary filter angles

void calibrateSensor() {
  float totalX = 0.0;
  for (int i = 0; i < 100; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    totalX += a.acceleration.x;
    delay(10); // Small delay for each sample
  }
  accelXOffset = totalX / 100.0;
  Serial.print("Calibration complete. X-axis offset: ");
  Serial.println(accelXOffset);
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");

  if (!mpu.begin()) {
    Serial.println("Failed to initialize MPU6050");
    while (1) delay(10);
  }
  Serial.println("MPU6050 initialized successfully");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);

  // Initial calibration
  calibrateSensor();

  // Initialize complementary filter angles
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  compAngleX = atan2(a.acceleration.y, a.acceleration.z) * RAD_TO_DEG;
  compAngleY = atan(-a.acceleration.x / sqrt(a.acceleration.y * a.acceleration.y + a.acceleration.z * a.acceleration.z)) * RAD_TO_DEG;

  lastTime = millis();
}

void loop() {
  unsigned long currentTime = millis();
  float deltaTime = (currentTime - lastTime) / 1000.0;  // Convert to seconds

  // Recalibrate periodically to adjust for drift (optional)
  if (currentTime - lastTime >= 60000) { // Every 60 seconds
    calibrateSensor();
  }

  // Get acceleration and gyroscope data
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Apply calibration offset to X-axis acceleration
  float accelX = a.acceleration.x - accelXOffset;
  float accelY = a.acceleration.y;
  float accelZ = a.acceleration.z;
  float gyroX = g.gyro.x;
  float gyroY = g.gyro.y;

  // Calculate roll and pitch from accelerometer
  float accelAngleX = atan2(accelY, accelZ) * RAD_TO_DEG;
  float accelAngleY = atan(-accelX / sqrt(accelY * accelY + accelZ * accelZ)) * RAD_TO_DEG;

  // Gyroscope rates in degrees per second
  float gyroRateX = gyroX / 131.0; // Convert raw gyroscope data to degrees/s
  float gyroRateY = gyroY / 131.0;

  // Complementary filter for angle calculation
  compAngleX = alpha * (compAngleX + gyroRateX * deltaTime) + (1.0 - alpha) * accelAngleX;
  compAngleY = alpha * (compAngleY + gyroRateY * deltaTime) + (1.0 - alpha) * accelAngleY;

  // Apply threshold to ignore small noise in acceleration for distance calculation
  if (abs(accelX) < ACCEL_THRESHOLD) accelX = 0;

  // First integration: Acceleration to velocity
  velocity += accelX * deltaTime;

  // Second integration: Velocity to distance
  distance += velocity * deltaTime;

  lastTime = currentTime;

  // Print filtered angles, distance, and velocity for debugging
  Serial.print("Comp Angle X: "); Serial.print(compAngleX); Serial.print("\t");
  Serial.print("Comp Angle Y: "); Serial.print(compAngleY); Serial.print("\t");
  Serial.print("Distance: "); Serial.print(distance); Serial.print(" m\t");
  Serial.print("Velocity: "); Serial.print(velocity); Serial.println(" m/s");

  // Send distance to the server periodically
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/json");

    // JSON payload with the distance data
    String jsonData = "{\"distance\":" + String(distance) + "}";
    int httpResponseCode = http.POST(jsonData);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println("Server Response: " + response);
    } else {
      Serial.print("Error in sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }

  delay(500);  // Delay between readings
}


