// === ACTIVER / DÉSACTIVER LES COMPOSANTS ===
#define LCD
#define RIFD
#define KEYPAD
#define SERVO

#include <Wire.h>
#include <Arduino.h>

#if defined(LCD)
  #include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd(0x27, 22, 16);
#endif

#if defined(SERVO)
  #include <ESP32Servo.h>
  Servo servo5;
  int servo5Pin = 17;
  int minUs = 500;
  int maxUs = 2400;

  void ouvrir() {
    servo5.write(0);
    delay(2500);
  }

  void fermer() {
    servo5.write(90);
    delay(2500);
  }
#endif

#if defined(KEYPAD)
  #define ROWS 4
  #define COLS 4
  char keys[ROWS][COLS] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
  };
  int colPins[COLS] = {14, 27, 26, 25};
  int rowPins[ROWS] = {33, 32, 21, 13};
  int currentrow = 0;
  volatile int lastPressedCol = -1;
  volatile unsigned long lastInterruptTime = 0;
  const unsigned long debounceDelay = 100;
  String inputCode = "";
  String correctCode = "1234";
  bool codeValide = false;

  void IRAM_ATTR debounce(int col) {
    unsigned long currentTime = millis();
    if (currentTime - lastInterruptTime > debounceDelay) {
      lastPressedCol = col;
      lastInterruptTime = currentTime;
    }
  }
  void IRAM_ATTR handleInterruptCol0() { debounce(0); }
  void IRAM_ATTR handleInterruptCol1() { debounce(1); }
  void IRAM_ATTR handleInterruptCol2() { debounce(2); }
  void IRAM_ATTR handleInterruptCol3() { debounce(3); }

  void init_keypad() {
    for (int row = 0; row < ROWS; row++) {
      pinMode(rowPins[row], OUTPUT);
      digitalWrite(rowPins[row], LOW);
    }
    for (int col = 0; col < COLS; col++) {
      pinMode(colPins[col], INPUT_PULLDOWN);
    }
    attachInterrupt(digitalPinToInterrupt(colPins[0]), handleInterruptCol0, RISING);
    attachInterrupt(digitalPinToInterrupt(colPins[1]), handleInterruptCol1, RISING);
    attachInterrupt(digitalPinToInterrupt(colPins[2]), handleInterruptCol2, RISING);
    attachInterrupt(digitalPinToInterrupt(colPins[3]), handleInterruptCol3, RISING);
  }
#endif

#if defined(RIFD)
  #include <MFRC522v2.h>
  #include <MFRC522DriverSPI.h>
  #include <MFRC522DriverPinSimple.h>
  #include <MFRC522Debug.h>

  MFRC522DriverPinSimple ss_pin(5);
  MFRC522DriverSPI driver{ss_pin};
  MFRC522 mfrc522{driver};
  MFRC522::MIFARE_Key key;
  byte blockAddress = 2;
  byte bufferblocksize = 18;
  byte blockDataRead[18];
  bool rfidValide = false;
#endif

// === FONCTIONS COMMUNES ===
void showLCDMessage(String line1, String line2) {
  #if defined(LCD)
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(line1);
    lcd.setCursor(0, 1);
    lcd.print(line2);
  #else
    Serial.println(line1 + " | " + line2);
  #endif
}

void resetAll() {
  #if defined(KEYPAD)
    inputCode = "";
    codeValide = false;
  #endif
  #if defined(RIFD)
    rfidValide = false;
  #endif
  showLCDMessage("Entrez le code", "");
}

// === SETUP ===
void setup() {
  Serial.begin(115200);
  Wire.begin(16, 22);

  #if defined(KEYPAD)
    init_keypad();
  #endif

  #if defined(LCD)
    lcd.init();
    lcd.backlight();
  #endif
  showLCDMessage("Entrez le code", "");

  #if defined(SERVO)
    ESP32PWM::allocateTimer(0);
    servo5.setPeriodHertz(50);
    servo5.attach(servo5Pin, minUs, maxUs);
    fermer(); // Fermer au démarrage
  #endif

  #if defined(RIFD)
    mfrc522.PCD_Init();
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
  #endif
}

// === LOOP ===
void loop() {

  #if defined(KEYPAD)
    digitalWrite(rowPins[currentrow], LOW);
    if (lastPressedCol != -1) {
      char pressedKey = keys[currentrow][lastPressedCol];
      lastPressedCol = -1;
      Serial.printf("Touche appuyée : %c\n", pressedKey);

      if (pressedKey == '#') {
        if (inputCode == correctCode) {
          codeValide = true;
          showLCDMessage("Code bon !", "Passez carte");
          delay(2000);
          showLCDMessage("Passez la carte", "");
        } else {
          showLCDMessage("Code incorrect", "Réessayez");
          delay(2000);
          resetAll();
        }
      } else if (pressedKey == '*') {
        inputCode = "";
        showLCDMessage("Code effacé", "");
        delay(1000);
        showLCDMessage("Entrez le code", "");
      } else {
        inputCode += pressedKey;
        lcd.setCursor(0, 1);
        lcd.print(inputCode);
      }
    }
    currentrow++;
    if (currentrow >= ROWS) currentrow = 0;
    digitalWrite(rowPins[currentrow], HIGH);
  #endif

  #if defined(RIFD) && defined(KEYPAD)
    if (codeValide && !rfidValide) {
      if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return;

      Serial.print("Card UID: ");
      MFRC522Debug::PrintUID(Serial, (mfrc522.uid));
      Serial.println();

      if (mfrc522.PCD_Authenticate(0x60, blockAddress, &key, &(mfrc522.uid)) != 0) {
        showLCDMessage("Erreur badge", "Réessayez");
        delay(2000);
        resetAll();
        return;
      }

      if (mfrc522.MIFARE_Read(blockAddress, blockDataRead, &bufferblocksize) != 0) {
        showLCDMessage("Lecture echouée", "");
        delay(2000);
        resetAll();
      } else {
        String data = "";
        for (byte i = 0; i < 16; i++) {
          char c = (char)blockDataRead[i];
          data += c;
        }

        if (data.indexOf("Rui") != -1) {
          rfidValide = true;
          showLCDMessage("Carte validée", "Ouverture...");
          Serial.println("✅ Carte RFID valide");

          #if defined(SERVO)
            ouvrir();
          #endif
          showLCDMessage("Appuyez #", "pour fermer");

          // Attente fermeture
          while (true) {
            digitalWrite(rowPins[currentrow], LOW);
            if (lastPressedCol != -1) {
              char pressedKey = keys[currentrow][lastPressedCol];
              lastPressedCol = -1;
              Serial.printf("Touche appuyée : %c\n", pressedKey);
              if (pressedKey == '#') {
                #if defined(SERVO)
                  fermer();
                #endif
                showLCDMessage("Fermeture", "Terminée");
              } else {
                showLCDMessage("Fermeture", "Annulée");
              }
              delay(2000);
              resetAll();
              break;
            }
            currentrow++;
            if (currentrow >= ROWS) currentrow = 0;
            digitalWrite(rowPins[currentrow], HIGH);
          }

        } else {
          showLCDMessage("Badge refusé", "Réessayez");
          Serial.println("❌ Carte invalide");
          delay(2000);
          resetAll();
        }
      }

      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
    }
  #endif
}
