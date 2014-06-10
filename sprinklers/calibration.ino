// Author: Adam Wilson
// this sketch is to give remote control to the sprinklers
// to aid in calibrate the physical sprinklers.
#include <SPI.h>         
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Time.h>
#include <avr/wdt.h>
#include "zones.h"

// The zones are defined in setup()

// the maximum run length of each zone
// this is to keep a zone from running forever
// (and makes a lazy way to water. >_>)
int run_duration = min_to_sec(30);

time_t start_time = -1;
time_t uptime = -1;

// class to handle the zone realys
Zone_Controller zones;

const time_t timeUpdateInterval = SECS_PER_DAY; // Update the time every day.
const int timeZone = -6; // The hour offest from GMT. i.e. MST is a -6 offset.

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[]     = {0x90, 0xA2, 0xDA, 0x0D, 0x23, 0x81 };
byte ip[]      = {192, 168, 1, 69 };
//byte gateway[] = {192, 168, 1, 1};
//byte subnet[]  = {255, 255, 255, 0};

unsigned int localUDPPort = 8888;      // local port to listen for UDP packets

IPAddress timeServer(97,107,134,213); // time.nist.gov NTP server

const int NTP_PACKET_SIZE= 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets 

// A UDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

void setup() 
{ 
  // disable the SD card
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH);
  
  // create the zones here
  zones.add(8); // zone 1
  zones.add(7); // zone 2
  zones.add(6); // zone 3
  //zones.add(5); // zone 4
  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) 
  {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
   
  // start the Ethernet connection and the server:
  Ethernet.begin(mac, ip);
  server.begin();
  Serial.print("Server is at ");
  Serial.println(Ethernet.localIP());
  
  // start UDP
  Udp.begin(localUDPPort);
  setSyncProvider(updateTime);
  setSyncInterval(timeUpdateInterval);
  
  // enable the watchdog
  wdt_enable(WDTO_8S);
  
  // wait for the time to be retrieved
  while (timeStatus() != timeSet)
    wdt_reset();
    
  // store the time the board was powered on
  uptime = now();
}

void loop()
{
  // tell the watchdog we're still alive
  wdt_reset();
  
  // handle any ethernet stuff we have to
  check_for_client();
  
  // check to see if we need 
  if (start_time > 0)
  {
     if (now() - start_time >= run_duration)
        zones.turn_all_off(); 
  }
}

// handles any ethernet clients
void check_for_client()
{
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    //Serial.println("new client");
    String line = "";
    int zone = -1;
    
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        line += String(c);
        
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          // turn zones on/off if needed
          if (zone > 0)
          {
            if (zones.valid(zone) && !zones.is_on(zone))
            {
              zones.turn_on(zone); 
              start_time = now();
            }
          }
          else if (zone == 0)
          {
            zones.turn_all_off();
            start_time = -1;
          }
          // send a standard http response header
          client.write("HTTP/1.1 200 OK\n");
          client.write("Content-Type: text/html\n");
          client.write("Connection: close\n\n");  // the connection will be closed after completion of the response
          client.write("<!DOCTYPE HTML>\n");
          client.write("<html><head><title>Calibrate Sprinklers</title></head><body>");
          
          time_t t = now();
          client.write("<h1>Current time: ");
          char buffer[20];
          sprintf(buffer, "%02d:%02d:%02d %02d-%02d-%04d", hour(t), minute(t), second(t), month(t), day(t), year(t)); 
          client.print(buffer);
          client.write("</h1>\n");
          
          time_t up = t - uptime;
          client.write("<h1>Up time: ");
          int days = elapsedDays(up);
          int hours = numberOfHours(up);
          int minutes = numberOfMinutes(up);
          int seconds = numberOfSeconds(up);
          sprintf(buffer, "%03dd %02dh %02dm %02ds", days, hours, minutes, seconds); 
          client.print(buffer);
          client.write("</h1>\n");

          for (int i = 1; i <= zones.count(); i++)
          {
            client.write("<h1> Zone ");
            client.print(i);
            client.write(" is ");
            if (zones.is_on(i))
            {
               client.write("on");
            }
            else
            {
               client.write("off");
            }
            client.write("</h1>");   

            client.write("<form method='GET'>");
            client.write("<input type='hidden' name='zone' value='");
            client.print(i);
            client.write("'/>");
            client.write("<input type='submit' value='");
            if (zones.is_on(i) && start_time > 0)
            {
              client.write("Left ");
              time_t left = run_duration - (now() - start_time);
              char buffer[9];
              sprintf(buffer, "%02d:%02d:%02d", hour(left), minute(left), second(left)); 
              client.print(buffer);
              client.write("' disabled id='zone_timer'");
            }
            else
            {
              client.write("Turn On'");
            }
            client.write(" style='width: 100%; height: 175px; font-size: 300%'/>");
            client.write("</form><br/>");
          }
          
          client.write("<hr><form method='GET'>");
          client.write("<input type='hidden' name='zone' value='0'/>");
          client.write("<input type='submit' value='Turn All Off' style='width: 100%; height: 175px; font-size: 300%'/>");
          client.write("</form><br/>");
          
          client.write("<form method='GET'>");
          client.write("<input type='submit' value='Refresh' style='width: 100%; height: 175px; font-size: 300%'/>");
          client.write("</form><br/>");
          
          client.write("</body></html>");
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          
          
          line.toLowerCase();
          // see if this line contains the GET
          int pos = line.indexOf("get /?zone=");
          if (pos != -1)
          {
             String snum = line.substring(pos+11,pos+12);
             //Serial.print("Zone picked: ");
             zone = snum.toInt();
             //Serial.println(zone);
          }
          
          currentLineIsBlank = true;
          //Serial.print(line);
          line = "";
        } 
        else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(10);
    // close the connection:
    client.stop();
    //Serial.println("client disonnected");
  }
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
  Serial.println("Retrieving NTP time");
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
