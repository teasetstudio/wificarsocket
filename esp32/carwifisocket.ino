#include <WiFi.h>
#include <WebSocketsClient.h>

// моторы L298N
#define IN1 27
#define IN2 26
#define IN3 25
#define IN4 33
// ENA and ENB
#define ENA 14
#define ENB 32

// Front center sensor
#define TRIG_CENTER 13
#define ECHO_CENTER 34

// Front left sensor
#define TRIG_LEFT 12
#define ECHO_LEFT 35

// Front right sensor
#define TRIG_RIGHT 5
#define ECHO_RIGHT 17

volatile long centerDistance = 999;
volatile long leftDistance = 999;
volatile long rightDistance = 999;

const int STOP_DISTANCE = 20;

// Speed
const int FULL_SPEED = 255;
const int TURN_SPEED = 200;

// WiFi
const char* ssid = "Cgates-9F0D";
const char* password = "J7tm3-JF4Dv94j";

// WebSocket server (твой Linux ПК)
const char* ws_host = "fantasybypro.com";
const uint16_t ws_port = 443;

WebSocketsClient ws;

// ========== STATE ==========
String currentCmd = "stop";
unsigned long lastMsg = 0;

// Gesture
bool gestureStarted = false;

long previousLeft = 999;
long previousCenter = 999;
long previousRight = 999;

unsigned long gestureStartTime = 0;
int gestureStep = 0;

unsigned long ignoreObstaclesUntil = 0;

// Gesture 2
bool gestureStartedBack = false;

long previousLeftBack = 999;
long previousCenterBack = 999;
long previousRightBack = 999;

unsigned long gestureStartTimeBack = 0;
int gestureStepBack = 0;

