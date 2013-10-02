//
// Laurentiu Cucos
// September 15 2013
//

#ifndef _BASE_COLLECT_H_
#define _BASE_COLLECT_H_

#include "Arduino.h"

struct SensVals{
  int ax;
  int ay;
  int az;
};

class BaseCollect{
private:
	int circ_buff_crtpos ;
	int hit_stamp ;
	int after_hit_samples;

    unsigned long lastTime ;
    unsigned long hit_time ; // when the event was first recorded

    SensVals circ_buff_storage[BUFFER_SIZE];

    int dH;
    int dL;

    int prevX;
    int prevY;
    int prevZ;

public:
    BaseCollect();
	void storeData(int dX, int dY, int dZ);

    void setTreshold(int val);

    void loopProcess(int x, int y, int z);
};

#endif
