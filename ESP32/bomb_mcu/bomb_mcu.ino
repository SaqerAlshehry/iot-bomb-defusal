#include <WiFi.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>

// ==========================================
// 1. NETWORK CREDENTIALS
// ==========================================
const char* ssid = "Alshehri";
const char* password = "Ha112233@";
const char* mqtt_server = "192.168.8.138"; // Your laptop's IP

WiFiClient espClient;
PubSubClient client(espClient);

// ==========================================
// 2. HARDWARE PINS
// ==========================================
LiquidCrystal lcd(4, 5, 6, 7, 15, 16);
const int modeBtnPin = 1;      
const int triggerBtnPin = 13;   
const int buzzerPin = 14;       

// ==========================================
// 3. GAME LOGIC STATES
// ==========================================
bool isEncryptedMode = false; // False = Basic, True = Encrypted
bool isGameRunning = false;
String currentPassword = "";
String currentGuess = "";

// --- NEW: Time Tracking Variables for the Panic Beeper ---
unsigned long gameStartTime = 0;
int gameDurationSeconds = 0;
unsigned long lastBeepTime = 0;

// ==========================================
// 4. MQTT INBOX (Listening to the Console)
// ==========================================
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // A. IF THE CONSOLE TIMER RAN OUT
  if (String(topic) == "game/bomb/status" && message == "BOOM" && isGameRunning) {
    loseGame();
  }

  // B. IF THE CONSOLE SENT A KEYPAD PRESS
  if (String(topic) == "game/pad/input" && isGameRunning) {
    // We removed the click sound here so it doesn't interrupt the panic beeper
    
    currentGuess += message; 
    
    if (isEncryptedMode) {
      lcd.setCursor(11 + currentGuess.length(), 0); 
    } else {
      lcd.setCursor(currentGuess.length() - 1, 1);
    }
    lcd.print("*");

    if (currentGuess.length() == currentPassword.length()) {
      if (currentGuess == currentPassword) {
        winGame();
      } else {
        loseGame();
      }
    }
  }
}

// ==========================================
// 5. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);

  pinMode(modeBtnPin, INPUT_PULLUP);
  pinMode(triggerBtnPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
  
  randomSeed(analogRead(0));

  delay(1000);
  lcd.begin(16, 2);
  updateIdleScreen();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// ==========================================
// 6. MAIN LOOP
// ==========================================
void loop() {
  if (!client.connected()) {
    String clientId = "BombBrain-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      client.subscribe("game/pad/input");   
      client.subscribe("game/bomb/status"); 
    }
  }
  client.loop(); 

  if (!isGameRunning) {
    if (digitalRead(modeBtnPin) == LOW) {
      isEncryptedMode = !isEncryptedMode;
      tone(buzzerPin, 2000, 30);
      updateIdleScreen();
      delay(300); 
    }

    if (digitalRead(triggerBtnPin) == LOW) {
      if (isEncryptedMode) {
        runEncryptedSequence();
      } else {
        runBasicSequence();
      }
    }
  } else {
    // --- NEW: If the game is running, handle the ticking sound! ---
    handleBeeper();
  }
}

// ==========================================
// 7. GAME SEQUENCE FUNCTIONS
// ==========================================

// --- NEW: The Panic Beeper Logic ---
void handleBeeper() {
  unsigned long currentMillis = millis();
  int elapsedSeconds = (currentMillis - gameStartTime) / 1000;
  int timeLeft = gameDurationSeconds - elapsedSeconds;

  int beepInterval;

  // Determine how fast it should beep based on time left
  if (timeLeft <= 5) {
    beepInterval = 150;  // Panic mode (Super fast!)
  } else if (timeLeft <= 10) {
    beepInterval = 500;  // Hurry mode (Medium fast)
  } else {
    beepInterval = 1000; // Normal mode (Once a second)
  }

  // If enough time has passed, trigger the beep
  if (currentMillis - lastBeepTime >= beepInterval) {
    lastBeepTime = currentMillis;
    tone(buzzerPin, 1200, 50); // A short, sharp 50ms beep!
  }
}

void updateIdleScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  lcd.setCursor(0, 1);
  if (isEncryptedMode) lcd.print("> Encrypted Mode");
  else lcd.print("> Basic Mode");
}

void runBasicSequence() {
  long code = random(100000, 1000000); // 6 digits!
  currentPassword = String(code);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MEMORIZE CODE:");
  lcd.setCursor(0, 1);
  lcd.print(currentPassword);
  
  tone(buzzerPin, 800, 100); delay(150);
  tone(buzzerPin, 1200, 150); delay(3000); 
  
  startGame("10"); 
}

void runEncryptedSequence() {
  const char padChars[] = "1234567890ABCD*#";
  char letters[4];
  char keys[4];
  currentPassword = ""; 

  for(int i = 0; i < 4; i++) {
    bool isUnique;
    do {
      isUnique = true;
      letters[i] = (char)random(69, 91); 
      for(int j = 0; j < i; j++) {
        if(letters[i] == letters[j]) isUnique = false;
      }
    } while(!isUnique);
  }

  for(int i = 0; i < 4; i++) {
    keys[i] = padChars[random(0, 16)];
    currentPassword += keys[i]; 
  }

  int indices[] = {0, 1, 2, 3};
  for (int i = 3; i > 0; i--) {
    int j = random(0, i + 1);
    int temp = indices[i];
    indices[i] = indices[j];
    indices[j] = temp;
  }

  String row1 = "PWD: ";
  for(int i = 0; i < 4; i++) {
    row1 += letters[i];
  }

  String row2 = "";
  for(int i = 0; i < 4; i++) {
    int idx = indices[i];
    row2 += letters[idx];
    row2 += "=";
    row2 += keys[idx];
    if(i < 3) row2 += " "; 
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(row1);
  lcd.setCursor(0, 1);
  lcd.print(row2);

  tone(buzzerPin, 1000, 200); 
  startGame("15");            
}

void startGame(String seconds) {
  isGameRunning = true;
  currentGuess = "";
  
  // --- NEW: Save the exact time the game started for the beeper ---
  gameStartTime = millis();
  gameDurationSeconds = seconds.toInt();
  lastBeepTime = millis();
  
  if (!isEncryptedMode) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ENTER PASSCODE:");
  }
  
  String modeName = isEncryptedMode ? "ENCRYPTED" : "BASIC";
  String mqttPayload = "START:" + seconds + ":" + modeName + ":" + currentPassword;
  
  client.publish("game/bomb/status", mqttPayload.c_str());
}

void winGame() {
  isGameRunning = false;
  client.publish("game/bomb/status", "DEFUSED"); 
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SYSTEM SECURE");
  lcd.setCursor(0, 1);
  lcd.print("BOMB DEFUSED");
  
  tone(buzzerPin, 1000, 100); delay(100);
  tone(buzzerPin, 1500, 100); delay(100);
  tone(buzzerPin, 2000, 300);
  
  delay(5000);
  updateIdleScreen();
}

void loseGame() {
  isGameRunning = false;
  client.publish("game/bomb/status", "BOOM"); 
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CRITICAL FAILURE");
  lcd.setCursor(0, 1);
  lcd.print(">>> BOOM <<<");
  
  tone(buzzerPin, 150, 1500);
  
  delay(5000);
  updateIdleScreen();
}