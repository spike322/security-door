#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <WiFiNINA.h>

#include "arduino_secrets.h" 

#define SS_PIN 10
#define RST_PIN 9
#define ALLOWED_LED 4
#define NOT_ALLOWED_LED 5


char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

Servo servo;

MFRC522 mfrc522(SS_PIN, RST_PIN); 

LiquidCrystal_I2C lcd(0x27, 16, 2);

void openDoor() {
    lcd.clear();
    lcd.print("Allowed to pass");
    digitalWrite(ALLOWED_LED, HIGH);
    for(int i = 0; i <= 90; i++) {
      servo.write(i);
    }
    delay(3000);
    for(int i = 90; i >= 0; i--) {
      servo.write(i);
    }
}

void wifiConnect() {
  Serial.println("Checking Wi-fi status ...");

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  // you're connected now, so print out the data:
  Serial.print("You're connected to the network");
  printCurrentNet();
  printWifiData();
}

void printWifiData() {
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);

  // print your MAC address:
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the MAC address of the router you're attached to:
  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

  // print the encryption type:
  byte encryption = WiFi.encryptionType();
  Serial.print("Encryption Type:");
  Serial.println(encryption, HEX);
  Serial.println();
}

void printMacAddress(byte mac[]) {
  for (int i = 5; i >= 0; i--) {
    if (mac[i] < 16) {
      Serial.print("0");
    }
    Serial.print(mac[i], HEX);
    if (i > 0) {
      Serial.print(":");
    }
  }
  Serial.println();
}

void setup() {
  pinMode(ALLOWED_LED, OUTPUT);
  pinMode(NOT_ALLOWED_LED, OUTPUT);

  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.print("Waiting for a card...");
  servo.attach(2);
  servo.write(0);
  Serial.begin(9600);  
  while (!Serial);
  wifiConnect();     
  SPI.begin();
  mfrc522.PCD_Init();  
  Serial.println("Approximate your card to the reader...");
  Serial.println();
}

void loop() {
  digitalWrite(ALLOWED_LED, LOW);
  digitalWrite(NOT_ALLOWED_LED, LOW);

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) 
  {
    return;
  }
  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) 
  {
    return;
  }
  //Show UID on serial monitor
  Serial.print("UID tag :");
  String content= "";
  byte letter;
  
  for (byte i = 0; i < mfrc522.uid.size; i++) {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  
  Serial.println();
  Serial.print("Message : ");
  content.toUpperCase();
  
  if (content.substring(1) == "17 0E 70 1C") {
    Serial.println("Authorized access");
    Serial.println();
    openDoor();
  } else {
    lcd.clear();
    lcd.print("Not allowed!");
    Serial.println("Access denied");
    Serial.println();
    digitalWrite(NOT_ALLOWED_LED, HIGH);
    delay(1000);
  }
}
