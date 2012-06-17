#include "Arduino.h"
#include "vectors.h"
#include "configuration.h"
#include "pins.h"
#include "extruder.h"
#include "cartesian_dda.h"


// VARIABLES

extern FloatPoint where_i_am;
extern volatile byte head;
extern volatile byte tail;
extern byte serial_count;
extern boolean comment;


// FUNCTIONS

void blink();
void shutdown();
byte getTimerResolution(const long&delay);
unsigned int getTimerCeiling(const long&delay);
void setTimerResolution(byte r);


// INLINE FUNCTIONS

inline void setPosition(const FloatPoint&p){
  where_i_am = p;  
};


inline bool qFull(){
  if(tail == 0)
    return head == (BUFFER_SIZE - 1);
  else
    return head == (tail - 1);
};


inline void qMove(const FloatPoint&p){
  while(qFull())
  {
    manageAllExtruders();
    delay(WAITING_DELAY);
  }
  byte h = head; 
  h++;
  if(h >= BUFFER_SIZE)
    h = 0;
  cdda[h]->set_target(p);
  head = h;
};


inline bool qEmpty(){
   return tail == head && !cdda[tail]->active();
};


inline void cancelAndClearQueue(){
	tail = head;	// clear buffer
	for(int i=0;i<BUFFER_SIZE;i++)
		cdda[i]->kill();
};


inline void dQMove(){
  if(qEmpty())
    return;
  byte t = tail;  
  t++;
  if(t >= BUFFER_SIZE)
    t = 0;
  cdda[t]->dda_start();
  tail = t; 
};


inline void init_process_string(){
	serial_count = 0;
  comment = false;
};


// Depending on how much work the interrupt function has to do, this is
// pretty accurate between 10 us and 0.1 s.  At fast speeds, the time
// taken in the interrupt function becomes significant, of course.

// Note - it is up to the user to call enableTimerInterrupt() after a call
// to this function.

inline void setTimer(long delay){
	// delay is the delay between steps in microsecond ticks.
	//
	// we break it into 5 different resolutions based on the delay. 
	// then we set the resolution based on the size of the delay.
	// we also then calculate the timer ceiling required. (ie what the counter counts to)
	// the result is the timer counts up to the appropriate time and then fires an interrupt.

        // Actual ticks are 0.0625 us, so multiply delay by 16
        
        delay <<= 4;
        
	setTimerCeiling(getTimerCeiling(delay));
	setTimerResolution(getTimerResolution(delay));
};


inline void setUnits(bool u){
   for(byte i = 0; i < BUFFER_SIZE; i++)
     cdda[i]->set_units(u); 
};
