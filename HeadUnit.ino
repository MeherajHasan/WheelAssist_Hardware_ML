// //////////////////////////////////////////////////
// // ESP32 TRANSMITTER
// // MPU6050 + ESP-NOW
// //////////////////////////////////////////////////

// #include <WiFi.h>
// #include <esp_now.h>
// #include <Wire.h>
// #include <Adafruit_MPU6050.h>
// #include <Adafruit_Sensor.h>

// Adafruit_MPU6050 mpu;

// // ← PASTE YOUR RECEIVER MAC HERE
// uint8_t receiverMAC[] = {0x00, 0x70, 0x07, 0x24, 0xC9, 0xB4};

// #define CMD_STOP     0
// #define CMD_FORWARD  1
// #define CMD_BACKWARD 2
// #define CMD_LEFT     3
// #define CMD_RIGHT    4

// #define CALIB_SAMPLES 200
// #define EMA_ALPHA     0.1f

// #define DEADZONE     2.0f
// #define TILT_THR     5.0f

// typedef struct {
//   int command;
//   int speed;
// } ControlPacket;

// ControlPacket packet;

// float offsetX = 0.0f;
// float offsetY = 0.0f;
// float smoothX = 0.0f;
// float smoothY = 0.0f;

// int lastCommand = CMD_STOP;

// //////////////////////////////////////////////////
// // CALLBACKS
// //////////////////////////////////////////////////

// void onDataSent(const wifi_tx_info_t *info,
//                 esp_now_send_status_t status) {}

// //////////////////////////////////////////////////
// // CALIBRATION
// //////////////////////////////////////////////////

// void calibrateMPU() {
//   Serial.println("Calibrating — hold the headset still...");
//   float sumX = 0, sumY = 0;

//   for (int i = 0; i < CALIB_SAMPLES; i++) {
//     sensors_event_t a, g, temp;
//     mpu.getEvent(&a, &g, &temp);
//     sumX += a.acceleration.x;
//     sumY += a.acceleration.y;
//     delay(10);
//   }

//   offsetX = sumX / CALIB_SAMPLES;
//   offsetY = sumY / CALIB_SAMPLES;
//   smoothX = 0;
//   smoothY = 0;

//   Serial.print("OffsetX: "); Serial.print(offsetX);
//   Serial.print("  OffsetY: "); Serial.println(offsetY);
//   Serial.println("Calibration done.");
// }

// //////////////////////////////////////////////////
// // MPU SETUP
// //////////////////////////////////////////////////

// void setupMPU() {
//   Wire.begin(21, 22);   // SDA=21, SCL=22

//   if (!mpu.begin()) {
//     Serial.println("MPU6050 NOT FOUND — check wiring");
//     while (1) delay(500);
//   }

//   mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
//   mpu.setGyroRange(MPU6050_RANGE_500_DEG);
//   mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

//   Serial.println("MPU6050 OK");
//   calibrateMPU();
// }

// //////////////////////////////////////////////////
// // ESP-NOW SETUP
// //////////////////////////////////////////////////

// void setupESPNow() {
//   WiFi.mode(WIFI_STA);

//   if (esp_now_init() != ESP_OK) {
//     Serial.println("ESP-NOW INIT FAILED");
//     while (1) delay(500);
//   }

// esp_now_register_send_cb(onDataSent);

//   esp_now_peer_info_t peer = {};
//   memcpy(peer.peer_addr, receiverMAC, 6);
//   peer.channel = 0;
//   peer.encrypt = false;

//   if (esp_now_add_peer(&peer) != ESP_OK) {
//     Serial.println("FAILED TO ADD PEER");
//     while (1) delay(500);
//   }

//   Serial.println("ESP-NOW OK");
// }

// //////////////////////////////////////////////////
// // MOVEMENT DETECTION + SEND
// //////////////////////////////////////////////////

// void detectAndSend() {
//   sensors_event_t a, g, temp;
//   mpu.getEvent(&a, &g, &temp);

//   float rawX = a.acceleration.x - offsetX;
//   float rawY = a.acceleration.y - offsetY;

//   smoothX = EMA_ALPHA * rawX + (1.0f - EMA_ALPHA) * smoothX;
//   smoothY = EMA_ALPHA * rawY + (1.0f - EMA_ALPHA) * smoothY;

//   int newCmd = CMD_STOP;

