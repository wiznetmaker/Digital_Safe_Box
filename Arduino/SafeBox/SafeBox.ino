 /* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID           "******************"
#define BLYNK_TEMPLATE_NAME         "****************"
#define BLYNK_AUTH_TOKEN            "******************"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Adafruit_Fingerprint.h>
#include <Servo.h>
#include <EEPROM.h>

// start reading from the first byte (address 0) of the EEPROM
byte value;

byte Password[6];
byte Password_Flash[6];
byte Password_New[6];

#include "TouchyTouch.h"

const int touch_threshold_adjust = 300;
const int touch_pins[] = {14,2,3,6,7,8,9,10,11,12,15,13};
const int touch_count = sizeof(touch_pins) / sizeof(int);
TouchyTouch touches[touch_count];

BlynkTimer timer;

#define FINGERPRINT_RX_PIN 23
#define FINGERPRINT_TX_PIN 24

SoftwareSerial fingerprintSerial(FINGERPRINT_RX_PIN, FINGERPRINT_TX_PIN); // Declare SoftwareSerial obj first
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerprintSerial);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SERVO_PIN 21
Servo servo;

int Key_num = 0;
int lock_status = 0;
int change_password_flag = 0;
uint32_t time_hold = 0;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(10, 0, 1, 226);

void setup() {  
  
  servo.attach(SERVO_PIN);
  // Initialize servo position
  servo.write(360); // Locked position

  for (int i = 0; i < touch_count; i++) {
    touches[i].begin( touch_pins[i] );
    touches[i].threshold += touch_threshold_adjust; // make a bit more noise-proof
  }  
  
  // Initialize components
  Serial.begin(115200);
  // while (!Serial) {
  //   ; // wait for serial port to connect. Needed for native USB port only
  // }

  EEPROM.begin(512);
  // //clear the flash
  // for (int i = 0; i < 512; i++) {
  //   EEPROM.write(i, 0);
  // }
  // EEPROM.commit();
  
  //read the password in the flash
  for(int i = 0; i<6; i++)
  {
//    Wrire default password to flash
    // EEPROM.write(i, 1);
    value = EEPROM.read(i);
    Password_Flash[i]= value;
    Serial.println(Password_Flash[i]); 
  }
  // EEPROM.commit();
  
  fingerprintSerial.begin(57600);
  
  display.begin(i2c_Address, true); // Address 0x3C default
  
  // Display initialization message
  display.clearDisplay();  
  display_logo(64,8);
  
  Ethernet.init(17);  // WIZnet W5100S-EVB-Pico
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  Blynk.begin(BLYNK_AUTH_TOKEN);

  finger.getTemplateCount();
  while (finger.templateCount == 0)
  {
    finger.getTemplateCount();
    getFingerprintEnroll(1);
  }
  Serial.print("Finger Count:"); 
  Serial.println(finger.templateCount);   
}

void loop() {
  
  Blynk.run();
  timer.run();
  
  getFingerprintID();
  // Check for touch button input
  // Add your touch button logic here
  Touch_handling();

  delay(20);            //don't ned to run this at full speed.
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) 
  {
    Serial.println("Found a print match!");
    unlockSafebox(); 
    lock_status = 1; 
  } 
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    
  // Display initialization message
  display.fillRect(21, 20, 89, 17, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(2);
  display.setCursor(36, 21);
  display.println("ALARM");
  // Send it to the server
  Blynk.virtualWrite(V5, "ALARM!");
  Blynk.logEvent("alarm1");
  display.display();  
  for(int n=0; n<5; n++)
  {    
    for(int i=200;i<=800;i++)              //用循环的方式将频率从200HZ 增加到800HZ
    {
      pinMode(29,OUTPUT);
      tone(29,i);                          //在四号端口输出频率
      delay(5);                            //该频率维持5毫秒   
    }
    delay(2000);
  }
  noTone(29);

  // Display initialization message
  display.fillRect(21, 20, 89, 17, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(2);
  display.setCursor(45, 21);
  display.println("lOCK");
  display.display();
  Blynk.virtualWrite(V5, "Lock!");
  //Blynk.logEvent("lock");
    return p;
  } else {
    Serial.println("Unknown error"); 
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

void unlockSafebox() {
  servo.write(180); // Unlock the safebox

  // Display initialization message
  display.fillRect(21, 20, 89, 17, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(2);
  display.setCursor(32, 21);
  display.println("Unlock");
  display.display();
  delay(2000);
  display.fillRect(0, 38, 128, 17, SH110X_BLACK); 
  display.setCursor(16, 43);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.println("Press \"#\" to lock");
  display.display();
  // Send it to the server
  Blynk.virtualWrite(V5, String("Found a Finger ID:" + String(finger.fingerID)));   
  Blynk.logEvent("unlock");
}

void lockSafebox() {
  servo.write(360);  // Lock the safebox again
  
  // Display initialization message
  display.fillRect(21, 20, 89, 17, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(2);
  display.setCursor(45, 21);
  display.println("Lock");
  display.display();
  delay(2000);
  // Send it to the server
  Blynk.virtualWrite(V5, "Lock!");
  Blynk.logEvent("lock");
}

void Touch_handling()
{
  // key handling
  for ( int i = 0; i < touch_count; i++) {
    touches[i].update();
    if ( touches[i].rose() ) {

      Serial.print("Button:");
      if ( i <= 9 ) {
        Serial.println(i);
        {
          if(Key_num == 0)
          {
            display.fillRect(0, 38, 128, 17, SH110X_BLACK); 
          }
          display.setTextColor(SH110X_WHITE);
          display.setTextSize(2);
          if(Key_num < 6)
          {
            display.setCursor(32+Key_num*12, 38);
            display.print("*");
            Password[Key_num] = i;
            // Serial.print("PSW:");
            // Serial.println(Key_num);
            // Serial.println(Password[Key_num]);
            // Serial.println(Password_Flash[Key_num]);
            Key_num ++;            
          }  
          display.display();
        }
      }
      else if ( i == 10 ) {
        time_hold = millis();
        Serial.println("*");
        display.fillRect(0, 38, 128, 17, SH110X_BLACK);
        display.display();
        Key_num = 0;
        for(int i =0; i<6; i++)
        {
          Password[i] = 0;
        }      
      }
      else if ( i == 11 ) {
        Serial.println("#");
        display.fillRect(0, 38, 128, 17, SH110X_BLACK); 
        if((change_password_flag == 2)||(change_password_flag == 3))  //
        {
          if(Key_num == 6)
          {
            Key_num = 0;
            display.fillRect(0, 38, 128, 17, SH110X_BLACK); 
            display.setTextSize(1);
            display.setTextColor(SH110X_WHITE);
            if(change_password_flag == 2)
            {
              display.setCursor(6, 43);
              display.println("Please Enter Again");
              for(int i =0; i<6; i++)
              {
                Password_New[i]= Password[i];
              }
              change_password_flag = 3;
            }
            else
            {
              if(Password[0] == Password_New[0] && Password[1] == Password_New[1] && Password[2] == Password_New[2] && Password[3] == Password_New[3] && Password[4] == Password_New[4] && Password[5] == Password_New[5] )
              {
                display.setCursor(9, 43);
                display.println("New Password Saved");
                display.display();
                change_password_flag = 0;
                for(int i = 0; i<6; i++)
                {
                  //Wrire d NEW password to flash
                  EEPROM.write(i, Password[i]);
                  Password_Flash[i]= Password[i];
                }
                EEPROM.commit();
                delay(2000);
                display.fillRect(0, 38, 128, 17, SH110X_BLACK); 
              }
              else
              {
                display.println("Do Not Match");
                change_password_flag = 0;
              }
            }
            display.display();
          }
          else
          {
            display.fillRect(0, 38, 128, 17, SH110X_BLACK); 
            display.setCursor(1, 43);
            display.setTextSize(1);
            display.setTextColor(SH110X_WHITE);
            display.println("Only Allows 6 Digits");
            display.display();
            Key_num = 0;
            for(int i =0; i<6; i++)
            {
              Password[i] = 0;
            }
          }
        }
        else if(change_password_flag == 4)
        {
          display.fillRect(0, 38, 128, 26, SH110X_BLACK); 
          display.setCursor(12, 43);
          display.setTextSize(1);
          display.setTextColor(SH110X_WHITE);
          display.println("Enter New Password");
          display.setTextSize(1);
          display.setCursor(0, 56);
          display.println("*:Clear       #:Enter");
          display.display();
          change_password_flag =2;
        }
        else
        {
          display.setTextColor(SH110X_WHITE);
          display.setTextSize(2);
          if(lock_status == 0)
          {
            if(Password[0] == Password_Flash[0] && Password[1] == Password_Flash[1] && Password[2] == Password_Flash[2] && Password[3] == Password_Flash[3] && Password[4] == Password_Flash[4] && Password[5] == Password_Flash[5] )
            {
              Key_num = 0;
              for(int i =0; i<6; i++)
              {
                Password[i] = 0;
              }
              if(change_password_flag == 1)
              {
                display.setCursor(25, 38);
                display.println("correct");
                display.display();
                delay(1000);
                display.fillRect(0, 38, 128, 26, SH110X_BLACK); 
                display.setCursor(16, 43);
                display.setTextSize(1);
                display.setTextColor(SH110X_WHITE);
                display.println("Select to change");
                display.setTextColor(SH110X_WHITE);
                display.setTextSize(1);
                display.setCursor(0, 56);
                display.println("*:Finger   #:Password");
                display.display();
                change_password_flag =4;
              }
              else
              {
                display.setCursor(25, 38);
                display.println("correct");
                display.display();
                unlockSafebox();
                lock_status = 1;
              }
            }
            else if(Key_num == 0)
            {
              display.fillRect(0, 38, 128, 17, SH110X_BLACK); 
              display.setCursor(1, 43);
              display.setTextSize(1);
              display.setTextColor(SH110X_WHITE);
              display.println("Please enter password");
              display.display();
              delay(1000);
              display.fillRect(0, 38, 128, 17, SH110X_BLACK); 
              display.display();
            }
            else{
              display.setCursor(38, 38);
              display.println("Error");
              display.display(); 
              for(int i=200;i<=800;i++)              //用循环的方式将频率从200HZ 增加到800HZ
              {
                pinMode(29,OUTPUT);
                tone(29,i);                          //在四号端口输出频率
                delay(5);                            //该频率维持5毫秒   
              }
              delay(1000);
              noTone(29);
              for(int i =0; i<6; i++)
              {
                Password[i] = 0;
              }
              delay(2000);
              display.fillRect(0, 38, 128, 17, SH110X_BLACK); 
              display.display();
              lock_status = 0;
              Key_num = 0;
            }
          }
          else
          { 
            lock_status = 0;
            display.fillRect(0, 38, 128, 17, SH110X_BLACK); 
            lockSafebox();
          }
        }
      }
    }
    if ( touches[i].fell() ) {
     Serial.printf("Release:");
     if ( i <= 9 ) {
        Serial.println(i);
      }
      else if ( i == 10 ) 
      {
        Serial.println("*");
        if((millis()-time_hold)>500)
        {
            Serial.println("hold enough");
            display.fillRect(0, 38, 128, 17, SH110X_BLACK); 
            display.setCursor(1, 43);
            display.setTextSize(1);
            display.setTextColor(SH110X_WHITE);
            display.println("Please Enter Password");
            display.display();
            change_password_flag = 1;
            time_hold = 0;
        }
        else if(change_password_flag == 4)
        {
          display.fillRect(0, 38, 128, 26, SH110X_BLACK); 
          display.setCursor(3, 43);
          display.setTextSize(1);
          display.setTextColor(SH110X_WHITE);
          display.println("Put Finger to Sensor");
          display.setTextSize(1);
          display.setCursor(0, 56);
          display.println("*:Clear       #:Enter");
          display.display();
          while(!getFingerprintEnroll(1));
          change_password_flag =0;
        }
      }
      else if ( i == 11 ) {
        Serial.println("#");
      }
    }
  }
}

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (! Serial.available());
    num = Serial.parseInt();
  }
  return num;
}

uint8_t getFingerprintEnroll(uint8_t id) {
  
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }

  // OK success!
  
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");

  display.fillRect(0, 38, 128, 17, SH110X_BLACK); 
  display.setCursor(1, 43);
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.println("Move and Touch again");
  display.display();

  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      //Serial.print(".");
      break;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      break;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      break;
    default:
      Serial.println("Unknown error");
      break;
    }
  }
  // OK success!

  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}

void display_logo(uint16_t x,uint16_t y)
{  
  uint8_t cloud_pixel[5*8]=
  {
    0b00110010,0b01001001,0b01001001,0b01001001,0b00100110, // S
    0b00000010,0b00010101,0b00010101,0b00010101,0b00001111, // a
    0b00001000,0b00111111,0b01001000,0b01001000,0b00100000, // f
    0b00001110,0b00010101,0b00010101,0b00010101,0b00001100, // e
    0b00000000,0b00000000,0b00000000,0b00000000,0b00000000, // space
    0b01111111,0b01001001,0b01001001,0b01001001,0b00110110, // B
    0b00001110,0b00010001,0b00010001,0b00010001,0b00001110, // o
    0b00010001,0b00001010,0b00000100,0b00001010,0b00010001, // x
  };
  uint16_t _x = x - 40;
  uint16_t _y = y - 10;
  for(uint8_t i=0;i<8;i++)
  {
    for(uint8_t m=0;m<5;m++)
    {
      _y = y - 8;
      for(uint8_t n=0;n<8;n++)
      {
        if((cloud_pixel[i*5+m]>>(7-n))&0x01)
        {
            display.fillRect(_x+1, _y+1, 1, 1, SH110X_WHITE);            
        }
        _y += 2;
      }
      _x = _x + 2;
    }  
  }
  display.setTextSize(2);
  display.fillRect(21, 20, 89, 17, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setCursor(45, 21);
  display.println("Lock");
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.println("*:Clear       #:Enter");
  display.display();
}
