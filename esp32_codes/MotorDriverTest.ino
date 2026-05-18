// #define ENB 23
// #define IN3 18
// #define IN4 19

// void setup() {
//   Serial.begin(115200);
//   pinMode(ENB, OUTPUT);
//   pinMode(IN3, OUTPUT);
//   pinMode(IN4, OUTPUT);

//   Serial.println("RIGHT FORWARD");
//   digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
//   analogWrite(ENB, 200);
//   delay(2000);
//   analogWrite(ENB, 0);
//   delay(1000);

//   Serial.println("RIGHT BACKWARD");
//   digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);
//   analogWrite(ENB, 200);
//   delay(2000);
//   analogWrite(ENB, 0);
// }

// void loop() {}


#include <WiFi.h>
#include <esp_now.h>

#define ENA 22
#define IN1 16
#define IN2 17
#define ENB 23
#define IN3 18
#define IN4 19

#define CMD_STOP     0
#define CMD_FORWARD  1
#define CMD_BACKWARD 2
#define CMD_LEFT     3
#define CMD_RIGHT    4

// TUNE IF DRIFTS
#define SPEED_LEFT  200
#define SPEED_RIGHT 200

// TUNE FOR TURN SHARPNESS
// 0 = full pivot, 80 = gentle arc, 120 = wide arc
#define TURN_FAST   200
#define TURN_SLOW   80

typedef struct {
  int command;
  int speed;
} ControlPacket;

ControlPacket packet;

void stopMotors() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void moveForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, packet.speed);
  analogWrite(ENB, packet.speed);
}

void moveBackward() {
  digitalWrite(IN1, LOW);  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);
  analogWrite(ENA, packet.speed);
  analogWrite(ENB, packet.speed);
}

void turnLeft() {
  // both wheels forward but left slows = arcs left
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, TURN_SLOW);
  analogWrite(ENB, TURN_FAST);
}

void turnRight() {
  // both wheels forward but right slows = arcs right
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);  digitalWrite(IN4, HIGH);
  analogWrite(ENA, TURN_FAST);
  analogWrite(ENB, TURN_SLOW);
}

void executeCommand() {
  switch (packet.command) {
    case CMD_FORWARD:
      moveForward();
      Serial.println("FORWARD");
      break;
    case CMD_BACKWARD:
      moveBackward();
      Serial.println("BACKWARD");
      break;
    case CMD_LEFT:
      turnLeft();
      Serial.println("LEFT");
      break;
    case CMD_RIGHT:
      turnRight();
      Serial.println("RIGHT");
      break;
    default:
      stopMotors();
      Serial.println("STOP");
      break;
  }
}

void onDataRecv(const esp_now_recv_info_t *info,
                const uint8_t *incomingData,
                int len) {
  memcpy(&packet, incomingData, sizeof(packet));
  executeCommand();
}

void setup() {
  Serial.begin(115200);

  pinMode(ENA, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopMotors();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW INIT FAILED");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  Serial.println("RECEIVER READY");
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
}

void loop() {}