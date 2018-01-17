#include "src/SoftwareSerial.h"
#include "src/SmartcardInterface.h"

// Comment-out line below to disable debugging via Serial Monitor.
// #define DEBUG

// Smartcard pins:
#define VCC 13
#define RST 12
#define RX 11
#define TX 10
#define TRG 2
#define CLK 9

CardInterface interface_card(VCC, RST, RX, TX);

// Variable declaration:
String cmd = "";
String result = "";
volatile boolean stat = false;

void setup(){
  // Generate 4 MHz clock on Arduino UNO Pin 9.
  TCNT1 = 0;
  TCCR1B = B00001001;
  TCCR1A = B01000000;
  OCR1A = 1; // CLK frequency = 4 MHz
  pinMode(CLK, OUTPUT);
  
  interface_card.init();
  interface_card.activate_card();

  String ATR = interface_card.read_response();
  Serial.println("ATR: " + ATR);
  interface_card.begin(ATR);
  delay(100);
  interface_card.transmit_pps();
  
  Serial.println("READY.");

  delay(1000);
  
  /* Example how to transmit APDU command: */
  Serial.print("00 A4 00 00 02 3F 00 -> ");
  result = interface_card.transmitAPDU("00 A4 00 00 02 3F 00");
  Serial.println(result);
}

void loop() {
  
}
