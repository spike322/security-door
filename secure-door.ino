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
#define UP 13
#define DOWN 12
#define CHECK 3

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
String options[] = {"       ENTER","      ADD USER","    DELETE USER"};
int status = WL_IDLE_STATUS;
int choice = 0;
String uid = "";
String sPayload;
char* cPayload;
bool first = false;
bool second = false;

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
    lcd.setCursor(0, 0);
    lcd.print("   ALLOWED TO PASS!");
    digitalWrite(ALLOWED_LED, HIGH);
    for(int i = 0; i <= 90; i++) {
      servo.write(i);
    }
    
    delay(3000);
    lcd.setCursor(0, 0);
    lcd.print("Please, close the   door :)");
    while(!digitalRead(CHECK));
    
    for(int i = 90; i >= 0; i--) {
      servo.write(i);
    }
}

void denyAccess() {
  lcd.clear();
  lcd.print("    NOT ALLOWED!");
  Serial.println("Access denied");
  Serial.println();
  digitalWrite(NOT_ALLOWED_LED, HIGH);
  delay(3000);
}

void mqttBrokerConnect() {
  mqttClient.setUsernamePassword(username, password);

  Serial.print("Attempting to connect to the MQTT broker: ");
  Serial.println(broker);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting to MQTT  broker");

  if (!mqttClient.connect(broker, port)) {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(mqttClient.connectError());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MQTT connection failed!");

    while (1);
  }

  Serial.println("You're connected to the MQTT broker!");
  Serial.println();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected to MQTT   broker");

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
  delay(500);
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
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(options[choice]);
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
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Connecting to WPA:");
    lcd.setCursor(0, 1);
    lcd.print(ssid);
    status = WiFi.begin(ssid, pass);

    delay(10000);
  }

  Serial.print("You're connected to the network");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connected!");
  printCurrentNet();
  printWifiData();
  delay(500);
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

  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(options[choice]);
}

void setup() {
  pinMode(ALLOWED_LED, OUTPUT);
  pinMode(NOT_ALLOWED_LED, OUTPUT);
  pinMode(CHECK, INPUT);
  pinMode(UP, INPUT);
  pinMode(DOWN, INPUT);

  lcd.begin();
  lcd.backlight();
  lcd.clear();

  if(!digitalRead(CHECK)) {
    lcd.setCursor(0, 0);
    lcd.print("  MAINTENANCE MODE");
    lcd.setCursor(0, 1);
    lcd.print("Close the door to");
    lcd.setCursor(0, 2);
    lcd.print("begin");

    digitalWrite(ALLOWED_LED, HIGH);
    digitalWrite(NOT_ALLOWED_LED, HIGH);
    
    while(!digitalRead(CHECK));
  }

  servo.attach(2);
  
  servo.write(90);
  delay(1000);
  
  servo.write(0);
  Serial.begin(9600);  
  while (!Serial);
  wifiConnect();
  mqttBrokerConnect();
  SPI.begin();
  mfrc522.PCD_Init();  
  Serial.println(options[choice]);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(options[choice]);
}

void loop() {
  mqttClient.poll();
  
  digitalWrite(ALLOWED_LED, LOW);
  digitalWrite(NOT_ALLOWED_LED, LOW);

  /*if (digitalRead(UP) == 1) { //INCREASE CHOICE
    choice = (choice + 1) % 3;
    Serial.println(options[choice]);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(options[choice]);
  } else if(digitalRead(DOWN) == 1) { //DECREASE CHOICE
    if (choice == 0) choice = 2;
    else choice--;
    Serial.println(options[choice]);
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(options[choice]);
  }

  delay(500);*/

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
  
  openDoorRequest(content.substring(1));

  delay(1000);
}
