
#include <SoftwareSerial.h>   //Software Serial Port

#define SAMPLE_SIZE         180
#define BEFORE_SIZE          20
#define BUFFER_SIZE      (SAMPLE_SIZE+BEFORE_SIZE)

#define RxD 6
#define TxD 7
 
#define DEBUG_ENABLED  1

struct SensVals{
  int ax;
  int ay;
  int az;
};

int sleepPin= 4; // Turning sleep high turns on the Accelerometer
int sixGPin = 3; // set this pin to high for 6G

int xpin = A0;
int ypin = A1;
int zpin = A2;

int prevX=0;
int prevY=0;
int prevZ=0;

int dH=50;
int dL=-50;

int circ_buff_crtpos = -1;
int hit_stamp = -1;
int after_hit_samples = 1;

unsigned long lastTime   = 0;
unsigned long hit_time = 0; // when the event was first recorded

SensVals circ_buff_storage[BUFFER_SIZE];

SoftwareSerial blueToothSerial(RxD,TxD);


void setup()
{
  
  Serial.begin(9600);
  
  pinMode(sleepPin,OUTPUT);
  digitalWrite(sleepPin, HIGH);//turns off sleep mode and activates device

  pinMode(sixGPin,OUTPUT);
  digitalWrite(sixGPin, HIGH);//turns off sleep mode and activates device
  
  pinMode(xpin, INPUT);//input mode
  digitalWrite(xpin, HIGH);//turn on pull up resistor
  
  pinMode(ypin, INPUT);//input mode
  digitalWrite(ypin, HIGH);//turn on pull up resistor
  
  pinMode(zpin, INPUT);//input mode
  digitalWrite(zpin, HIGH);//turn on pull up resistor
    
  pinMode(RxD, INPUT);
  pinMode(TxD, OUTPUT);
  setupBlueToothConnection();
}

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

void loop()
{
  int newX=analogRead(xpin);
  int newY=analogRead(ypin);
  int newZ=analogRead(zpin);
  
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
    /*
    unsigned long time = millis();
    if(time - lastTime > 250){
      //stremData(dX, dY, dZ, time);
      lastTime = time;
    }
    */
  }
}
