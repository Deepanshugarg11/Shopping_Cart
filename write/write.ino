#include <SPI.h>      // Include the SPI library
#include <MFRC522.h>  // Include the MFRC522 RFID reader library

#define RST_PIN 9     // Reset pin
#define SS_PIN 10     // Slave select pin
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create a MFRC522 instance
MFRC522::MIFARE_Key key;            // Create a MIFARE_Key struct

// Data for three fields (16 bytes each)
byte field1[16] = {"Field1-Data"};  
byte field2[16] = {"Field2-Data"};  
byte field3[16] = {"Field3-Data"};  
byte readbackblock[18];              // Array for reading

void setup() {
  Serial.begin(115200);        // Initialize serial communication
  SPI.begin();                 // Init SPI bus
  mfrc522.PCD_Init();          // Init MFRC522 card
  Serial.println("Scan a MIFARE Classic card");
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;     // Prepare security key
  }
}

void loop() {
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  // Read from the card
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  
  Serial.println("Card detected. Writing data");
  writeBlock(1, field1); // Write field1 to block 1
  writeBlock(2, field2); // Write field2 to block 2
  writeBlock(4, field3); // Write field3 to block 4 (not trailer)

  Serial.println("Reading data from the tag");
  readBlock(1, readbackblock);   // Read block 1
  Serial.print("Read block 1: ");
  Serial.write(readbackblock, 16);
  Serial.println("");

  readBlock(2, readbackblock);   // Read block 2
  Serial.print("Read block 2: ");
  Serial.write(readbackblock, 16);
  Serial.println("");

  readBlock(4, readbackblock);   // Read block 4 (not trailer)
  Serial.print("Read block 4: ");
  Serial.write(readbackblock, 16);
  Serial.println("");
}

// Function to write to a data block
int writeBlock(int blockNumber, byte arrayAddress[]) {
  // Check if the block number corresponds to a data block or trailer block
  int trailerBlock = (blockNumber / 4 * 4) + 3; // Determine trailer block
  if (blockNumber % 4 == 3) {
    Serial.print(blockNumber);
    Serial.println(" is a trailer block: Error");
    return 2; // Error: Attempt to write to trailer block
  }

  // Authentication
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return 3; // Return error message
  }
  
  // Writing data to the block
  status = mfrc522.MIFARE_Write(blockNumber, arrayAddress, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Data write failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return 4; // Return error message
  }
  
  Serial.print("Data written to block ");
  Serial.println(blockNumber);
  return 0; // Successful write
}

// Function to read from a data block
int readBlock(int blockNumber, byte arrayAddress[]) {
  int trailerBlock = (blockNumber / 4 * 4) + 3; // Determine trailer block
  
  // Authentication of the desired block
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Authentication failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return 3; // Return error message
  }
  
  // Reading data from the block
  byte buffersize = 18; // Buffer size
  status = mfrc522.MIFARE_Read(blockNumber, arrayAddress, &buffersize);
  if (status != MFRC522::STATUS_OK) {
    Serial.print("Data read failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return 4; // Return error message
  }
  
  Serial.println("Data read successfully");
  return 0; // Successful read
}
