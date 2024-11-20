#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

const char* ssid = "IPHONE12";
const char* password = "12345678";
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic_add = "rfid/product/add";
const char* mqtt_topic_delete = "rfid/product/delete";
const char* mqtt_topic_payment_status = "rfid/payment/status";

const int buttonPin = D5;
const int greenLedPin = D3;
const int redLedPin = D7;
const int buzzerPin = D6;
const int IRSensor = D8;

WiFiClient espClient;
PubSubClient client(espClient);

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool addMode = true;
bool buttonState;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Function to update the display with mode and product status
void updateDisplay(const char* mode, const char* status) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Mode: ");
  lcd.print(mode);

  lcd.setCursor(0, 1);
  lcd.print(status);
}

// Function to set up pin modes
void pinMode_setup() {
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(IRSensor, INPUT);
  pinMode(greenLedPin, OUTPUT);
  pinMode(redLedPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
}

// Function to set up WiFi connection
void setup_wifi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wifi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" ");
  Serial.println("WiFi connected");
}

// MQTT callback function
void callback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument jsonDoc(256);
  deserializeJson(jsonDoc, payload, length);
  const char* status = jsonDoc["status"];
  float totalBill = jsonDoc["totalBill"];

  // Update display with payment status
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(status);
  lcd.setCursor(0, 1);
  lcd.print("Total: $");
  lcd.print(totalBill);

  // Play appropriate tone
  int toneFreq = (strcmp(status, "success") == 0) ? 1000 : 500;
  tone(buzzerPin, toneFreq);
  delay(200);
  noTone(buzzerPin);
}

// Function to reconnect to MQTT
void reconnect() {
  while (!client.connected()) {
    Serial.println("Reconnecting to MQTT");
    if (client.connect("NodeMCU_Client")) {
      Serial.println("Connected to MQTT");
      client.subscribe(mqtt_topic_payment_status);
    } else {
      Serial.println("MQTT connection failed, retrying again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode_setup();
  lcd.init();
  lcd.backlight();
  updateDisplay("Add", "Please Scan...");
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
  reconnect();
}

void loop() {
  client.loop();

  int reading = digitalRead(buttonPin);
  int IRState = digitalRead(IRSensor);  // Read the state of IR sensor

  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState == HIGH) {
        addMode = !addMode;
        digitalWrite(greenLedPin, addMode ? HIGH : LOW);
        digitalWrite(redLedPin, addMode ? LOW : HIGH);

        // Update display with mode change
        updateDisplay(addMode ? "Add" : "Remove", "Please Scan...");
      }
    }
  }

  lastButtonState = reading;

  // Check if data is available from Arduino
  if (Serial.available()) {
    String jsonStr = Serial.readStringUntil('\n');
    DynamicJsonDocument jsonDoc(256);
    DeserializationError error = deserializeJson(jsonDoc, jsonStr);

    if (!error) {
      if (addMode && IRState == LOW) {
        if (client.connected()) {
          client.publish(mqtt_topic_add, jsonStr.c_str());  // Changed this line
          Serial.println(jsonStr);                          // Print the JSON data received
          String prodName = jsonDoc["ProductName"];

          // Update display with product added status
          updateDisplay("ADD", (prodName + " Added").c_str());  // Changed this line

          int toneFreq = 1500;
          tone(buzzerPin, toneFreq);
          delay(1000);
          noTone(buzzerPin);
        } else {
          Serial.println("Mqtt Not Connected");
          reconnect();
        }
      } else if (!addMode && IRState == LOW) {
        if (client.connected()) {
          client.publish(mqtt_topic_delete, jsonStr.c_str());  // Changed this line
          Serial.println(jsonStr);                             // Print the JSON data received
          String prodName = jsonDoc["ProductName"];

          // Update display with product removed status
          updateDisplay("REMOVE", (prodName + " Removed").c_str());  // Changed this line

          int toneFreq = 100;
          tone(buzzerPin, toneFreq);
          delay(1000);
          noTone(buzzerPin);
        } else {
          Serial.println("Mqtt Not Connected");
          reconnect();
        }
      }
    } else {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
    }
  }
}
