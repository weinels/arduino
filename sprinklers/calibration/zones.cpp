#include "Arduino.h"
#include "zones.h"

Zone_Controller::Zone_Controller()
{
 _pins = new int[1];
 _size = 1;
 _count = 0;
}

Zone_Controller::~Zone_Controller()
{
  delete _pins;
}

// increases the size of the array
void Zone_Controller::enlarge_array()
{
  int *old_array = _pins;
  int old_size = _size;
  
  // increase the the size of the array by 1
  _pins = new int[_size+1];
  _size++;
  
  for (int i = 0; i < old_size; i++)
    _pins[i] = old_array[i];
    
  delete old_array;
}

// determines if the give zone is valid
bool Zone_Controller::valid(int zone)
{
  // see if the zone is a valid one
 if (zone < 1 || zone > _count)
   return false;
   
 return true;
}

// adds a new zone
// returns the zone number on success
// returns -1 on failure
int Zone_Controller::add(int pin)
{
  // only non-negative intergers can be pins
  if (pin < 0)
    return -1;
  
  // see if we need to enlarge the array
  if (_count + 1 > _size)
    enlarge_array();
    
  // set the pin to output
  pinMode(pin, OUTPUT);
  
  // make sure the relay is off
  digitalWrite(pin, HIGH);
  
  // add the pin to the array
  _pins[_count] = pin;
  _count ++;
  
  return _count;
}

// gets the pin for a given zone
// -1 means an invalid zone was passed
int Zone_Controller::pin(int zone)
{
 // see if the zone is a valid one
 if (!valid(zone))
   return -1;
   
 return _pins[zone-1];

}

// true of zone is on, otherwise false
bool Zone_Controller::is_on(int zone)
{
  if (!valid(zone))
    return false;
    
  return digitalRead(pin(zone)) == LOW;
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
  delay(100);
  
  // turn on the zone
  digitalWrite(pin(zone), LOW);
}

// stops a zone
void Zone_Controller::turn_off(int zone)
{
 turn_all_off();
}

// turns all zones off
void Zone_Controller::turn_all_off()
{
  // LOW is relay off
  // turn all the relays off
  for (int i = 0; i < _count; i++)
    digitalWrite(_pins[i], HIGH);
}

int Zone_Controller::count()
{
  return _count;
}

