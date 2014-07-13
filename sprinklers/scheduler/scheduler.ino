// Author: Adam Wilson
// this sketch is to schedule automatic lawn waterings
#include <SPI.h>   
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <avr/wdt.h>
#include "zones.h"
#include <EEPROM.h>
#include "EEPROMAnything.h"

// The zones are defined in setup()

// Holds the details of when to water
struct water_schedule
{
  int hour;
  int min;
  int period;
  time_t last;
} schedule;

// track the uptime
time_t uptime = 0;

// class to handle the zone realys
// my yard has three zones
Zone_Controller zones(3);

const time_t timeUpdateInterval = SECS_PER_DAY; // Update the time every day.
const int timeZone = -6; // The hour offest from GMT. i.e. MST is a -6 offset.

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[]     = {0x90, 0xA2, 0xDA, 0x0D, 0x23, 0x81};
byte ip[]      = {192, 168, 1, 69};
//byte gateway[] = {192, 168, 1, 1};
//byte subnet[]  = {255, 255, 255, 0};

byte server[]  = {192, 168, 1, 42}; // ip to send status updates to

unsigned int localUDPPort = 8888;      // local port to listen for UDP packets

IPAddress timeServer(97,107,134,213); // time.nist.gov NTP server

const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

EthernetClient client;

int zone_to_water;
void setup() 
{ 
  // enable the watchdog
  wdt_enable(WDTO_4S);
  wdt_reset();
  
  // disable the SD card
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) 
  {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  Serial.println("Starting up.");
  
  // create the zones here
  // the order they are added is the order they will be watered
  zones.add(7, 60); // frontyard
  zones.add(6, 20); // side strip
  zones.add(8, 60); // backyard
  
  // set when to water
  schedule.hour = 5;
  schedule.min = 0;
  schedule.period = 2; // everyother day
  
//  for(int i = 0; i < zones.count(); ++i)
//  {
//    Serial.print("Zone ");
//    Serial.print(i+1);
//    Serial.print(": pin ");
//    Serial.print(zones.pin(i));
//    Serial.print(" and time ");
//    Serial.println(zones.time(i));
//  }
 
  // start UDP
  Ethernet.begin(mac,ip);
  Udp.begin(localUDPPort);
  setSyncProvider(updateTime);
  setSyncInterval(timeUpdateInterval);
  
  // wait for the time to be retrieved
  while (timeStatus() != timeSet)
  { 
    ; // if it takes longer than 4 seconds to get the time, then the ethernet shield has fucked up.
  }
  
  wdt_reset();
  
  Serial.print("Current time is: ");
  print_time(now());
  Serial.print("\n");
 
  // store the time the board was powered on
  uptime = now();
  
  // use to reset the last day watered
//  time_t t = 42;
//  EEPROM_writeAnything(0,t);
  
  // get the last time watered
  EEPROM_readAnything(0,schedule.last);
  
  // make sure we water as soon as possible, used for testing
//  schedule.last = 0;
//  time_t t = now() + SECS_PER_MIN;
//  schedule.hour = hour(t);
//  schedule.min = minute(t);
  
  Serial.print("Last watered on ");
  print_time(schedule.last);
  Serial.print("\n");
  
  // print the time of the next watering
  Serial.print("Next watering is at: ");
  print_time(next_watering());
  Serial.print("\n");
  
  // setup the alarm to see if we water today
  Alarm.alarmRepeat(schedule.hour, schedule.min, 0, check_if_we_water);
  
  int size = 50;
  char buffer[size];
  snprintf(buffer, size, "Device%%20beginning%%20operation.");
  send_status(buffer);
  
  time_t next = next_watering();
  snprintf(buffer, size, "Next%%20watering:%%20%02d:%02d:%02d%%20%02d-%02d-%04d.", hour(next), minute(next), second(next), month(next), day(next), year(next));
  send_status(buffer);
}

void loop()
{
  // tell the watchdog we're still alive
  wdt_reset();
 
  Alarm.delay(0);
}

