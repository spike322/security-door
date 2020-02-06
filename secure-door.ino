#include <Servo.h>
#include <SPI.h>
#include <RFID.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiNINA.h>
#include <ArduinoMqttClient.h>

#include "arduino_secrets.h" 

#define SDA_DIO 10
#define RESET_DIO 9
#define ALLOWED_LED 4
#define NOT_ALLOWED_LED 5
#define UP 13
#define DOWN 12
#define CHECK 3
#define BUZZER 7

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
String options[] = {"        ENTER","      ADD USER","    DELETE USER"};
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
const char delete_request[] = "delete/request";
const char delete_response[] = "delete/response";

Servo servo;
RFID RC522(SDA_DIO, RESET_DIO); 
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

    tone(BUZZER, 1000);
    delay(100);
    noTone(BUZZER);
    tone(BUZZER, 1000);
    delay(100);
    noTone(BUZZER);
    
    servo.write(90);
    
    delay(4000);
    lcd.setCursor(0, 0);
    lcd.print("Please, close the   door :)");
    while(!digitalRead(CHECK));
    delay(1000);
    
    servo.write(0);
}

void checkIsOpen() {
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

    printActualChoice();
  }
}

void denyAccess() {
  lcd.clear();
  lcd.print("    NOT ALLOWED!");
  Serial.println("Access denied");
  Serial.println();
  digitalWrite(NOT_ALLOWED_LED, HIGH);
  tone(BUZZER, 1000);
  delay(1000);
  noTone(BUZZER);
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
  Serial.print(" - ");
  Serial.println(delete_request);
  mqttClient.subscribe(delete_request);
  Serial.print(" - ");
  Serial.println(delete_response);
  mqttClient.subscribe(delete_response);
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
    lcd.clear();
    lcd.setCursor(0, 1);
    s.toUpperCase();
    lcd.print(s);
    delay(3000);
  } else if(topic == delete_response) {
    Serial.println(s);
    lcd.clear();
    lcd.setCursor(0, 1);
    s.toUpperCase();
    lcd.print(s);
    delay(3000);
  }
  
  printActualChoice();
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

void printActualChoice() {
  Serial.println(options[choice]);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print(options[choice]);
}

void readCard() {
  byte i;

  if (RC522.isCard()) {
    tone(BUZZER, 1000);
    delay(200);
    noTone(BUZZER);
    RC522.readCardSerial();
    Serial.println("Card UID:");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wait...");
 
    for(i = 0; i <= 4; i++) {
      uid += String (RC522.serNum[i],HEX);
      uid.toUpperCase();
    }
    Serial.println(uid);
    delay(800);
  }
}

void addCard(String newCard) {
  uid = "";
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("New card UID:");
  lcd.setCursor(0, 1);
  lcd.print(newCard);
  Serial.print("New card UID: ");
  Serial.println(newCard);
  Serial.println();
  delay(3000);
  Serial.println("Confirm with administrator card...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Admin confirmation...");
  while(uid == "") {
    readCard();
    delay(500);
  }
  String admin = uid;
  uid = "";
  
    
  mqttClient.beginMessage(register_request);
  mqttClient.print((String) "{ \"newCard\": \""+ newCard + "\", \"admin\": \""+ admin + "\" }");
  mqttClient.endMessage();
  
  choice = 0;
}

void deleteCard(String deleteUid) {
  uid = "";
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Delete UID:");
  lcd.setCursor(0, 1);
  lcd.print(deleteUid);
  Serial.print("Delete UID: ");
  Serial.println(deleteUid);
  Serial.println();
  delay(3000);
  Serial.println("Confirm with administrator card...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Admin confirmation...");
  while(uid == "") {
    readCard();
    delay(500);
  }
  String admin = uid;
  uid = "";
  
    
  mqttClient.beginMessage(delete_request);
  mqttClient.print((String) "{ \"deleteUid\": \""+ deleteUid + "\", \"admin\": \""+ admin + "\" }");
  mqttClient.endMessage();
  
  choice = 0;
}

void setup() {
  // OUTPUT PINS
  pinMode(ALLOWED_LED, OUTPUT);
  pinMode(NOT_ALLOWED_LED, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  // INPUT PINS
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

    tone(BUZZER, 1000);
    delay(1000);
    noTone(BUZZER);

    digitalWrite(ALLOWED_LED, HIGH);
    digitalWrite(NOT_ALLOWED_LED, HIGH);
    
    while(!digitalRead(CHECK));
  }

  servo.attach(2);
  servo.write(90);
  servo.write(0);
  Serial.begin(9600);  
  while (!Serial);
  wifiConnect();
  mqttBrokerConnect();
  SPI.begin(); 
  RC522.init();
  printActualChoice();
  tone(BUZZER, 1000);
  delay(200);
  noTone(BUZZER);
}

void loop() {  
  mqttClient.poll();
  
  digitalWrite(ALLOWED_LED, LOW);
  digitalWrite(NOT_ALLOWED_LED, LOW);

  if (digitalRead(UP) == 1) { //INCREASE CHOICE
    choice = (choice + 1) % 3;
    printActualChoice();
  } else if(digitalRead(DOWN) == 1) { //DECREASE CHOICE
    if (choice == 0) choice = 2;
    else choice--;
    printActualChoice();
  }

  delay(150);

  readCard();

  if(uid != "") {
    switch(choice) {
      case 0:
        Serial.println("Request!");
        openDoorRequest(uid);
        break;
      case 1:
        Serial.println("Add!");
        addCard(uid);
        break;
      case 2: 
        Serial.println("Delete!");
        deleteCard(uid);
        break;
      default: break;
    }
    uid = "";
  }

  //checkIsOpen();
}
