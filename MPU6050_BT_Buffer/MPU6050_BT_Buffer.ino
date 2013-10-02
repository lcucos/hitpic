// I2C device class (I2Cdev) demonstration Arduino sketch for MPU6050 class
// 10/7/2011 by Jeff Rowberg <jeff@rowberg.net>
// Updates should (hopefully) always be available at https://github.com/jrowberg/i2cdevlib
//
// Changelog:
//     2011-10-07 - initial release
// 
// LC: Add bluetooth

/* ============================================
I2Cdev device library code is placed under the MIT license
Copyright (c) 2011 Jeff Rowberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
===============================================
*/

// Arduino Wire library is required if I2Cdev I2CDEV_ARDUINO_WIRE implementation
// is used in I2Cdev.h
#include "Wire.h"
#include <SoftwareSerial.h>   //Software Serial Port

#define RxD 6
#define TxD 7
 
 
// I2Cdev and MPU6050 must be installed as libraries, or else the .cpp/.h files
// for both classes must be in the include path of your project
#include "I2Cdev.h"
#include "MPU6050.h"

// class default I2C address is 0x68
// specific I2C addresses may be passed as a parameter here
// AD0 low = 0x68 (default for InvenSense evaluation board)
// AD0 high = 0x69
MPU6050 accelgyro;

int16_t ax, ay, az;
int16_t gx, gy, gz;

#define LED_PIN 13
bool blinkState = false;

//===================== common processing ======================
#define SAMPLE_SIZE         180
#define BEFORE_SIZE          20
#define BUFFER_SIZE      (SAMPLE_SIZE+BEFORE_SIZE)

int prevX=0;
int prevY=0;
int prevZ=0;

int dH=50;
int dL=-50;

struct SensVals{
  int ax;
  int ay;
  int az;
};

int circ_buff_crtpos = -1;
int hit_stamp = -1;
int after_hit_samples = 1;

unsigned long lastTime   = 0;
unsigned long hit_time = 0; // when the event was first recorded

SensVals circ_buff_storage[BUFFER_SIZE];
SoftwareSerial blueToothSerial(RxD,TxD);
//===================================================================


void setup() {
    // join I2C bus (I2Cdev library doesn't do this automatically)
    Wire.begin();

    // initialize serial communication
    // (38400 chosen because it works as well at 8MHz as it does at 16MHz, but
    // it's really up to you depending on your project)
    Serial.begin(9600);

    // initialize device
    Serial.println("Initializing I2C devices...");
    accelgyro.initialize();

    // verify connection
    Serial.println("Testing device connections...");
    Serial.println(accelgyro.testConnection() ? "MPU6050 connection successful" : "MPU6050 connection failed");

    // configure Arduino LED for
    pinMode(LED_PIN, OUTPUT);
    
        
  pinMode(RxD, INPUT);
  pinMode(TxD, OUTPUT);
  setupBlueToothConnection();
}

//================== common ==================================
void setupBlueToothConnection()
{
  blueToothSerial.begin(9600); //Set BluetoothBee BaudRate to default baud rate 38400
  blueToothSerial.print("\r\n+STWMOD=0\r\n"); //set the bluetooth work in slave mode
  blueToothSerial.print("\r\n+STNA=SeeedBTSlave\r\n"); //set the bluetooth name as "SeeedBTSlave"
  blueToothSerial.print("\r\n+STOAUT=1\r\n"); // Permit Paired device to connect me
  blueToothSerial.print("\r\n+STAUTO=0\r\n"); // Auto-connection should be forbidden here
  blueToothSerial.print("\r\n+STPIN=0000\r\n");
  delay(2000); // This delay is required.
  blueToothSerial.print("\r\n+INQ=1\r\n"); //make the slave bluetooth inquirable 
  Serial.println("The slave bluetooth is inquirable!");
  delay(2000); // This delay is required.
  blueToothSerial.println("Bluetooth Started");
  blueToothSerial.flush();
}
void streamData(int pos, int index){
      String outStr = String(pos);
      outStr += ",";
      outStr += circ_buff_storage[index].ax;
      outStr += ",";    
      outStr += circ_buff_storage[index].ay;
      outStr += ","; 
      outStr += circ_buff_storage[index].az;
      outStr += "\n";
      Serial.print(outStr);
      blueToothSerial.print(outStr);
}

void streemHitData(){
     unsigned long stop_time=millis();
     
     // we go back in time SAMPLE_SIZE samples before the hit and SAMPLE_SIZE after the hit
     int startStamp = (hit_stamp + SAMPLE_SIZE+1) % BUFFER_SIZE;
     String header=String("StartAt=");
     header+=hit_time;
     header+=", Duration=";
     header+=(int)(stop_time-hit_time); 
     header+=", HitIndex=";
     header+=(BEFORE_SIZE-1);
     header+="\n";
     Serial.print(header);
     blueToothSerial.print(header);
   
     for(int i=0;i<BUFFER_SIZE;i++){
        int pos = (i + startStamp) % BUFFER_SIZE;
        streamData(i, pos);
     }
}  

void storeData(int dX, int dY, int dZ){
   circ_buff_crtpos++;
   if(circ_buff_crtpos == BUFFER_SIZE){
       circ_buff_crtpos=0;
   }
   circ_buff_storage[circ_buff_crtpos].ax = dX;
   circ_buff_storage[circ_buff_crtpos].ay = dY;
   circ_buff_storage[circ_buff_crtpos].az = dZ;
}

void loop() {
    // read raw accel/gyro measurements from device
    accelgyro.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // these methods (and a few others) are also available
    //accelgyro.getAcceleration(&ax, &ay, &az);
    //accelgyro.getRotation(&gx, &gy, &gz);
/*
    // display tab-separated accel/gyro x/y/z values
    Serial.print("a/g:\t");
    Serial.print(ax); Serial.print("\t");
    Serial.print(ay); Serial.print("\t");
    Serial.print(az); Serial.print("\t");
    Serial.print(gx); Serial.print("\t");
    Serial.print(gy); Serial.print("\t");
    Serial.println(gz);
    String dataStr = String("acc");
    dataStr+=ax;
    dataStr+="\n";
    blueToothSerial.print(dataStr);
*/
    int newX=ax;
    int newY=ay;
    int newZ=az;
    int dX = prevX-newX;
    int dY = prevY-newY;
    int dZ = prevZ-newZ;
    prevX=newX;
    prevY=newY;
    prevZ=newZ;
    storeData(newX, newY, newZ);
    // see if is time to send the data
    if(hit_stamp >= 0 && after_hit_samples++ >= SAMPLE_SIZE){
       // save data
       streemHitData();
       hit_stamp=-1;
    }
    if( dX>dH || dX < dL || dY>dH || dY < dL || dZ>dH || dZ < dL) {
      if(hit_stamp == -1){
         hit_stamp = circ_buff_crtpos;
         hit_time=millis();
         after_hit_samples=1;
      }
    }
    
    // blink LED to indicate activity
    //blinkState = !blinkState;
    //digitalWrite(LED_PIN, blinkState);
}
