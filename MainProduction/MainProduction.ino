#include <LiquidCrystal.h>
#include <WiFiEsp.h>
#include <SPI.h>
#include <MFRC522.h>
#include <stdlib.h>

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

//Setting up constants for networking
char ssid[] = "Dure Baksteen";
char pass[] = "vierkantekaasyosti";
int status = WL_IDLE_STATUS;
WiFiEspClient client;

//Server Adress
char server[] = "cdc2-2a02-a210-a443-2c80-94d-5219-562d-4022.eu.ngrok.io";

//List of Acceptable QR codes
const int qrCodes[] = {1, 2, 3, 5, 6, 7, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 21, 22, 23, 26, 27, 28, 29, 30, 31, 33, 35, 37, 39, 40, 41, 42, 43, 44, 45, 46, 47, 49, 51, 53, 55, 56, 57, 58, 59, 60, 61, 62, 63, 68, 69, 70, 71, 76, 77, 78, 79, 84, 85, 86, 87, 92, 93, 94, 95, 97, 98, 99, 101, 102, 103, 105, 106, 107, 108, 109, 110, 111, 113, 114, 115, 117, 118, 119, 121, 122, 123, 124, 125, 126, 127, 141, 142, 143, 157, 158, 159, 170, 171, 173, 175, 186, 187, 189, 191, 197, 198, 199, 205, 206, 207, 213, 214, 215, 221, 222, 223, 229, 231, 237, 238, 239, 245, 247, 253, 254, 255, 325, 327, 335, 341, 343, 351, 365, 367, 381, 383, 495, 511};
const int acceptAmount = 139;

//Variables for the program
bool loggedIn = false;
int activePassID = 0;
long loginTime = 0;
const int logoutTime = 60000;

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
  //Begin communication channels
  Serial.begin(9600); //Only for debugging
  lcd.begin(16, 2);
  SPI.begin();
  mfrc522.PCD_Init();
  Serial1.begin(9600);

  //Initialize wifi-module
  WiFi.init(&Serial1);

  //Attempt network connection
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    
    status = WiFi.begin(ssid, pass);
    lcd.print("Attempting to connect...");
  }  

  //Display default message
  lcd.print("Scan card to log in...");
  lcd.noCursor();
}

//logout shortcut
void logout () {
  loggedIn = false;
  loginTime = 0;
  lcd.print("Scan card to log in...");
}

void loop() {
  //Check if a user is logged in
  if(!loggedIn){
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

    //Connect to the server
    client.stop();
    if(client.connect(server, 80)) {
      Serial.println("Connected to server");
    } else {
      Serial.println("Failed to Connect to server");
      lcd.print("Failed server connect");
    }

    //Send the passID to the server
    client.println("GET \get_user.php?api_key=aardappel&pass_id=" + String(activePassID) + " HTTP/1.1");
    client.println("HOST: " + String(server));
    client.println("Connection: close");
    client.println();

    //Reveive response from server
    String response = "";
    while (client.available()) {
      char c = client.read();
      response.concat(c);
    }
    Serial.println(response);

    //Disconnect the server
    if (!client.connected()) {
      Serial.println("Server disconnected");
      client.stop();
    }

    //Output the response
    lcd.print("Welcome, " + response);

    //Update tracking vars
    loggedIn = true;
    loginTime = millis();
  } else if (loggedIn) {
    //logout if idle for too long
    if (millis() - loginTime >= logoutTime) {
      logout();
    } else if (millis() - currentScan >= scanTimeout) {
        //scan each pixel of the qr and compute the signature
        int scannedQr = 0;
        for (int i = 0; i < 9; i++) {
          int val = analogRead(sensorPins[i]);
          if (val <= sensitivity) {
            scannedQr += pow(2, i);
          } 
        }

        //Check if the signature is a valid qr
        if(inArray(qrCodes, scannedQr, acceptAmount)) {
          //Connect to server
          client.stop();
          if(client.connect(server, 80)) {
            Serial.println("Connected to server");
          } else {
            Serial.println("Failed to Connect to server");
            lcd.print("Failed server connect");
          }

          //Send qr to server for processing
          client.println("GET /check_qr.php?api_key=aardappel&qr_id=" + String(scannedQr) + "&pass_id=" + String(activePassID) + " HTTP/1.1");
          client.println("HOST: " + String(server));
          client.println("Connection: close");
          client.println();

          //Receive response from server
          String response = "";
          while (client.available()) {
            char c = client.read();
            response.concat(c);
          }
          Serial.println(response);

          //Disconnect the server
          if (!client.connected()) {
            Serial.println("Server disconnected");
            client.stop();
          }

          //Output the response
          lcd.print("Current Balance: " + response);

          //Pause for 2 seconds
          delay(2000);

          //Logout
          logout();
        }

        //Reset scantimer
        currentScan = 0;
      }
  }
}