//   if (fabs(smoothX) < DEADZONE && fabs(smoothY) < DEADZONE) {
//     newCmd = CMD_STOP;
//   } else if (smoothX >  TILT_THR)  newCmd = CMD_FORWARD;
//   else if   (smoothX < -TILT_THR)  newCmd = CMD_BACKWARD;
//   else if   (smoothY >  TILT_THR)  newCmd = CMD_LEFT;
//   else if   (smoothY < -TILT_THR)  newCmd = CMD_RIGHT;

//   // Only transmit on command change — reduces spam
//   if (newCmd != lastCommand) {
//     packet.command = newCmd;
//     packet.speed   = 120;
//     esp_now_send(receiverMAC, (uint8_t *)&packet, sizeof(packet));
//     lastCommand = newCmd;

//     Serial.print("X: "); Serial.print(smoothX, 2);
//     Serial.print("  Y: "); Serial.print(smoothY, 2);
//     Serial.print("  → ");
//     const char* names[] = {"STOP","FORWARD","BACKWARD","LEFT","RIGHT"};
//     Serial.println(names[newCmd]);
//   }
// }

// //////////////////////////////////////////////////
// // SETUP / LOOP
// //////////////////////////////////////////////////

// void setup() {
//   Serial.begin(115200);
//   Serial.println("TRANSMITTER STARTING");
//   setupMPU();
//   setupESPNow();
// }

// void loop() {
//   detectAndSend();
//   delay(50);
// }


#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

Adafruit_MPU6050 mpu;

uint8_t receiverMAC[] = {0x00, 0x70, 0x07, 0x24, 0xC9, 0xB4};

#define CMD_STOP     0
#define CMD_FORWARD  1
#define CMD_BACKWARD 2
#define CMD_LEFT     3
#define CMD_RIGHT    4

#define MODE_GYRO 0
#define MODE_APP  1

#define CALIB_SAMPLES 200
#define EMA_ALPHA     0.1f
#define DEADZONE      2.0f
#define TILT_THR      5.0f

#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHAR_RX_UUID        "12345678-1234-1234-1234-123456789abd"  // app -> esp
#define CHAR_TX_UUID        "12345678-1234-1234-1234-123456789abe"  // esp -> app

typedef struct {
  int command;
  int speed;
} ControlPacket;

ControlPacket packet;

float offsetX = 0.0f;
float offsetY = 0.0f;
float smoothX = 0.0f;
float smoothY = 0.0f;

int currentMode    = MODE_GYRO;
int appCommand     = CMD_STOP;
int appSpeed       = 150;
int turnSlow       = 80;
int speedLeft      = 200;
int speedRight     = 200;
int lastCommand    = -1;

bool deviceConnected = false;

BLEServer*         pServer     = nullptr;
BLECharacteristic* pTxChar     = nullptr;

unsigned long lastAppCmd   = 0;
unsigned long lastFeedback = 0;

//////////////////////////////////////////////////
// ESP-NOW
//////////////////////////////////////////////////

void onDataSent(const wifi_tx_info_t *info,
                esp_now_send_status_t status) {}

//////////////////////////////////////////////////
// BLE CALLBACKS
//////////////////////////////////////////////////

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE CONNECTED");
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    currentMode = MODE_GYRO;
    Serial.println("BLE DISCONNECTED — fallback to GYRO");
    pServer->startAdvertising();
  }
};

class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) {
    String value = pChar->getValue().c_str();
    if (value.length() == 0) return;

    StaticJsonDocument<200> doc;
    DeserializationError err = deserializeJson(doc, value);
    if (err) {
      Serial.println("JSON PARSE ERROR");
      return;
    }

    if (doc.containsKey("mode"))       currentMode  = doc["mode"];
    if (doc.containsKey("cmd"))        appCommand   = doc["cmd"];
    if (doc.containsKey("speed"))      appSpeed     = doc["speed"];
    if (doc.containsKey("turn_slow"))  turnSlow     = doc["turn_slow"];
    if (doc.containsKey("speed_l"))    speedLeft    = doc["speed_l"];
    if (doc.containsKey("speed_r"))    speedRight   = doc["speed_r"];

    lastAppCmd = millis();

    Serial.print("APP CMD: "); Serial.print(appCommand);
    Serial.print("  SPEED: "); Serial.print(appSpeed);
    Serial.print("  MODE: ");  Serial.println(currentMode);
  }
};

//////////////////////////////////////////////////
// BLE SETUP
//////////////////////////////////////////////////

