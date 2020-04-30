
#define SDAPIN      6 // (PB1)
#define SCLPIN      7 // (PB0)
#define STATUSLED   8 // (PA1)
#define STATUSPIN   2 // (PA6)

#include <Wire.h>

// Global variables
bool firstResponse = false;
bool secondResponse = false;

// Static function definitions
void receiveEvent(int howMany);


void receiveEvent(int howMany) {
    
  (void) howMany;
  while (3 < Wire.available()) { // loop through all but the last 3
    byte c = Wire.read();
  }
  byte received[3];
  received[0] = Wire.read(); 
  received[1] = Wire.read();
  received[2] = Wire.read();

  if (received[0] == 0x00 && received[1] == 0x00 && received[2] == 0xBE) {
    firstResponse = true;
  } 
  else if (received[0] == 0x60 && received[1] == 0x00 && received[2] == 0x4E) {
    secondResponse = true;
  } 
  else {
    //unknown message
  }

}


void setup() {
  
  pinMode(STATUSLED, OUTPUT);
  digitalWrite(STATUSLED, LOW);
  pinMode(STATUSPIN, OUTPUT);
  digitalWrite(STATUSPIN, HIGH);
     
  Wire.begin(77, true);
  Wire.onReceive(receiveEvent);
  
}

// Main()
void loop() {
  
  //unsigned long currentMillis = millis();

  if (firstResponse) {
    firstResponse = false;
    
    while (digitalRead(SCLPIN) == LOW) {
    }
    
    Wire.end();
    
    delay(10);

    //Write response as to itho fan i2c master
    Wire.begin();
    
    Wire.beginTransmission(byte(0x41));
    Wire.write(byte(0xEF));
    Wire.write(byte(0xC0));
    Wire.write(byte(0x00));
    Wire.write(byte(0x01));
    Wire.write(byte(0x06));
    Wire.write(byte(0x00));
    Wire.write(byte(0x00));
    Wire.write(byte(0x09));
    Wire.write(byte(0x00));
    Wire.write(byte(0x09));
    Wire.write(byte(0x00));
    Wire.write(byte(0xB6));
    Wire.endTransmission(true);

    Wire.end();

    //Switch to i2c slave again and wait for the reply
    Wire.begin(77, 1);
    Wire.onReceive(receiveEvent);
      
  }
  if (secondResponse) {
    secondResponse = false;
    
    //disable i2c
    Wire.end();

    //put i2c pins in high-impedance state
    pinMode(SDAPIN, INPUT);
    pinMode(SCLPIN, INPUT);
    
    //set status
    digitalWrite(STATUSLED, HIGH);//switch status led off
    digitalWrite(STATUSPIN, LOW); //signal Wemos that init has finished successful

  }

}