void check_if_we_water()
{
   if (next_watering() <= now())
   {
     zone_to_water = 0;
     int offset = 0;
     time_t total = 5; // start watering in one second, that we're sure the timers will trigger
     // time to start watering
     for(int i = 0; i < zones.count(); ++i)
     {
       Alarm.timerOnce(numberOfHours(total), numberOfMinutes(total), numberOfSeconds(total), water_zone);
       total += zones.time(i) * SECS_PER_MIN;
     }
     int h = numberOfHours(total);
     int m = numberOfMinutes(total);
     int s = numberOfSeconds(total);
     Alarm.timerOnce(h, m, s, stop_watering);
     
     int size = 40;
     char buffer[size];
     snprintf(buffer, size, "Started%%20watering.");
     send_status(buffer);
     snprintf(buffer, size, "Watering%%20finished%%20in%%20%dh%%20%dm.", h, m);
     send_status(buffer);
   }
}

void water_zone()
{
  // only water valid zones
  if (!zones.valid(zone_to_water))
    return;
  
  zones.turn_on(zone_to_water++);
  
  int size = 25;
  char buffer[size];
  snprintf(buffer, size, "Zone%%20%d%%20started.", zone_to_water);
  send_status(buffer);
}

void stop_watering()
{
  // stop the zones
  zones.turn_all_off();
  
  // updated the last watered time
  schedule.last = now();
  EEPROM_writeAnything(0,schedule.last);
  
  int size = 50;
  char buffer[size];
  snprintf(buffer, size, "Finished%%20watering.");
  send_status(buffer);
  
  time_t next = next_watering();
  snprintf(buffer, size, "Next%%20watering:%%20%02d:%02d:%02d%%20%02d-%02d-%04d.", hour(next), minute(next), second(next), month(next), day(next), year(next));
  send_status(buffer);
}

void send_status(char* buf)
{
   //Serial.println(buf);
   if (client.connect(server, 80)) 
   {
//     Serial.println("connected");
    // Make a HTTP request:
    client.print("GET /~akamu/sprinkler_status.php?m=");
    client.print(buf);
    client.println(" HTTP/1.1");
    client.println("Host: www.google.com");
    client.println("Connection: close");
    client.println();
    // we don't care about the response, so wait a tenth of a second and close the connection
    delay(100);
    client.stop();
    }
    
//    while (client.available()) 
//    {
//    char c = client.read();
//    Serial.print(c);
//    }
//
//    // if the server's disconnected, stop the client:
//    if (!client.connected()) 
//    {
//      Serial.println();
//      Serial.println("disconnecting.");
//      client.stop();
//    } 
}

time_t next_watering()
{
  time_t next = (elapsedDays(schedule.last)+schedule.period)*SECS_PER_DAY + schedule.hour*SECS_PER_HOUR + schedule.min*SECS_PER_MIN;
  if (next < now())
  {
    // find the next avaliable watering time
    int i = 0;
    do
    {
      next = (elapsedDays(now())+i++)*SECS_PER_DAY + schedule.hour*SECS_PER_HOUR + schedule.min*SECS_PER_MIN;
    } while(next < now());  
  }
  return next;
}

void print_time(time_t t)
{
  char buffer[20];
  snprintf(buffer, 20, "%02d:%02d:%02d %02d-%02d-%04d", hour(t), minute(t), second(t), month(t), day(t), year(t)); 
  Serial.print(buffer);
}

// converts minutes to milliseconds
unsigned long min_to_sec(int m)
{
  return m * 60;
}

// send an NTP request to the time server at the given address 
unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE); 
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49; 
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp: 		   
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer,NTP_PACKET_SIZE);
  Udp.endPacket(); 
}

unsigned long updateTime()
{
  //Serial.println("Retrieving NTP time.");
  sendNTPpacket(timeServer); // send an NTP packet to a time server

    // wait to see if a reply is available
  delay(1000);  
  if ( Udp.parsePacket() ) {  
    // We've received a packet, read the data from it
    Udp.read(packetBuffer,NTP_PACKET_SIZE);  // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);  
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;     
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;  
    //return the time, adjusting for the timezone
    return epoch+(timeZone*60*60);
  }    
  return 0; // return 0 if something went wrong
}
