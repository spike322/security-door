#include <Servo.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define SS_PIN 10
#define RST_PIN 9
#define ALLOWED_LED 4
#define NOT_ALLOWED_LED 5

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

void setup() {
  pinMode(ALLOWED_LED, OUTPUT);
  pinMode(NOT_ALLOWED_LED, OUTPUT);

  lcd.begin();
  lcd.backlight();
  lcd.clear();
  lcd.print("Waiting for a card...");
  servo.attach(2);
  servo.write(0);
  Serial.begin(9600);  // Initialize serial communications with the PC
  while (!Serial);     
  SPI.begin();         // Init SPI bus
  mfrc522.PCD_Init();  // Init MFRC522 card
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