// ========== MOTOR FUNCTIONS ==========
void stopMotors() {
  ledcWrite(ENA, 0);
  ledcWrite(ENB, 0);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void resetGestures() {
  gestureStarted = false;
  previousLeft = 999;
  previousCenter = 999;
  previousRight = 999;
  gestureStartTime = 0;
  gestureStep = 0;

  gestureStartedBack = false;
  gestureStartTimeBack = 0;
  gestureStepBack = 0;
}

unsigned long lastForwardPrint = 0;

void forward() {
  if (millis() > ignoreObstaclesUntil)
  {
    if (
      (centerDistance < STOP_DISTANCE && centerDistance > 0) ||
      (leftDistance < STOP_DISTANCE && leftDistance > 0) ||
      (rightDistance < STOP_DISTANCE && rightDistance > 0)
    )
    {
      Serial.println("Obstacle!");
      stopMotors();
      resetGestures();
      return;
    }
  }

  if (millis() - lastForwardPrint > 500)
  {
    Serial.println("Forward");
    lastForwardPrint = millis();
  }
  

  ledcWrite(ENA, FULL_SPEED);
  ledcWrite(ENB, FULL_SPEED);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

unsigned long lastBackPrint = 0;

void back() {
  if (millis() > ignoreObstaclesUntil)
  {
    if (
      (centerDistance < STOP_DISTANCE && centerDistance > 0) ||
      (leftDistance < STOP_DISTANCE && leftDistance > 0) ||
      (rightDistance < STOP_DISTANCE && rightDistance > 0)
    )
    {
      Serial.println("Obstacle!");
      stopMotors();
      resetGestures();
      return;
    }
  }

  if (millis() - lastBackPrint > 500)
  {
    Serial.println("Back");
    lastBackPrint = millis();
  }

  ledcWrite(ENA, FULL_SPEED);
  ledcWrite(ENB, FULL_SPEED);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void left() {
  Serial.println("Left");

  ledcWrite(ENA, TURN_SPEED);
  ledcWrite(ENB, TURN_SPEED);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void right() {
  Serial.println("Right");

  ledcWrite(ENA, TURN_SPEED);
  ledcWrite(ENB, TURN_SPEED);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void forwardLeft() {
  if (
    (centerDistance < STOP_DISTANCE && centerDistance > 0) ||
    (leftDistance < STOP_DISTANCE && leftDistance > 0) ||
    (rightDistance < STOP_DISTANCE && rightDistance > 0)
  )
  {
    Serial.println("Obstacle!");
    stopMotors();
    return;
  }

  Serial.println("Forward Left");

  ledcWrite(ENA, FULL_SPEED);
  ledcWrite(ENB, TURN_SPEED);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void forwardRight() {
  if (
    (centerDistance < STOP_DISTANCE && centerDistance > 0) ||
    (leftDistance < STOP_DISTANCE && leftDistance > 0) ||
    (rightDistance < STOP_DISTANCE && rightDistance > 0)
  )
  {
    Serial.println("Obstacle!");
    stopMotors();
    return;
  }

  Serial.println("Forward Right");

  ledcWrite(ENA, TURN_SPEED);
  ledcWrite(ENB, FULL_SPEED);

  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backLeft() {
  Serial.println("Back Left");

  // Left motor slower, right motor full speed
  ledcWrite(ENA, TURN_SPEED);
  ledcWrite(ENB, FULL_SPEED);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void backRight() {
  Serial.println("Back Right");

  // Right motor slower, left motor full speed
  ledcWrite(ENA, FULL_SPEED);
  ledcWrite(ENB, TURN_SPEED);

  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
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

    resetGestures();

    // Diagonal movement
    if (y < -0.3 && x < -0.3) forwardLeft();
    else if (y < -0.3 && x > 0.3) forwardRight();
    else if (y > 0.3 && x < -0.3) backLeft();
    else if (y > 0.3 && x > 0.3) backRight();

    // Straight movement
    else if (y < -0.3) forward();
    else if (y > 0.3) back();
    else if (x < -0.3) left();
    else if (x > 0.3) right();
    else stopMotors();

    return;
  }
}

long getDistance(int trigPin, int echoPin)
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);


  long duration = pulseIn(
    echoPin,
    HIGH,
    30000
  );


  if (duration == 0)
    return -1;


  return duration * 0.034 / 2;
}

void sensorTask(void * parameter)
{
  while(true)
  {

    // читаем центральный
    long d1 = getDistance(
      TRIG_CENTER,
      ECHO_CENTER
    );

    if (d1 > 0)
      centerDistance = d1;

    // маленькая пауза,
    // чтобы датчики не слышали друг друга
    vTaskDelay(40 / portTICK_PERIOD_MS);

    // читаем левый
    long d2 = getDistance(
      TRIG_LEFT,
      ECHO_LEFT
    );

    if (d2 > 0)
      leftDistance = d2;

    // RIGHT
    long d3 = getDistance(TRIG_RIGHT, ECHO_RIGHT);

    if (d3 > 0)
      rightDistance = d3;


    vTaskDelay(40 / portTICK_PERIOD_MS);

    // Serial.print("Center: ");
    // Serial.print(centerDistance);
    // Serial.print(" Left: ");
    // Serial.println(leftDistance);
    // Serial.print(" Right: ");
    // Serial.println(rightDistance);

    vTaskDelay(60 / portTICK_PERIOD_MS);
  }
}

bool detectWaveGesture()
{
  unsigned long now = millis();
  // Step 1:
  // Hand appears on the left side

  if (gestureStep == 0)
  {
    if (leftDistance < 25 &&
        centerDistance > 30 &&
        rightDistance > 30)
    {
      Serial.println("Gesture: left detected");

      gestureStep = 1;
      gestureStartTime = now;
    }
  }


  // Step 2:
  // Hand moves to center
  else if (gestureStep == 1)
  {
    if (centerDistance < 25 && rightDistance > 30)
    {
      Serial.println("Gesture: center detected");

      gestureStep = 2;
      gestureStartTime = now;
    }
  }


  // Step 3:
  // Hand moves to right

  else if (gestureStep == 2)
  {
    if (rightDistance < 25 && leftDistance > 30)
    {
      Serial.println("Gesture: right detected");

      gestureStep = 3;
      gestureStartTime = now;
    }
  }

  else if (gestureStep == 3)
  {
    if (leftDistance > 30 &&
        centerDistance > 30 &&
        rightDistance > 30)
    {
      Serial.println("Gesture: 2 phase detected");

      gestureStep = 4;
      gestureStartTime = now;
    }
  }

  if (gestureStep == 4)
  {
    if (rightDistance < 25 && centerDistance > 30 && leftDistance > 30)
    {
      Serial.println("Gesture: right detected in 2 phase");

      gestureStep = 5;
      gestureStartTime = now;
    }
  }

  if (gestureStep == 5)
  {
    if (centerDistance < 25 && leftDistance > 30)
    {
      Serial.println("Gesture: center detected in 2 phase");

      gestureStep = 6;
      gestureStartTime = now;
    }
  }

  if (gestureStep == 6)
  {
    if (leftDistance < 25 && rightDistance > 30)
    {
      Serial.println("Gesture: wave complete");

      gestureStep = 0;
      return true;
    }
  }


  // timeout
  if (now - gestureStartTime > 1500 && gestureStep != 0)
  {
    Serial.println("Gesture: timeout");
    gestureStep = 0;
  }


  return false;
}

bool detectWaveGestureBack()
{
  unsigned long now = millis();
  // Step 1:
  // Hand appears on the left side

  if (gestureStepBack == 0)
  {
    if (rightDistance < 25 &&
        centerDistance > 30 &&
        leftDistance > 30)
    {
      Serial.println("Gesture: Right detected");

      gestureStepBack = 1;
      gestureStartTimeBack = now;
    }
  }


  // Step 2:
  // Hand moves to center
  else if (gestureStepBack == 1)
  {
    if (centerDistance < 25 && leftDistance > 30)
    {
      Serial.println("Gesture: center detected");

      gestureStepBack = 2;
      gestureStartTimeBack = now;
    }
  }


  // Step 3:
  // Hand moves to right

  else if (gestureStepBack == 2)
  {
    if (leftDistance < 25 && rightDistance > 30)
    {
      Serial.println("Gesture: left detected");

      gestureStepBack = 3;
      gestureStartTimeBack = now;
    }
  }

  else if (gestureStepBack == 3)
  {
    if (leftDistance > 30 &&
        centerDistance > 30 &&
        rightDistance > 30)
    {
      Serial.println("Gesture BAck: 2 phase detected");

      gestureStepBack = 4;
      gestureStartTimeBack = now;
    }
  }

  if (gestureStepBack == 4)
  {
    if (leftDistance < 25 && centerDistance > 30 && rightDistance > 30)
    {
      Serial.println("Gesture: right detected in 2 phase");

      gestureStepBack = 5;
      gestureStartTimeBack = now;
    }
  }

  if (gestureStepBack == 5)
  {
    if (centerDistance < 25 && rightDistance > 30)
    {
      Serial.println("Gesture: center detected in 2 phase");

      gestureStepBack = 6;
      gestureStartTimeBack = now;
    }
  }

  if (gestureStepBack == 6)
  {
    if (rightDistance < 25 && leftDistance > 30)
    {
      Serial.println("Gesture: wave complete");

      gestureStepBack = 0;
      return true;
    }
  }


  // timeout
  if (now - gestureStartTimeBack > 1500 && gestureStepBack != 0)
  {
    Serial.println("Gesture: timeout");
    gestureStepBack = 0;
  }


  return false;
}

// ========== WEBSOCKET EVENTS ==========
void wsEvent(WStype_t type, uint8_t * payload, size_t length)
{
  switch(type)
  {
    case WStype_CONNECTED:
      Serial.println("WebSocket connected");

      ws.sendTXT(R"({
        "type":"controller"
      })");

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

  // Sensors
  pinMode(TRIG_CENTER, OUTPUT);
  pinMode(ECHO_CENTER, INPUT);

  pinMode(TRIG_LEFT, OUTPUT);
  pinMode(ECHO_LEFT, INPUT);

  pinMode(TRIG_RIGHT, OUTPUT);
  pinMode(ECHO_RIGHT, INPUT);


  xTaskCreatePinnedToCore(
    sensorTask,
    "Sensor Task",
    4096,
    NULL,
    1,
    NULL,
    1   // Core 1
  );

  // PWM
  ledcAttach(ENA, 20000, 8);   // 20 кГц, 8 бит
  ledcAttach(ENB, 20000, 8);

  ledcWrite(ENA, FULL_SPEED);
  ledcWrite(ENB, FULL_SPEED);

  WiFi.begin(ssid, password);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());

  ws.beginSSL("fantasybypro.com", 443, "/car");
  ws.onEvent(wsEvent);
  ws.setReconnectInterval(2000);
}

// ========== LOOP ==========
unsigned long lastSensorRead = 0;
const unsigned long sensorInterval = 100; // 100 ms

void loop()
{
  ws.loop();
  safetyCheck();

  if (!gestureStarted)
  {
    if (detectWaveGesture())
    {
      ignoreObstaclesUntil = millis() + 1000; // Ignore sensors for 1 second
      gestureStarted = true;

      lastMsg = millis();   // prevent safety stop

      Serial.println("START DRIVING");
    }
  } else {
    lastMsg = millis();
    forward();
  }

  if (!gestureStartedBack)
  {
    if (detectWaveGestureBack())
    {
      ignoreObstaclesUntil = millis() + 1000; // Ignore sensors for 1 second
      gestureStartedBack = true;

      lastMsg = millis();   // prevent safety stop

      Serial.println("START DRIVING Back");
    }
  } else {
    lastMsg = millis();
    back();
  }
}
