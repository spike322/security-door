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
#define UP 1
#define DOWN 2
#define CHECK 3

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
String options[] = {"Approximate your card to the reader...","Add user...","Delete user..."};
int status = WL_IDLE_STATUS;
int choice = 0;
String uid = "";
String sPayload;
char* cPayload;

/*************** TEMPORARY SIMULATION ***************/
const int deniedButton = 8;
const int allowedButton = 7;
/****************************************************/

//MQTT Credentials
const char broker[] = MQTT_BROKER;
const int port = MQTT_BROKER_PORT;
const char username[] = MQTT_BROKER_USER;
const char password[] = MQTT_BROKER_PWD;
const char access_request[] = "access/request";
const char access_response[] = "access/response";
const char register_request[] = "register/request";
const char register_response[] = "register/response";

Servo servo;
MFRC522 mfrc522(SS_PIN, RST_PIN); 
LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

unsigned long lastMillis = 0;

void openDoorRequest(String addressCard) {
  mqttClient.beginMessage(access_request);
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
  Serial.println(access_request);
  mqttClient.subscribe(access_request);
  Serial.print(" - ");
  Serial.println(access_response);
  mqttClient.subscribe(access_response);
  Serial.print(" - ");
  Serial.println(register_request);
  mqttClient.subscribe(register_request);
  Serial.print(" - ");
  Serial.println(register_response);
  mqttClient.subscribe(register_response);
}

// On receive message from the topic
void onMqttMessage(int messageSize) {
  String topic = (String) mqttClient.messageTopic();
  String s = "";
  while (mqttClient.available()) {
    s.concat((char)mqttClient.read());
  }
  if (topic == access_response) {
    if (s == "success") {
      openDoor();
    } else {
      denyAccess();
    }
  } else if (topic == register_response) {
    Serial.println(s);
  }
  
  Serial.println(options[choice]);
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

String readCard() {
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
  return (String) content;
}


String readCardWaiting() {
  // Look for new cards
  while ( ! mfrc522.PICC_IsNewCardPresent());
  // Select one of the cards
  while ( ! mfrc522.PICC_ReadCardSerial());
  String content= "";
  byte letter;
  
  for (byte i = 0; i < mfrc522.uid.size; i++) {
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  
  content.toUpperCase();
  return (String) content;
}

void addCard(String newCard) {
  Serial.print("New card UID: ");
  Serial.println(newCard);
  Serial.println();
  Serial.println("Confirm with administrator card...");
  String admin = "E9 33 97 47";//readCardWaiting();
    
  mqttClient.beginMessage(register_request);
  mqttClient.print((String) "{ \"newCard\": \""+ newCard + "\", \"admin\": \""+ admin + "\" }");
  mqttClient.endMessage();
  
  choice = 0;
}

void setup() {
  pinMode(ALLOWED_LED, OUTPUT);
  pinMode(NOT_ALLOWED_LED, OUTPUT);
  pinMode(CHECK, INPUT);
  pinMode(UP, INPUT);
  pinMode(DOWN, INPUT);

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
  Serial.println(options[choice]);
}

void loop() {
  mqttClient.poll();
  
  digitalWrite(ALLOWED_LED, LOW);
  digitalWrite(NOT_ALLOWED_LED, LOW);

  if (digitalRead(UP) == 1) { //INCREASE CHOICE
    choice = (choice + 1) % 3;
    Serial.println(options[choice]);
  } else if(digitalRead(DOWN) == 1) { //DECREASE CHOICE
    if (choice > 0) {
      choice = (choice - 1) % 3;
      Serial.println(options[choice]);
    }
  }

  delay(150);

  /*switch(choice) {
    case 0:
      Serial.println("Approximate your card to the reader...");
      Serial.println();
      break;
    case 1:
      Serial.println("Add user card...");
      Serial.println();
      break;
    case 2:
      Serial.println("Delete user card...");
      Serial.println();
      break;
    default: break;
  }*/

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
  String uid= "";
  byte letter;
  
  for (byte i = 0; i < mfrc522.uid.size; i++) {
     Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
     Serial.print(mfrc522.uid.uidByte[i], HEX);
     uid.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     uid.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  uid.toUpperCase();*/

  /*************** TEMPORARY SIMULATION ***************/
  if(digitalRead(allowedButton) == 1) {
    uid = "17 0E 70 1C";
    Serial.println("Green pressed!");
  }
  if(digitalRead(deniedButton) == 1) {
    uid = "AA BB CC DD";
    Serial.println("Red pressed!");
  }
  /****************************************************/

  if(uid != "") {
    switch(choice) {
      case 0:
        openDoorRequest(uid);
        break;
      case 1:
        addCard(uid);
        break;
      case 2:
        //removeCard(uid);
        break;
      default: break;
    }
  }

  uid = "";
}
