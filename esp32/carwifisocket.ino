#include <WiFi.h>
#include <WebSocketsClient.h>

// WiFi
const char* ssid = "Cgates-9F0D";
const char* password = "J7tm3-JF4Dv94j";

// WebSocket server (твой Linux ПК)
const char* ws_host = "192.168.88.5";
const uint16_t ws_port = 3030;

// моторы L298N
#define IN1 27
#define IN2 26
#define IN3 25
#define IN4 33

WebSocketsClient ws;

// ========== STATE ==========
String currentCmd = "stop";
unsigned long lastMsg = 0;

// ========== MOTOR FUNCTIONS ==========
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void back() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void left() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void right() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

// ========== COMMAND HANDLER ==========
void handleCommand(String cmd)
{
  cmd.trim();
  lastMsg = millis();

  Serial.println("CMD: " + cmd);

  // joystick format: x:0.5,y:-0.2
  if (cmd.startsWith("x:")) {

    int iy = cmd.indexOf(",y:");
    float x = cmd.substring(2, iy).toFloat();
    float y = cmd.substring(iy + 3).toFloat();

    if (abs(x) < 0.2 && abs(y) < 0.2) {
      stopMotors();
      return;
    }

    if (y > 0.3) back();
    else if (y < -0.3) forward();
    else if (x > 0.3) left();
    else if (x < -0.3) right();
    else stopMotors();

    return;
  }

  // button mode (HOLD from browser)
  if (cmd == "forward") {
    currentCmd = cmd;
    forward();
  }
  else if (cmd == "back") {
    currentCmd = cmd;
    back();
  }
  else if (cmd == "left") {
    currentCmd = cmd;
    left();
  }
  else if (cmd == "right") {
    currentCmd = cmd;
    right();
  }
  else {
    currentCmd = "stop";
    stopMotors();
  }
}

// ========== WEBSOCKET EVENTS ==========
void wsEvent(WStype_t type, uint8_t * payload, size_t length)
{
  switch(type)
  {
    case WStype_CONNECTED:
      Serial.println("WebSocket connected");
      ws.sendTXT("ESP32_READY");
      break;

    case WStype_DISCONNECTED:
      Serial.println("WebSocket disconnected");
      stopMotors();
      break;

    case WStype_TEXT:
      handleCommand((char*)payload);
      break;

    default:
      break;
  }
}

// ========== SAFETY STOP ==========
void safetyCheck()
{
  if (millis() - lastMsg > 800)
  {
    stopMotors();
    currentCmd = "stop";
  }
}

// ========== SETUP ==========
void setup()
{
  Serial.begin(115200);

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  stopMotors();

  WiFi.begin(ssid, password);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  ws.begin(ws_host, ws_port, "/");
  ws.onEvent(wsEvent);
  ws.setReconnectInterval(2000);
}

// ========== LOOP ==========
void loop()
{
  ws.loop();
  safetyCheck();
}