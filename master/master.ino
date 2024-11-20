// Mater
#include <SPI.h>
#include <MFRC522.h>
#include <ArduinoJson.h>

constexpr uint8_t RST_PIN = 9;   
constexpr uint8_t SS_PIN = 10;
constexpr uint8_t BUTTON_PIN = 2;
constexpr uint8_t BLUE_LED_PIN = 3;


MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

byte bufferLen = 18;
byte readBlockData[18];

MFRC522::StatusCode status;

bool weightMode = false;
bool buttonState;
bool lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();  
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // Button pin as input with pull-up resistor
  pinMode(BLUE_LED_PIN, OUTPUT);      // Blue LED pin as output
}

void loop() {
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  int reading = digitalRead(BUTTON_PIN);

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == LOW) {
        weightMode = !weightMode;
        digitalWrite(BLUE_LED_PIN, weightMode ? HIGH : LOW);  // Turn on blue LED if weight mode is active
      }
    }
  }

  lastButtonState = reading;

  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    for (byte i = 0; i < mfrc522.uid.size; i++) {
    }
    MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    // JSON object to hold RFID data
    DynamicJsonDocument jsonDoc(256);  // Adjust the capacity as needed

    for (int blockNum = 12; blockNum <= 14; blockNum++) {
      ReadDataFromBlock(blockNum, readBlockData);

      // Add RFID data to JSON object
      if (blockNum == 12)
        jsonDoc["ProductID"] = String((char*)readBlockData);
      else if (blockNum == 13)
        jsonDoc["ProductName"] = String((char*)readBlockData);
      else if (blockNum == 14)
        jsonDoc["Price"] = String((char*)readBlockData);
    }

    // Add weight data if weight mode is active
    if (weightMode) {
      float randomWeight = random(0, 1000) / 1000.0;  // Random weight between 0 to 1 kg
      jsonDoc["Weight"] = randomWeight;
    }

    // Serialize JSON object to a string
    String jsonString;
    serializeJson(jsonDoc, jsonString);

    // Send JSON string over serial
    Serial.println(jsonString);

    // Reset the card detection state
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
}

void ReadDataFromBlock(int blockNum, byte readBlockData[]) {
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(mfrc522.uid));

  if (status != MFRC522::STATUS_OK) {
    return;
  } else {
  }

  status = mfrc522.MIFARE_Read(blockNum, readBlockData, &bufferLen);
  if (status != MFRC522::STATUS_OK) {
    return;
  } else {
  }
}


  