void setupBLE() {
  BLEDevice::init("CarController");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  // RX — app writes commands here
  BLECharacteristic* pRxChar = pService->createCharacteristic(
    CHAR_RX_UUID,
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_WRITE_NR
  );
  pRxChar->setCallbacks(new RxCallbacks());

  // TX — esp notifies app with feedback
  pTxChar = pService->createCharacteristic(
    CHAR_TX_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pTxChar->addDescriptor(new BLE2902());

  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("BLE ADVERTISING");
}

//////////////////////////////////////////////////
// CALIBRATION
//////////////////////////////////////////////////

void calibrateMPU() {
  Serial.println("Calibrating — hold still...");
  float sumX = 0, sumY = 0;

  for (int i = 0; i < CALIB_SAMPLES; i++) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    sumX += a.acceleration.x;
    sumY += a.acceleration.y;
    delay(10);
  }

  offsetX = sumX / CALIB_SAMPLES;
  offsetY = sumY / CALIB_SAMPLES;
  smoothX = 0;
  smoothY = 0;

  Serial.print("OffsetX: "); Serial.print(offsetX);
  Serial.print("  OffsetY: "); Serial.println(offsetY);
  Serial.println("Calibration done.");
}

//////////////////////////////////////////////////
// MPU SETUP
//////////////////////////////////////////////////

void setupMPU() {
  Wire.begin(21, 22);

  if (!mpu.begin()) {
    Serial.println("MPU6050 NOT FOUND");
    while (1) delay(500);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("MPU6050 OK");
  calibrateMPU();
}

//////////////////////////////////////////////////
// ESP-NOW SETUP
//////////////////////////////////////////////////

void setupESPNow() {
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FAILED");
    while (1) delay(500);
  }

  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, receiverMAC, 6);
  peer.channel = 0;
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("FAILED TO ADD PEER");
    while (1) delay(500);
  }

  Serial.println("ESP-NOW OK");
}

//////////////////////////////////////////////////
// GYRO COMMAND DETECTION
//////////////////////////////////////////////////

int getGyroCommand() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  float rawX = a.acceleration.x - offsetX;
  float rawY = a.acceleration.y - offsetY;

  smoothX = EMA_ALPHA * rawX + (1.0f - EMA_ALPHA) * smoothX;
  smoothY = EMA_ALPHA * rawY + (1.0f - EMA_ALPHA) * smoothY;

  if (fabs(smoothX) < DEADZONE && fabs(smoothY) < DEADZONE)
    return CMD_STOP;
  else if (smoothX >  TILT_THR) return CMD_FORWARD;
  else if (smoothX < -TILT_THR) return CMD_BACKWARD;
  else if (smoothY >  TILT_THR) return CMD_LEFT;
  else if (smoothY < -TILT_THR) return CMD_RIGHT;

  return CMD_STOP;
}

//////////////////////////////////////////////////
// SEND FEEDBACK TO APP
//////////////////////////////////////////////////

void sendFeedback(int cmd) {
  if (!deviceConnected) return;

  StaticJsonDocument<200> doc;
  doc["x"]    = smoothX;
  doc["y"]    = smoothY;
  doc["cmd"]  = cmd;
  doc["mode"] = currentMode;

  char buf[200];
  serializeJson(doc, buf);
  pTxChar->setValue(buf);
  pTxChar->notify();
}

//////////////////////////////////////////////////
// SETUP
//////////////////////////////////////////////////

void setup() {
  Serial.begin(115200);
  Serial.println("TRANSMITTER STARTING");

  setupMPU();

  WiFi.mode(WIFI_STA);
  setupESPNow();
  setupBLE();
}

//////////////////////////////////////////////////
// LOOP
//////////////////////////////////////////////////

void loop() {
  int cmd = CMD_STOP;

  if (currentMode == MODE_APP) {
    cmd = appCommand;
    packet.speed = appSpeed;
  } else {
    cmd = getGyroCommand();
    packet.speed = 150;
  }

  // only send on change
  if (cmd != lastCommand) {
    packet.command = cmd;
    esp_now_send(receiverMAC, (uint8_t *)&packet, sizeof(packet));
    lastCommand = cmd;

    const char* names[] = {"STOP","FORWARD","BACKWARD","LEFT","RIGHT"};
    Serial.print(currentMode == MODE_APP ? "[APP] " : "[GYRO] ");
    Serial.println(names[cmd]);
  }

  // send feedback every 100ms
  if (millis() - lastFeedback > 100) {
    sendFeedback(cmd);
    lastFeedback = millis();
  }

  delay(50);
}