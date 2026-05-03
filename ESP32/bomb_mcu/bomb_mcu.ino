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
    tone(buzzerPin, 2000, 50); // Tiny click for each button pressed
    
    currentGuess += message; // Add the typed letter to the guess
    
    // Draw an asterisk so the player knows the button press was received!
    if (isEncryptedMode) {
      // In Encrypted mode, the bottom row is full! Draw asterisks in the top right
      lcd.setCursor(11 + currentGuess.length(), 0); 
    } else {
      // In Basic mode, draw them normally on the bottom row
      lcd.setCursor(currentGuess.length() - 1, 1);
    }
    lcd.print("*");

    // Check if they have typed enough characters
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
  
  // Randomizer seed based on empty analog pin
  randomSeed(analogRead(0));

  delay(1000);
  lcd.begin(16, 2);
  updateIdleScreen();

  // Network Setup
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

  // Only check buttons if game is NOT running
  if (!isGameRunning) {
    if (digitalRead(modeBtnPin) == LOW) {
      isEncryptedMode = !isEncryptedMode;
      tone(buzzerPin, 2000, 30);
      updateIdleScreen();
      delay(300); // Debounce
    }

    if (digitalRead(triggerBtnPin) == LOW) {
      if (isEncryptedMode) {
        runEncryptedSequence();
      } else {
        runBasicSequence();
      }
    }
  }
}

// ==========================================
// 7. GAME SEQUENCE FUNCTIONS
// ==========================================

void updateIdleScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("System Ready");
  lcd.setCursor(0, 1);
  if (isEncryptedMode) lcd.print("> Encrypted Mode");
  else lcd.print("> Basic Mode");
}

void runBasicSequence() {
  int code = random(100000, 1000000);
  currentPassword = String(code);
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("MEMORIZE CODE:");
  lcd.setCursor(0, 1);
  lcd.print(currentPassword);
  
  tone(buzzerPin, 800, 100); delay(150);
  tone(buzzerPin, 1200, 150); delay(3000); 
  
  startGame("15");
}

void runEncryptedSequence() {
  const char padChars[] = "1234567890ABCD*#";
  char letters[4];
  char keys[4];
  currentPassword = ""; 

  // 1. Generate 4 unique random letters (Using E-Z to prevent keypad confusion)
  for(int i = 0; i < 4; i++) {
    bool isUnique;
    do {
      isUnique = true;
      letters[i] = (char)random(69, 91); // ASCII for 'E' to 'Z'
      for(int j = 0; j < i; j++) {
        if(letters[i] == letters[j]) isUnique = false;
      }
    } while(!isUnique);
  }

  // 2. Pick 4 random characters from the keypad that the user actually has to type
  for(int i = 0; i < 4; i++) {
    keys[i] = padChars[random(0, 16)];
    currentPassword += keys[i]; 
  }

  // 3. Create a shuffled order for displaying the mapping
  int indices[] = {0, 1, 2, 3};
  for (int i = 3; i > 0; i--) {
    int j = random(0, i + 1);
    int temp = indices[i];
    indices[i] = indices[j];
    indices[j] = temp;
  }

  // 4. Build Row 1 (The Word) e.g., "PWD: WXYZ"
  String row1 = "PWD: ";
  for(int i = 0; i < 4; i++) {
    row1 += letters[i];
  }

  // 5. Build Row 2 (The scrambled mapping) e.g., "X=3 Y=A W=* Z=9"
  String row2 = "";
  for(int i = 0; i < 4; i++) {
    int idx = indices[i];
    row2 += letters[idx];
    row2 += "=";
    row2 += keys[idx];
    if(i < 3) row2 += " "; // Add space between pairs, but not at the end
  }

  // Instantly show the puzzle (No ARMING screen!)
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(row1);
  lcd.setCursor(0, 1);
  lcd.print(row2);

  tone(buzzerPin, 1000, 200); // Quick start beep
  startGame("20");            // Instantly start 90-second timer!
}

void startGame(String seconds) {
  isGameRunning = true;
  currentGuess = "";
  
  if (!isEncryptedMode) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ENTER PASSCODE:");
  }
  
  // NEW: We pack the Mode and the Password into the MQTT message!
  // Example: "START:15:BASIC:123456"
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