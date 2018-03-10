/*
  Example for receiving
  
  https://github.com/sui77/rc-switch/
  
  If you want to visualize a telegram copy the raw data and 
  paste it into http://test.sui.li/oszi/
*/

#include <RCSwitch.h>

RCSwitch mySwitch = RCSwitch();
RCSwitch sendSwitch = RCSwitch();

void setup() {
  Serial.begin(9600);
  mySwitch.enableReceive(D3);  // Receiver on interrupt 0 => that is pin #2

  sendSwitch.enableTransmit(D5);
}

void loop() {
  if (mySwitch.available()) {
    output(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(), mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(),mySwitch.getReceivedProtocol());
    mySwitch.resetAvailable();
  }
//  sendSwitch.send(3962048, 24);
//  delay(2000);
}
