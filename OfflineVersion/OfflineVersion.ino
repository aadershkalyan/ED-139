//Add all necessary libraries
#include <LiquidCrystal.h>
#include <SPI.h>
#include <MFRC522.h>
#include <stdlib.h>
#include <Dictionary.h>

//Setting up constants for the display
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Setting up constants for the NFC reader
#define SS_PIN 53
#define RST_PIN 49
MFRC522 mfrc522(SS_PIN, RST_PIN);

//Setting up constants for the sensor
const int sensorPins[] = {A0, A1, A2, A3, A4, A5, A6, A7, A8};
const int sensitivity = 200; 
const int scanTimeout = 1000;
int currentScan = 0;

//Variables for the program
bool loggedIn = false;
int activePassID = 0;
long loginTime = 0;
const int logoutTime = 60000;

//Assigning the active qrs
const int qrCodes[] = {186, 27};
const int acceptAmount = 2;

//User Database
Dictionary &users_name = *(new Dictionary(6));

//Active Items Databse
Dictionary &qr_deposit = *(new Dictionary(2));

//User Balance Database
Dictionary &users_balance = *(new Dictionary(6));

//Func to check element in array
bool inArray (int array[], int element, int size) {
  for (int i = 0; i < size; i++) {
    if(array[i] == element) {
      return true;
    } 
  }

  return false;
}

void setup() {
  //Populate Databases
  users_name("209", "Henk Geert");
  users_name("108", "Geert Peter");
  users_name("236", "Jan Jans");
  users_name("227", "Ron Jans");
  users_name("20", "Bob Henk");
  users_name("71", "Willem Hout");

  users_balance("209", "1.18");
  users_balance("108", "5.87");
  users_balance("236", "0.00");
  users_balance("227", "2.55");
  users_balance("20", "1.12");
  users_balance("71", "3,89");

  qr_deposit("27", "0.05");
  qr_deposit("186", "0.15");

  //Begin communication channels
  Serial.begin(9600); //Only for debugging
  lcd.begin(16, 2);
  SPI.begin();
  mfrc522.PCD_Init();

  //Display default message
  lcd.print("Scan card to log in...");
  lcd.noCursor();
}

//logout shortcut
void logout () {
  loggedIn = false;
  loginTime = 0;
  lcd.clear();
  lcd.print("Scan card to log in...");
}

void loop() {
  //Check if a user is logged in
  if(!loggedIn) {
    // Look for new cards
    if (!mfrc522.PICC_IsNewCardPresent()) 
    {
      return;
    }
    // Select one of the cards
    if (!mfrc522.PICC_ReadCardSerial()) 
    {
      return;
    }

    //Read NFC tag contents 
    String content = "";
    byte letter;
    //Compensate for error
    for (byte i = 0; i < mfrc522.uid.size; i++) {
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));        
    }

    //Process read UID
    char temp[10];
    content.toUpperCase();
    content = content.substring(1,3);
    content.toCharArray(temp, content.length() + 1);
    activePassID = strtol(temp, NULL, 16);  

    //Check if user exists and display greeting
    String name = users_name[String(activePassID)];
    lcd.clear();
    lcd.print("Welcome, " + name);

    //Update tracking vars
    loggedIn = true;
    loginTime = millis();
  } else if (loggedIn){
    //logout if idle for too long
    if (millis() - loginTime >= logoutTime) {
      logout();
    } else if (millis() - currentScan >= scanTimeout) {
        //scan each pixel of the qr and compute the signature
        int scannedQr = 0;
        for (int i = 0; i < 9; i++) {
          int val = analogRead(sensorPins[i]);
          if (val >= sensitivity) {
            scannedQr += pow(2, i);
          } 
        }

        Serial.println(scannedQr);

        //Check if the signature is a valid qr
        if(inArray(qrCodes, scannedQr, acceptAmount)) {
          //See if the qr is active
          if(qr_deposit(String(scannedQr))){
            //Lookup current userbalance
            float balance = users_balance[String(activePassID)].toFloat();
            float value = qr_deposit[String(scannedQr)].toFloat();
            balance = balance + value;
            users_balance(String(activePassID), String(balance));

            //Remove entry from active
            qr_deposit.remove(String(scannedQr));

            //Output the response
            lcd.clear();
            lcd.print("New Balance: " + String(balance));

            //Pause for 2 seconds
            delay(2000);

            //Logout
            logout();
          }
        }

        //Reset scantimer
        currentScan = 0;
      }      
  }

}
