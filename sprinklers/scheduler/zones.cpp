#include "Arduino.h"
#include "zones.h"

Zone::Zone(int pin, int time)
{
  _pin = pin;
  _time = time;
  
  // set the pin to output
  pinMode(_pin, OUTPUT);
  
  // make sure the relay is off
  digitalWrite(_pin, HIGH);
}

void Zone::on()
{
  // turn on the zone
  digitalWrite(_pin, LOW);
}

void Zone::off()
{
  // turn off the zone
  digitalWrite(_pin, HIGH);
}

bool Zone::is_on()
{
  return digitalRead(_pin) == LOW;
}

Zone_Controller::Zone_Controller(int init_size)
{
  if (init_size < 1)
    init_size = 1;
    
 _zones = new Zone*[init_size];
 _zones[0] = NULL;
 _size = 1;
 _count = 0;
}

Zone_Controller::~Zone_Controller()
{
  for (int i = 0; i < _size; ++i)
    if(_zones[i])
      delete _zones[i];
  delete[] _zones;
}

// increases the size of the array
void Zone_Controller::enlarge_array()
{
    
  Zone **old_array = _zones;
  int old_size = _size;
  
  // increase the the size of the array by 1
  _zones = new Zone*[++_size];
  
  // null the new array
  for( int i = 0; i < _size; ++i)
    _zones[i] = NULL;
    
  // copy the pointer out of the old array
  for (int i = 0; i < old_size; ++i)
    _zones[i] = old_array[i];
    
  // delete the old array
  delete[] old_array;
}

// determines if the give zone is valid
bool Zone_Controller::valid(int zone)
{
  // see if the zone is a valid one
 if (zone < 0 || zone >= _count)
   return false;
 
 if (_zones[zone])
   return true;
 else
   return false;
}

// adds a new zone
// time is in minutes
// returns the zone number on success
// returns -1 on failure
int Zone_Controller::add(int pin, int time)
{
  // only non-negative intergers can be pins
  if (pin < 0)
    return -1;
    
  // only positive times
  if (time < 0)
    return -1;
    
  // see if we need to enlarge the array
  if (_count + 1 > _size)
    enlarge_array();
  
  // add the zone to the list  
  _zones[_count] = new Zone(pin, time);
  return _count++;
}

// gets the pin for a given zone
// -1 means an invalid zone was passed
int Zone_Controller::pin(int zone)
{
 // see if the zone is a valid one
 if (!valid(zone))
   return -1;
   
 return (*_zones[zone])._pin;
}

// true of zone is on, otherwise false
bool Zone_Controller::is_on(int zone)
{
  if (!valid(zone))
    return false;
  
  return _zones[zone]->is_on(); 
}

// starts a zone
void Zone_Controller::turn_on(int zone)
{
  // check that the zone is valid
  if (!valid(zone))
    return;
    
  // if the selected zone is already on,
  // nothing to do
  if (is_on(zone))
    return;
    
  // make sure all the relays are off
  turn_all_off();
  delay(25);
  
  // turn on the zone
  _zones[zone]->on();
}

// stops a zone
void Zone_Controller::turn_off(int zone)
{
 turn_all_off();
}

// turns all zones off
void Zone_Controller::turn_all_off()
{
  // turn all the relays off
  for (int i = 0; i < _count; ++i)
    _zones[i]->off();
}

int Zone_Controller::count()
{
  return _count;
}

int Zone_Controller::time(int zone)
{
  if (!valid(zone))
    return -1;
  
  return _zones[zone]->_time; 
}

