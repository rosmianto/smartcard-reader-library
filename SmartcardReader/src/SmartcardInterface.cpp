/*
	SmartcardInterface.cpp - Library for Xirka Silicon Tech. Smartcard Reader
	Created by Rosmianto A. Saputro, June 5, 2017.
*/

#include "Arduino.h"
#include "SmartcardInterface.h"
#include "SoftwareSerial.h"

#define BAUD_SC 10753 //2400

// Constructor:
CardInterface::CardInterface(int VCC, int RST, int RX, int TX) : card(RX, TX){
  _VCC = VCC;
  _RST = RST;
  _RX = RX;
  _TX = TX;
}

void CardInterface::begin(String ATR){
  _atr = ATR;
  _atr.replace(" ","");
  
  // Extract TD1 byte
  String tmp = _atr.substring(2, 4);
  tmp.toCharArray(_buf, 3);
  _td1 = (byte)strtol(_buf, 0, 16);
  
  // Obtain supported protocol:
  if((_td1 & 0x80)){
    // Very bad practice: Please refer to Smartcard Handbook how to detect
    // supported protocol (check TD1).
    protocol = SCARD_PROTOCOL_T1;
    #ifndef DEBUG
    Serial.println("T=1");
    #endif
  }
  else{
    protocol = SCARD_PROTOCOL_T0;
    #ifndef DEBUG
    Serial.println("T=0");
    #endif
  }
}

void CardInterface::init(){
  pinMode(_VCC, OUTPUT);
  pinMode(_RST, OUTPUT); pinMode(_TRG, INPUT);
  
  // Open serial communication and wait for port to open:
  #ifndef DEBUG
  Serial.begin(115200);
  while (!Serial);

  Serial.println("\n--------------------------");
  Serial.println("XST Smartcard Reader");
  Serial.println("--------------------------");
  #endif
  // set the data rate for the SoftwareSerial port
  card.begin(BAUD_SC);
  card.listen();
}

void CardInterface::activate_card(){
  // Activate the card
  digitalWrite(_VCC, HIGH);
  delayMicroseconds(5400);
  digitalWrite(_RST, HIGH);
}

void CardInterface::transmit_pps(){
  String resp;
  // Bad practice: Sanitation checking should be implemented. But this will
  // work anyway.
//  resp = transmit_raw("FF1113FD");  // PPS Request
//  Serial.println(resp);
//  resp = transmit_raw("00C101FEE0");  // IFS Request at 0xFE length.
}

String CardInterface::transmitAPDU(String apdu){
  apdu.replace(" ","");
  if(protocol == SCARD_PROTOCOL_T0)
    return transmitAPDU_T0(apdu);
  else
    return transmitAPDU_T1(apdu);
}

String CardInterface::transmitAPDU_T0(String apdu){
  String sub;
  char buf[3];

  card.stopListening();
//  card.ignore();
  for(int i = 0; i < 10; i += 2){
    sub = apdu.substring(i, i+2);
    sub.toCharArray(buf, 3);
    byte b = (byte)strtol(buf, 0, 16);
    card.write_8E2(b);
  }
  card.listen();
  String response1 = read_response();
  if(response1.substring(0, 1) != "6" && response1.substring(0, 1) != "9")
    response1.remove(0, 2);

  int _size = apdu.length();
  if(_size <= 10)
    return response1;
  else
  {
    card.stopListening();
//    card.ignore();
    for(int i = 10; i < _size; i += 2){
      sub = apdu.substring(i, i+2);
      sub.toCharArray(buf, 3);
      byte b = (byte)strtol(buf, 0, 16);
      card.write_8E2(b);
  }    
    card.listen();
    String response2 = read_response();

    return response2;
  }
}

/* transmitAPDU_T1() sends T=1 block frame to smartcard.
 *  This function appends the APDU command with Prologue and Epilogue field.
 *  Response Block Frame from smartcard checked with LRC.
*/
String CardInterface::transmitAPDU_T1(String apdu){
  int byte_length = apdu.length() / 2;
  String sub, response;
  byte buf[3];
  byte NAD, PCB, LEN, LRC;  
  byte apdu_byte[byte_length];

  card.stopListening();
//  card.ignore();

  // Convert APDU (hex string) to APDU (byte array)
  for(int i = 0; i < apdu.length(); i += 2){
    sub = apdu.substring(i, i+2);
    sub.toCharArray(buf, 3);
    apdu_byte[i / 2] = (byte)strtol(buf, 0, 16);
  }

  // Construct T=1 Prologue field
  NAD = 0x00;
  _sequence_number ^= 0x40;
  PCB = _sequence_number;
  LEN = byte_length;

  // Calculate LRC
  LRC = 0x00;
  for(int i = 0; i < byte_length; i++)
    LRC ^= apdu_byte[i];
  LRC = LRC ^ NAD ^ PCB ^ LEN;

  // Send Block frame to Smartcard
  card.write_8E2(NAD);
  card.write_8E2(PCB);
  card.write_8E2(LEN);
  
  for(int i = 0; i < byte_length; i++)
    card.write_8E2(apdu_byte[i]);
  card.write_8E2(LRC);
    
  // Receive APDU response from smartcard
  card.listen();
  response = read_response();
  response.replace(" ","");
  response.toUpperCase();
  
  int response_byte_length = response.length() / 2;
  byte apdu_response_byte[response_byte_length];

  // Convert APDU response (hex string) to (byte array)
  for(int i = 0; i < response.length(); i += 2){
    sub = response.substring(i, i+2);
    sub.toCharArray(buf, 3);
    apdu_response_byte[i / 2] = (byte)strtol(buf, 0, 16);
  }

  // Calculate LRC and compare...
  LRC = 0x00; // Bad practice: LRC is shared with another part.
  for(int i = 0; i < (response_byte_length - 1); i++)
    LRC ^= apdu_response_byte[i];

  // Compare:
  if(LRC == apdu_response_byte[response_byte_length - 1])
    return response.substring(6, response.length() - 2);
  else
    return "";
}

String CardInterface::read_response(){
  String result = "";
  pinMode(_TX, INPUT_PULLUP);  // Prevent signal collision.
  unsigned long first = millis();
  while(millis() - first < 100){  // If there's no incoming data for 100ms. Break.
    if(card.available()){
      first = millis();
      byte c = card.read();
      if(c < 0x10) result += '0';
      result += String(c, HEX) + ' ';
    }
  }
  pinMode(_TX, OUTPUT);
  if(result.length() < 2)
    Serial.println("No responses.");
  return result;
}

/* transmit_raw() sends raw data to smartcard. This function is used
to send PPS Request and IFS Request.
*/
String CardInterface::transmit_raw(String raw){
  String sub;
  char buf[3];

  card.stopListening();
//  card.ignore();
  for(int i = 0; i < raw.length(); i += 2){
    sub = raw.substring(i, i+2);
    sub.toCharArray(buf, 3);
    byte b = (byte)strtol(buf, 0, 16);
    card.write_8E2(b);
  }
  card.listen();
  return read_response();
}
