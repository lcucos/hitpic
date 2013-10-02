#include "Arduino.h"
#include "BaseCollect.h"

BaseCollect::BaseCollect(){
	circ_buff_crtpos = -1;
	hit_stamp = -1;
	after_hit_samples = 1;
    unsigned long lastTime = 0;
    unsigned long hit_time = 0; // when the event was first recorded

    int dH=50;
    int dL=-50;
    prevX = 0;
    prevY = 0;
    prevZ = 0;
}

void BaseCollect::storeData(int dX, int dY, int dZ){
   circ_buff_crtpos++;
   if(circ_buff_crtpos == BUFFER_SIZE){
       circ_buff_crtpos=0;
   }
   circ_buff_storage[circ_buff_crtpos].ax = dX;
   circ_buff_storage[circ_buff_crtpos].ay = dY;
   circ_buff_storage[circ_buff_crtpos].az = dZ;
}
