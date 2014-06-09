#ifndef zones_h
#define zones_h
#include "Arduino.h"

class Zone_Controller
{
  private:
    int *_pins;
    int _size;
    int _count;
    void enlarge_array();
    
  public: 
    Zone_Controller();
    ~Zone_Controller();
    int add(int pin);
    int pin(int zone);
    bool valid(int zone);
    bool is_on(int zone);
    int count();
    void turn_on(int zone);
    void turn_off(int zone);
    void turn_all_off();
};

#endif

