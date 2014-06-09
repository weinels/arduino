// Author: Adam Wilson
// this sketch is to give remote control to the sprinklers
// to aid in calibrate the physical sprinklers.
#include <SPI.h>         
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <Time.h>
#include <SD.h>
#include <SPI.h>
#include <Ethernet.h>

const int zones[] = {8,7,6,5};
const int zones_count = 4;

// gets the pin of the selected zone.
// returns -1 if an invalid zone was passed
int zone_to_pin(int zone)
{
 // see if the zone is a valid one
 if (zone < 1 || zone > zones_count)
   return -1;
   
 return zones[zone-1];
}

// LOW is relay off
// turns all zones off
void stop_all_zones()
{
  // turn all the relays off
  for (int i = 0; i < zones_count; i++)
    digitalWrite(zones[i], HIGH);
}

// determines if the passed zone is on or off
bool is_zone_on(int zone)
{
  int pin = zone_to_pin(zone);
  if (pin < 0)
    return false;
    
  return digitalRead(pin) == LOW;
}

// HIGH is relay on
void start_zone(int zone)
{
  // if the selected zone is already on,
  // nothing to do
  if (is_zone_on(zone))
    return;
  
  // get the pin of the zone
  int pin = zone_to_pin(zone);
  if (pin < 0)
    return;
    
  // make sure all the relays are off
  stop_all_zones();
  delay(100);
  
  // turn on the zone
  digitalWrite(pin, LOW);
}
 
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[]     = {0x90, 0xA2, 0xDA, 0x0D, 0x23, 0x81 };
byte ip[]      = {192, 168, 1, 69 };
//byte gateway[] = {192, 168, 1, 1};
//byte subnet[]  = {255, 255, 255, 0};

unsigned int localUDPPort = 8888;      // local port to listen for UDP packets

// Initialize the Ethernet server library
// with the IP address and port you want to use 
// (port 80 is default for HTTP):
EthernetServer server(80);

void setup() 
{ 
  // set the pins for the relay
  for (int i = 0; i < zones_count; i++)
    pinMode(zones[i],OUTPUT);
  
  // make sure all the relays are off
  stop_all_zones();
  
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
}

void loop()
{
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("new client");
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
            start_zone(zone); 
          }
          else if (zone == 0)
          {
            stop_all_zones();
          }
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  // the connection will be closed after completion of the response
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html><head><title>Calibrate Sprinklers</title><body>");

          for (int i = 1; i <= 4; i++)
          {
            client.print("<h1> Zone ");
            client.print(i);
            client.print(" is ");
            if (is_zone_on(i))
               client.print("on");
            else
               client.print("off");
            client.println("</h1>");   

            client.println("<form method='GET'>");
            client.print("<input type='hidden' name='zone' value='");
            client.print(i);
            client.println("'/>");
            client.print("<input type='submit' value='Turn Zone ");
            client.print(i);
            client.println(" On' style='width: 100%; height: 175px;'/>");
            client.println("</form><br/>");
          }
          
          client.println("<form method='GET'>");
          client.print("<input type='hidden' name='zone' value='0'/>");
          client.print("<input type='submit' value='Turn All Zones Off' style='width: 100%; height: 175px;'/>");
          client.println("</form><br/>");
          
          client.println("</body></html>");
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
             Serial.print("Zone picked: ");
             zone = snum.toInt();
             Serial.println(zone);
          }
          
          currentLineIsBlank = true;
          Serial.print(line);
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
    Serial.println("client disonnected");
  }
}


