#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>

#include "arduino_secrets.h" 

#define SS_PIN 10
#define RST_PIN 9
#define ALLOWED_LED 4
#define NOT_ALLOWED_LED 5

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
int status = WL_IDLE_STATUS;

/*************** TEMPORARY SIMULATION ***************/
const int deniedButton = 8;
const int allowedButton = 7;
/****************************************************/

//MQTT Credentials
const char broker[] = MQTT_BROKER;
const int port = MQTT_BROKER_PORT;
const char username[] = MQTT_BROKER_USER;
const char password[] = MQTT_BROKER_PWD;
const char access_allowed[] = "access/allowed";
const char access_denied[] = "access/denied";
const char sender_topic[] = "access/request";
const char register_new[] = "register/new";
const char register_allowed[] = "register/allowed";
const char register_denied[] = "register/denied";

Servo servo;
MFRC522 mfrc522(SS_PIN, RST_PIN); 
LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

unsigned long lastMillis = 0;

void openDoorRequest(String addressCard) {
  mqttClient.beginMessage(sender_topic);
  mqttClient.print((String) addressCard);
  mqttClient.endMessage();
}

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

void denyAccess() {
  lcd.clear();
  lcd.print("Not allowed!");
  Serial.println("Access denied");
  Serial.println();
  digitalWrite(NOT_ALLOWED_LED, HIGH);
  delay(1000);
}

void mqttBrokerConnect() {
  mqttClient.setUsernamePassword(username, password);

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();

  // set the message receive callback
  mqttClient.onMessage(onMqttMessage);

  // subscribe to topics
  Serial.println("Subscribing to topics: ");
  Serial.print(" - ");
  Serial.println(sender_topic);
  mqttClient.subscribe(sender_topic);
  Serial.print(" - ");
  Serial.println(access_allowed);
  mqttClient.subscribe(access_allowed);
  Serial.print(" - ");
  Serial.println(access_denied);
  mqttClient.subscribe(access_denied);
}

// On receive message from the topic
void onMqttMessage(int messageSize) {
  if (mqttClient.messageTopic() == access_allowed) {
    openDoor();
  } else if (mqttClient.messageTopic() == access_denied) {
    denyAccess();
  }
} 

void wifiConnect() {
  Serial.println("Checking Wi-fi status ...");

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < "1.0.0") {
    Serial.println("Please upgrade the firmware");
  }

  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);

    delay(10000);
  }

  Serial.print("You're connected to the network");
  printCurrentNet();
  printWifiData();
}

void printWifiData() {
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println(ip);

  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  printMacAddress(mac);
}

void printCurrentNet() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  byte bssid[6];
  WiFi.BSSID(bssid);
  Serial.print("BSSID: ");
  printMacAddress(bssid);

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);

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

  // TEMPORARY SIMULATION
  pinMode(allowedButton, INPUT);
  pinMode(deniedButton, INPUT);

  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.print("Waiting for a card...");
  servo.attach(2);
  servo.write(0);
  Serial.begin(9600);  
  while (!Serial);
  wifiConnect();
  mqttBrokerConnect();
  SPI.begin();
  mfrc522.PCD_Init();  
  Serial.println("Approximate your card to the reader...");
  Serial.println();
}

void loop() {
  mqttClient.poll();
  
  digitalWrite(ALLOWED_LED, LOW);
  digitalWrite(NOT_ALLOWED_LED, LOW);

  // Look for new cards
  /*if ( ! mfrc522.PICC_IsNewCardPresent()) 
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
  content.toUpperCase();*/

  if(digitalRead(allowedButton) == 1) {
    openDoorRequest("17 0E 70 1C");//content.substring(1)
    Serial.println("Green pressed!");
  }
  if(digitalRead(deniedButton) == 1) {
    openDoorRequest("E9 33 97 47");
    Serial.println("Red pressed!");
  }
  
  /*if (content.substring(1) == "17 0E 70 1C") {
    lcd.clear();
    Serial.println("Authorized access");
    Serial.println();
    openDoorRequest(content.substring(1));
  } else {
    lcd.clear();
    lcd.print("Not allowed!");
    Serial.println("Access denied");
    Serial.println();
    digitalWrite(NOT_ALLOWED_LED, HIGH);
    delay(1000);
  }*/
  delay(500);
}
