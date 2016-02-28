#include <rn2483.h>

/*
 * Author: JP Meijers
 * Date: 2016-02-07
 * 
 * Transmit a one byte packet via TTN. This happens as fast as possible, while still keeping to 
 * the 1% duty cycle rules enforced by the RN2483's built in LoRaWAN stack. Even though this is 
 * allowed by the radio regulations of the 868MHz band, the fair use policy of TTN may prohibit this.
 * 
 * CHECK THE RULES BEFORE USING THIS PROGRAM!
 * 
 * CHANGE ADDRESS!
 * Change the address this device should use to either a testing address or one from a range you "own".
 * The appropriate line is "myLora.initTTN("XXXXXX");
 * 
 * Connect the RN2483 as follows:
 * RN2483 -- Arduino
 * Uart TX -- 10
 * Uart RX -- 11
 * Reset -- 12
 * Vcc -- 3.3V
 * Gnd -- Gnd
 * 
 * If you use an Arduino with a free hardware serial port, you can replace 
 * the line "rn2483 myLora(mySerial);"
 * with     "rn2483 myLora(SerialX);"
 * where the parameter is the serial port the RN2483 is connected to.
 * Remember that the serial port should be initialised before calling initTTN().
 * For best performance the serial port should be set to 57600 baud, which is impossible with a software serial port.
 * If you use 57600 baud, you can remove the line "myLora.autobaud();".
 * 
 * Modified and improved, to a working prototype for Hackaton in Groningen, by Kibet Kipkemoi
 */
#include <SoftwareSerial.h>
#include <Timer.h>

SoftwareSerial mySerial(10, 11); // RX, TX

String str;
unsigned long time = 0;

//create an instance of the rn2483 library, 
//giving the software UART as stream to use,
//and using LoRa WAN
rn2483 myLora(mySerial);

#define MSG_LED 13
#define TV_LED 10
#define RN2483_RESET 12
#define ON 1
#define OFF 0
#define POWER 8
#define TELEVISION 2 
#define UPDATE_TIME_NODE 10000
#define ONE_SECOND 1000

Timer t;

bool newData = false;
bool secondPassed = false;
bool minutePassed = false;
bool powerON = true;
unsigned int credits = 250; //half a minute of power

void checkTime(void){
  static byte seconds = 0;
  static byte minutes = 0;
  
  secondPassed = true;
  if(seconds >= 60) {
    minutePassed = true;
    seconds = 0;
    minutes++;
    if(minutes >= 5){
      newData = true;
      minutes = 0;  
    }
  }
  
  seconds++;
}

byte * checkPowerUsage(void){
  static byte powerUsage[5] = {0};
  static byte currentMinute = 0;
  
  if(secondPassed){
    secondPassed = false;
    if(minutePassed){
      minutePassed = false;
      currentMinute++;
      for(byte i = 4; i > 0; i--){
        powerUsage[i] = powerUsage[i-1]; // shift to the right (new to old)
      }
      powerUsage[0] = 0;
      if(currentMinute >= 5) currentMinute = 0;
    }
    if(digitalRead(TELEVISION)){
      digitalWrite(TV_LED, HIGH); // turn TV on
      powerUsage[0]++;
      if(powerON) credits--;
      if(!credits) powerON = false; 
    }
    else{
      digitalWrite(TV_LED, LOW); // turn TV off
    } 
  }
  return powerUsage;
}

//void checkNewData(){
// newData = true;
//}

void transmitData(byte *powerUsage){
  static char outputBuffer[25] = {4}; // Characters used: package ID can go to 9999, powerStatus is 2, powerUsage is 3 for each minute (5) 
  static unsigned int packageID = 0;
  static byte powerStatus = ON;

  digitalWrite(MSG_LED, HIGH);
  sprintf(outputBuffer,"%d,%d,%d,%d,%d,%d,%d,%d", packageID, powerON, credits, *(powerUsage),*(powerUsage+1),*(powerUsage+2),*(powerUsage+3),*(powerUsage+4));
  //sprintf(outputBuffer,"%d,%d,%d,%d,%d,%d,%d,%d", packageID, powerON, credits, *(powerUsage),*(powerUsage+1),*(powerUsage+2),*(powerUsage+3),*(powerUsage+4));
  myLora.tx(outputBuffer); //one byte, blocking function
  packageID++;
  digitalWrite(MSG_LED, LOW);
}

void smartMeter(void){
  byte *powerUsage = 0;
//  char testBuffer[10] = {0};
  for(;;){
    if(newData){
      transmitData(powerUsage);
      newData = false;
    }
    digitalWrite(POWER,powerON);
    powerUsage = checkPowerUsage();
    t.update();
  }
}

// the setup routine runs once when you press reset:
void setup() {
  //output LED pin
  pinMode(MSG_LED, OUTPUT);
  pinMode(TV_LED, OUTPUT);
  pinMode(POWER, OUTPUT);
  pinMode(TELEVISION, OUTPUT);
  
  // Open serial communications and wait for port to open:
  Serial.begin(57600);
  mySerial.begin(9600);
  Serial.println("There has been an awakening");

  //reset rn2483
  pinMode(RN2483_RESET, OUTPUT);
  digitalWrite(RN2483_RESET, LOW);
  delay(500);
  digitalWrite(RN2483_RESET, HIGH);
  digitalWrite(POWER,ON);

 // int tickEvent = t.every(UPDATE_TIME_NODE, checkNewData);
  int tickTimePassed = t.every(33, checkTime); // 100 = half a minute
  //initialise the rn2483 module
 // myLora.autobaud(); // *** Kibet - Comment this one out or else program doesn't work properly
  myLora.initTTN("02D1E514");

  //transmit a startup message
  myLora.txUncnf("Jambo Dunia");

  delay(2000);
  smartMeter();
}

void loop() {} // Not used in this code, has to be here though
