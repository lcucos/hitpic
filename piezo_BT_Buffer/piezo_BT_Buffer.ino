//#include <EEPROM.h>

#include <SoftwareSerial.h>   //Software Serial Port

// Sample code that reads data from a piezo sensor and identifies when tapped
// Please note this code is provided "as is" with no express or implied warranty of any kind about the completeness, accuracy, reliability, suitability
// 
// 
// The function of the code (the mode of operation) can change based on a 
// parameter that can be send at runtime
// 
// Operation modes:
//  - record and immediately display all raw data around the moment of impact
//  - record only the impact time
//  - dump all recorded impact times
// 
// 
// This code was written for a tinylily board
// Please adjust the I/O pins as needed

#define RxD 2
#define TxD 3
 
#define RECORD_TIMESTAMP   't'   // record timestamps when the hit happen
#define RECORD_HIT_DATA    'h'   // send all raw data after each hit
#define UPLOAD_TIMESTAMP   'u'   // send all recorded hits
#define STOP_ALL           's'   // 

int pinSensor=A4; 

char mode = ' ';

//===================== common processing ======================
#define BUFFER_SIZE      500

struct SensVals{
  byte ax;         // max value read at peak
  byte timeStamp; // at 100ms resolution
};

int circ_buff_crtpos = -1;

boolean bUseBlueTooth = true;//false;
unsigned long lastTime = 0;
unsigned long startTime = 0;

int totHits = 0;

SensVals circ_buff_storage[BUFFER_SIZE];

SoftwareSerial blueToothSerial(RxD,TxD);

#define D(x)    Serial.println(x)
//#define D(x) 
#define BT_D(x) {blueToothSerial.println(x);blueToothSerial.flush();}

//===================================================================
void storeData(int dX, byte timeDiffFromLast){
   circ_buff_crtpos++;
   if(circ_buff_crtpos == BUFFER_SIZE){
       circ_buff_crtpos=0;
   }
   circ_buff_storage[circ_buff_crtpos].ax = (byte)dX;
   circ_buff_storage[circ_buff_crtpos].timeStamp = timeDiffFromLast;
}

void printStrObj(String obj){
  D(obj);
  BT_D(obj);
}

void streamData(int index){
    String outStr = String(circ_buff_storage[index].timeStamp);
    outStr += ":";
    outStr += circ_buff_storage[index].ax;
    //outStr += "\n";
    printStrObj(outStr);
}

void dumpHitData(unsigned long hit_time, unsigned long duration){  
     String header=String("StartAt=");
     header+=hit_time;
     header+=", Duration(ms)=";
     header+=duration; 
     header+="\n";
     printStrObj(header);
     
     for(int i=0;i<BUFFER_SIZE;i++){
       streamData(i);
     }
}  

int getHitPic(int maxVal){
   unsigned long hit_time = millis();
   for(int i =0;i < BUFFER_SIZE;i++){
      int val=analogRead(pinSensor);
      if(mode==RECORD_HIT_DATA){
        circ_buff_storage[i].ax=(byte)val;
        circ_buff_storage[i].timeStamp=0;
      }
      if(val > maxVal){
        maxVal=val;
      }
   }
   unsigned long stopLogtime = millis();
   unsigned long diffTime = (hit_time - lastTime)/100 ; // store time up to 0.1 sec resolution
   // can't be closer than the time resolution
   if(diffTime==0){
       return 0 ;
   }
   totHits++;
   String hitInfo="<Hts:";
   hitInfo+=totHits;
   hitInfo+=">";
   hitInfo+=diffTime;
   hitInfo+=":";
   hitInfo+=maxVal;
   hitInfo+=" time=";
   hitInfo+=hit_time;
   hitInfo+="";
   printStrObj(hitInfo);

   if(mode == RECORD_HIT_DATA){
      dumpHitData(hit_time, stopLogtime - hit_time);
   }else if(mode==RECORD_TIMESTAMP){
      // assume the time is less than two hours between consecutive hits
      while(diffTime > 255){
         storeData(0, 255);
         diffTime = diffTime - 255;
      }
      storeData(maxVal, diffTime);
   }
   lastTime = hit_time;
   return maxVal;
}

