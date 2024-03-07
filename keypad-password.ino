#include <Servo.h>
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>
#include <SoftwareSerial.h>


#define RST_PIN         9
#define SS_PIN          10
#define rxPin           16
#define txPin           15
#define pinBuzz         9


Servo myServo;
MFRC522 mfrc522(SS_PIN, RST_PIN);
LiquidCrystal_I2C lcd(0x27,16,2);
SoftwareSerial sim800L(rxPin,txPin); 



int UID[4], i;
int ID1[4] = {65, 172, 97, 71 }; //Thẻ mở đèn
int ID2[4] = {83, 113, 66, 31 }; //Thẻ tắt đèn



int ID3[4] = {80, 145, 153, 138 }; //Thẻ đúng

const byte ROWS = 4; //four rows
const byte COLS = 3; //four columns

// Password variables
char enteredPassword[5]=""; // Assuming a 4-digit password
char correctPassword[] = "1234"; //
bool continueOpen = false;
//define the cymbols on the buttons of the keypads
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte pin_rows[ROWS] = {2, 3, 4, 5}; // Roll D1 d2 s3 s2
byte pin_column[COLS] = {6, 7, 8}; // col  d5 d6 d7
// setlock
int wrongPassword=0;
const int maxwrongPassword =3;
int timelock=0;
int count=0;
int flag=0;
//initialize an instance of class NewKeypad - Khởi tạo đối tượng customKeypad
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROWS, COLS); 

void setup(){
Serial.begin(9600);
sim800L.begin(9600);

pinMode(pinBuzz,OUTPUT);
SPI.begin();    // rfid 
mfrc522.PCD_Init();
myServo.attach(14);
lcd.init();
lcd.backlight();
lcd.clear();
lcd.setCursor(1, 0);
lcd.print("Enter CARD:");
//display_quetthe();
delay(50);
myServo.write(0);
check_sim800l();
 // Bật thông báo nhận dạng cuộc gọi và tin nhắn
  sim800L.print("AT+CNMI=2,2,0,0,0\r\n");
}

void loop(){
check_sms_to_unlock(); 
if (continueOpen==true){
    step2();
}
else
    step1();
}

void handleRfid(){
    // rfid
    flag=0; 
if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    Serial.println("RFID Card Detected!");
    Serial.print("Card UID: ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      UID[i] = mfrc522.uid.uidByte[i];
      Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
      Serial.print(mfrc522.uid.uidByte[i]);
    }

  if (UID[i] == ID3[i])
  {
    //continueOpen = true;
    if(continueOpen == false)
    {
    lcd.setCursor(0, 1);
    lcd.print("id unlock");
    delay(1000);
    lcd.setCursor(0, 1);
    lcd.print("                 ");
    Serial.println("Unlock Door"); 
    continueOpen = true;
    }
    else
    {
    lcd.setCursor(0, 1);
    lcd.print("id lock");
    delay(1000);
    lcd.setCursor(0, 1);
    lcd.print("                 ");
    Serial.println("Lock Door"); 
    continueOpen = false;
    }
  }

  else
  {
    continueOpen = false;
    lcd.setCursor(0, 1);
    lcd.print("Please try again"); 
    delay(1000);
    lcd.setCursor(0, 1);
    lcd.print("                "); 
  }
    Serial.println();
    mfrc522.PICC_HaltA();
    delay(1000); // Add delay to avoid multiple readings
  }
}

void keyPad(){
char key = keypad.getKey();
 if (key) {
    if (key == '#') {
      // Check the entered password
      if (strcmp(enteredPassword, correctPassword) == 0) {
        // Toggle the LED if the password is correct
        lcd.clear();
        lcd.print("Unlock Door");
        myServo.write(90);
        delay(5000);
        myServo.write(0);
        lcd.clear();
        lcd.print("                 ");
        lcd.setCursor(1, 0);
        //lcd.print("Enter Password:");
        clearEnteredPassword();
        wrongPassword=0;
        continueOpen = false;

      } 
      else {
        lcd.clear();
        lcd.print("Wrong Password");
        delay(2000);
        lcd.clear();
         lcd.print("Enter Password:");
        clearEnteredPassword();
        wrongPassword++;
        if (wrongPassword>=maxwrongPassword){
          send_sms();
          digitalWrite(pinBuzz, HIGH);
          delay(3000);
          digitalWrite(pinBuzz,LOW);
          delay(1000);
          lcd.clear();
          lcd.print("Keypad Lock");
        for(timelock=0;timelock<=1000;timelock++){
          delay(10);
        }
        wrongPassword=0;
        lcd.clear();
        //lcd.print("Enter Password:");
        continueOpen = false;
        }
      }
    }
    else if (key == '*') {
      delay(200);
      changePassword();
    } else {
      if (strlen(enteredPassword) < 4) 
      {
        enteredPassword[strlen(enteredPassword)] = key;
        lcd.setCursor(strlen(enteredPassword) - 1, 1);
        lcd.print('*');
      }
  }
  }
}

void clearEnteredPassword() {
  memset(enteredPassword, 0, sizeof(enteredPassword));
  lcd.setCursor(0, 1);
  lcd.print("                "); // Clear the password display
  lcd.setCursor(0, 1);
}

void changePassword() {
  lcd.clear();
  lcd.print("Enter New Password:");
  clearEnteredPassword();
  delay(200);
  while (strlen(enteredPassword) < 4) {
    char key = keypad.getKey();
    if (key && key != '*' && key != '#') {
      enteredPassword[strlen(enteredPassword)] = key;
      lcd.setCursor(strlen(enteredPassword) - 1, 1);
      lcd.print('*');
    }
  }
  delay(200);
  memcpy(correctPassword, enteredPassword, sizeof(enteredPassword)); // Copies the entered password to a buffer named correctPassword.
  lcd.clear();
  lcd.print("Password Changed");
  delay(2000);
  lcd.clear();
  lcd.print("Enter Password:");
  clearEnteredPassword();
}


void check_sim800l()
{
  sim800L.println("AT");
  waitForResponse();

  sim800L.println("ATE1");
  waitForResponse();

  sim800L.println("AT+CMGF=1");
  waitForResponse();

  sim800L.println("AT+CNMI=1,2,0,0,0");
  waitForResponse();
}

void make_call(){
  sim800L.println("ATD+84981121557;"); //84898341301
  waitForResponse();
}

void waitForResponse(){
  delay(1000);
  while(sim800L.available()){
    Serial.println(sim800L.readString());
  }
  sim800L.read();
}


void send_sms(){
  sim800L.print("AT+CMGS=\"+84981121557\"\r");
  waitForResponse();
  sim800L.print("Canh bao xam nhap");
  sim800L.write(0x1A);
  waitForResponse();
}


void step1(){
    lcd.setCursor(1, 0);
    lcd.print("Enter CARD:");
    handleRfid();
}

void step2()
{
    lcd.setCursor(1, 0);
    lcd.print("Enter Password:");
    keyPad();
}

void check_sms_to_unlock()
{
   if (sim800L.available()) {
    char incomingChar = sim800L.read();

    if (incomingChar == '1') {
        sim800L.print("AT+CMGS=\"+84981121557\"\r");
        waitForResponse();
        sim800L.print("unlock");
        sim800L.write(0x1A);
        waitForResponse();
        lcd.clear();
        lcd.print("Unlock Door");
        myServo.write(90);
        delay(5000);
        myServo.write(0);
        lcd.clear();
        lcd.print("                 ");
        lcd.setCursor(1, 0);
        continueOpen = false;
     } 
    }
  }
