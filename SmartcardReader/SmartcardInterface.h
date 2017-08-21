/*
	SmartcardInterface.h - Library for Xirka Silicon Tech. Bluetooth Reader
	Created by Rosmianto A. Saputro, June 5, 2017.
*/

#ifndef smartcard_interface_h
#define smartcard_interface_h

#include "Arduino.h"
#include "src/SoftwareSerial/SoftwareSerial.h"

class CardInterface
{
  public:
    CardInterface(int VCC, int RST, int RX, int TX, int TRG);
    void begin(String ATR);
    void peripheral_init();
    void activate_card();
    String transmitAPDU(String apdu);
    String transmitAPDU_T0(String apdu);
    String transmitAPDU_T1(String apdu);
    void init_card();
    String read_response();
    String transmit_raw(String raw);    
    int protocol;
        
  private:
    byte _buf[3];
    byte _td1;
    byte _sequence_number = 0x40;
    String _atr;
    const int SCARD_PROTOCOL_T0 = 0;
    const int SCARD_PROTOCOL_T1 = 1;
    int _VCC, _RST, _RX, _TX, _TRG;
    SoftwareSerial card;
};

#endif
