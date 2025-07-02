#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

// Déclarations de fonctions
void showLCDMessage(String line1, String line2);
void resetToScanMessage();

// RFID
MFRC522DriverPinSimple ss_pin(5);
MFRC522DriverSPI driver{ss_pin};
MFRC522 mfrc522{driver};
MFRC522::MIFARE_Key key;

// LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Données RFID
byte blockAddress = 2;
byte bufferblocksize = 18;
byte blockDataRead[18];

void setup() {
  Serial.begin(115200);
  while (!Serial);

  lcd.init();
  lcd.backlight();
  showLCDMessage("Scan badge RFID", "");

  mfrc522.PCD_Init();
  Serial.println(F("RFID prêt. Scanner un badge."));

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(200);
    return;
  }

  Serial.print("Card UID: ");
  MFRC522Debug::PrintUID(Serial, (mfrc522.uid));
  Serial.println();

  if (mfrc522.PCD_Authenticate(0x60, blockAddress, &key, &(mfrc522.uid)) != 0) {
    Serial.println("Authentication failed.");
    showLCDMessage("Mauvais code", "Reessayez");
    delay(2000);
    resetToScanMessage();
    return;
  }

  if (mfrc522.MIFARE_Read(blockAddress, blockDataRead, &bufferblocksize) != 0) {
    Serial.println("Read failed.");
    showLCDMessage("Erreur lecture", "Reessayez");
    delay(2000);
    resetToScanMessage();
  } else {
    Serial.print("Data read: ");
    String data = "";
    for (byte i = 0; i < 16; i++) {
      char c = (char)blockDataRead[i];
      Serial.print(c);
      data += c;
    }
    Serial.println();

    if (data.indexOf("Rui") != -1) {
      showLCDMessage("Coffre ouvert!", "");
      Serial.println("✅ Badge valide");
    } else {
      showLCDMessage("Mauvais code", "Reessayez");
      Serial.println("❌ Badge invalide");
    }

    delay(3000);
    resetToScanMessage();
  }

  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}

void showLCDMessage(String line1, String line2) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

void resetToScanMessage() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Scan badge RFID");
}
