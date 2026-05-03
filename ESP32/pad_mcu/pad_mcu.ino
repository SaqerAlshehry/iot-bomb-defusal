#include <WiFi.h>
#include <PubSubClient.h>
#include <SevSeg.h>
#include <Keypad.h>

// ==========================================
// 1. NETWORK CREDENTIALS
// ==========================================
const char* ssid = "Alshehri";
const char* password = "Ha112233@";
const char* mqtt_server = "192.168.8.138"; // Your laptop's IP

WiFiClient espClient;
PubSubClient client(espClient);

// ==========================================
// 2. HARDWARE: 7-SEGMENT TIMER
// ==========================================
SevSeg sevseg;
unsigned long previousMillis = 0;
int remainingTimeDeciseconds = 0; 
bool isCountingDown = false;

// ==========================================
// 3. HARDWARE: DEFUSAL PAD
// ==========================================
const byte ROWS = 4; 
const byte COLS = 4; 
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

// Your specific mapped & flipped pins!
byte rowPins[ROWS] = {47, 48, 45, 35}; 
byte colPins[COLS] = {36, 37, 38, 39}; 

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ==========================================
// 4. MQTT INBOX (LISTENING TO THE BRAIN)
// ==========================================
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  if (String(topic) == "game/bomb/status") {
    if (message.startsWith("START:")) {
      int seconds = message.substring(6).toInt();
      remainingTimeDeciseconds = seconds * 10; 
      isCountingDown = true;
      Serial.println("INCOMING COMM: Timer Started!");
    } 
    else if (message == "DEFUSED") {
      isCountingDown = false; // Freeze the timer on the winning time
      Serial.println("INCOMING COMM: Bomb Defused!");
    }
    else if (message == "BOOM") {
      isCountingDown = false; // Freeze the timer
      remainingTimeDeciseconds = 0; // Force to zero
      Serial.println("INCOMING COMM: YOU DIED.");
    }
  }
}

// ==========================================
// 5. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);

  // --- TIMER SETUP (Updated with your working hardware spec!) ---
  byte numDigits = 4;
  byte digitPins[] = {11, 10, 9, 3}; 
  byte segmentPins[] = {4, 5, 6, 7, 8, 16, 17, 18}; 
  
  bool resistorsOnSegments = true;
  byte hardwareConfig = COMMON_CATHODE; 
  bool updateWithDelays = false; 
  bool leadingZeros = false; 
  bool disableDecPoint = false; 
  
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments,
               updateWithDelays, leadingZeros, disableDecPoint);
  
  sevseg.setBrightness(90);

  // --- WIFI SETUP ---
  Serial.print("Connecting to Wi-Fi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected!");

  // --- MQTT SETUP ---
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// ==========================================
// 6. MAIN LOOP
// ==========================================
void loop() {
  // 1. Maintain MQTT Connection
  if (!client.connected()) {
    String clientId = "BombConsole-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe("game/bomb/status"); 
    }
  }
  client.loop(); 

  // 2. Read the Keypad & Publish immediately
  char key = keypad.getKey();
  if (key) {
    String payload = String(key); 
    client.publish("game/pad/input", payload.c_str()); 
    Serial.print("Sending to Brain: ");
    Serial.println(payload);
  }

  // 3. Handle the Countdown
  if (isCountingDown) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 100) {
      previousMillis = currentMillis;
      remainingTimeDeciseconds--;

      if (remainingTimeDeciseconds <= 0) {
        remainingTimeDeciseconds = 0;
        isCountingDown = false;
        // Tell the friend's board we ran out of time!
        client.publish("game/bomb/status", "BOOM"); 
      }
    }
  }

  // 4. Draw the Display
  if (remainingTimeDeciseconds > 0) {
    sevseg.setNumber(remainingTimeDeciseconds, 1);
  } else {
    sevseg.setNumber(0, 0); 
  }
  
  // Keep the LEDs alive!
  sevseg.refreshDisplay(); 
}