//===================================================================================
void processRequest(char var){    
    //String tmp = String("read: ");
    //tmp+=(int)var;
    //printStrObj(tmp);
      
    if(var==RECORD_TIMESTAMP){
      String info=String("Set mode: record timestamp, HitsCapacity=");
      info+=BUFFER_SIZE;
      printStrObj(info);
      mode = var;
      circ_buff_crtpos = -1;
      totHits=0;
      startTime = millis();
      lastTime = 0;
    }else if(var == RECORD_HIT_DATA){
      mode = var;
      printStrObj("Set mode: Dump raw hit data");
      circ_buff_crtpos=-1;
    }else if(var == UPLOAD_TIMESTAMP){
      dumpTimeStamp();
    }else if(var == STOP_ALL){
      mode = STOP_ALL;
    }
}

void dumpTimeStamp(){
    unsigned long crtTime = millis();
    printStrObj("TimeDifferences:");
    // dump timestamps in reversed order but fill in the timestamps
    int nShow=0;
    unsigned long tmpTime = lastTime;
    for(int i=totHits-1; nShow<totHits && i >= 0 ;i--){
      nShow++;
      int index = i%BUFFER_SIZE;
      String outStr = String(tmpTime);
      outStr += ":";
      outStr += circ_buff_storage[index].timeStamp;
      outStr += ":";
      outStr += circ_buff_storage[index].ax;
      printStrObj(outStr);
      tmpTime = tmpTime - circ_buff_storage[index].timeStamp*100;
      //streamData(i%BUFFER_SIZE);
    }
    // dump points cronologically
    /*
    int nStart = 0;
    int nLimit = totHits;
    if(totHits >= BUFFER_SIZE){
      nStart = (circ_buff_crtpos + 1) % BUFFER_SIZE;
      nLimit = BUFFER_SIZE;
    }
    int count=0;
    for(int i=nStart; i<nLimit; i++, count++){
      streamData(i%BUFFER_SIZE);
    }
    */
    
    String header = String("StartTime=");
    header+=startTime;
    header+=", CrtTime=";
    header+=crtTime;
    unsigned long diffTime = (startTime - lastTime)/100 ;
    header+=", TimeDiff=";
    header+=diffTime;
    header+=", TotHits=";    
    header+=totHits;
    printStrObj(header);
}

//===================================================================================
//================== common ==================================
void setupBlueToothConnection(boolean bForce)
{
    if(bUseBlueTooth && bForce==false){
      return;
    }
    bUseBlueTooth = true;
    blueToothSerial.begin(19200);//4800);//9600);//19200); //tiny-lily works at 19200, nano at 9600
    blueToothSerial.print("\r\n+STWMOD=0\r\n"); //set the bluetooth work in slave mode
    blueToothSerial.print("\r\n+STNA=SeeedBTSlave\r\n"); //set the bluetooth name as "SeeedBTSlave"
    blueToothSerial.print("\r\n+STOAUT=1\r\n"); // Permit Paired device to connect me
    blueToothSerial.print("\r\n+STAUTO=0\r\n"); // Auto-connection should be forbidden here
    blueToothSerial.print("\r\n+STPIN=0000\r\n");
    delay(2000); // This delay is required.
    blueToothSerial.print("\r\n+INQ=1\r\n"); //make the slave bluetooth inquirable 
    D("The slave bluetooth is inquirable!");
    delay(2000); // This delay is required.
    blueToothSerial.println("Bluetooth Started");
    blueToothSerial.flush();
}
void setup()
{  
  Serial.begin(9600);

  pinMode(RxD, INPUT);
  pinMode(TxD, OUTPUT);
  //if(bUseBlueTooth)
  {
    setupBlueToothConnection(true);
  }  
  processRequest(RECORD_TIMESTAMP);
}


void loop()
{
    if(Serial.available()>=1){processRequest(Serial.read());}
    //printStrObj("abc");
    if(bUseBlueTooth== true && blueToothSerial.available()){
      processRequest(blueToothSerial.read());
    }    
    int val=analogRead(pinSensor);
    
    
    if(val < 90){
      return;
    }

   int hitVal = getHitPic(val);
}

