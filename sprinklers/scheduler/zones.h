#ifndef zones_h
#define zones_h
#include "Arduino.h"

class Zone
{
  friend class Zone_Controller;
  
 private:
  int _pin;
  int _time;
  
 public:
   Zone(int pin, int time);
   void on();
   void off();
   bool is_on();
};

class Zone_Controller
{
  private:
    Zone **_zones;
    int *_times;
    int _size;
    int _count;
    void enlarge_array();
    
  public: 
    Zone_Controller(int count);
    ~Zone_Controller();
    int add(int pin, int secs);
    int pin(int zone);
    bool valid(int zone);
    bool is_on(int zone);
    int count();
    void turn_on(int zone);
    void turn_off(int zone);
    void turn_all_off();
    int time(int zone);
};

#endif

