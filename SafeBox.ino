/* Fill-in information from Blynk Device Info here */
#define BLYNK_TEMPLATE_ID           "TMPL26_JX2Nd5"
#define BLYNK_TEMPLATE_NAME         "Quickstart Template"
#define BLYNK_AUTH_TOKEN            "5S0f6Q-4H4zOmZOhdnmd7uYzxC0onpmX"

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial
#include <SPI.h>
#include <Ethernet.h>
#include <BlynkSimpleEthernet.h>
#include <SoftwareSerial.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Fingerprint.h>
#include <Servo.h>

BlynkTimer timer;

#define FINGERPRINT_RX_PIN 27
#define FINGERPRINT_TX_PIN 26

SoftwareSerial fingerprintSerial(FINGERPRINT_RX_PIN, FINGERPRINT_TX_PIN); // Declare SoftwareSerial obj first
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&fingerprintSerial);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
// Initialize components
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define SERVO_PIN 22
Servo servo;

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
  
  // Initialize components
  Serial.begin(9600);
  fingerprintSerial.begin(57600);
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  
  // Display initialization message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(24, 0);
  display.println("Safebox");
  display.setCursor(48, 16);
  display.println("Lock");
  display.display();
  delay(2000);
  
  Ethernet.init(17);  // WIZnet W5100S-EVB-Pico
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (true) {
      delay(1); // do nothing, no point running without Ethernet hardware
    }
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  Blynk.begin(BLYNK_AUTH_TOKEN);
    
  finger.getTemplateCount();

  if (finger.templateCount == 0) {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else {
    Serial.println("Waiting for valid finger...");
      Serial.print("Sensor contains "); Serial.print(finger.templateCount); Serial.println(" templates");
  } 
}

void loop() {
  
  Blynk.run();
  timer.run();

  getFingerprintID();
  delay(50);            //don't ned to run this at full speed.
  
  // Your main logic here
  
  // Check for fingerprint input
  if (fingerprintSerial.available()) {
    // Read fingerprint data and process
    // Add your fingerprint recognition logic here
    // Example: if (recognizedFingerprint) { unlockSafebox(); }
  }
  
  // Check for touch button input
  // Add your touch button logic here
  
  // Display status on OLED
  // Add OLED display logic here
}

uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
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
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
    unlockSafebox(); 
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    
  // Display initialization message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(24, 0);
  display.println("Safebox");
  display.setCursor(32, 16);
  display.println("ALARM");
  // Send it to the server
  Blynk.virtualWrite(V5, "ALARM!");
  display.display();  
    
    for(int i=200;i<=800;i++)              //用循环的方式将频率从200HZ 增加到800HZ
    {
    pinMode(6,OUTPUT);
    tone(6,i);                             //在四号端口输出频率
    delay(5);                              //该频率维持5毫秒   
    }
    delay(4000);  
    noTone(6);

  // Display initialization message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(24, 0);
  display.println("Safebox");
  display.setCursor(48, 16);
  display.println("lOCK");
  display.display();
  Blynk.virtualWrite(V5, "Lock!");
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
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(24, 0);
  display.println("Safebox");
  display.setCursor(32, 16);
  display.println("Unlock");
  display.display();

  // Send it to the server
  Blynk.virtualWrite(V5, String("Found a Finger ID:" + String(finger.fingerID)));   
  
  delay(2000);     // Keep it unlocked for 2 seconds
  servo.write(360);  // Lock the safebox again
  
  // Display initialization message
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(24, 0);
  display.println("Safebox");
  display.setCursor(48, 16);
  display.println("Lock");
  display.display();
  delay(2000);
  // Send it to the server
  Blynk.virtualWrite(V5, "Lock!");
}
