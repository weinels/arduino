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

const int zone1 = 8;
const int zone2 = 7;
const int zone3 = 6;
const int zone4 = 5;

// LOW is relay off
void stop_all_zones()
{
  digitalWrite(zone1,HIGH);
  digitalWrite(zone2,HIGH);
  digitalWrite(zone3,HIGH);
  digitalWrite(zone4,HIGH);
}

// HIGH is relay on
void start_zone(int zone)
{
  // close all the relays
  stop_all_zones();
  
  // give the relay a bit to close
  delay(100);
  
  // turn on the right relay
  switch (zone)
 {
    case 1:
      digitalWrite(zone1, LOW);
      break;
    case 2:
      digitalWrite(zone2, LOW);
      break;
    case 3:
      digitalWrite(zone3, LOW);
      break;
    case 4:
      digitalWrite(zone4, LOW);
      break;
    default:
      // ignore any other values passed
      return;
 } 
 
 delay(100);
}
 
bool is_zone_on(int zone)
{
  // read the relay
  switch (zone)
 {
    case 1:
      return digitalRead(zone1) == LOW;
      break;
    case 2:
      return digitalRead(zone2) == LOW;
      break;
    case 3:
      return digitalRead(zone3) == LOW;
      break;
    case 4:
      return digitalRead(zone4) == LOW;
      break;
    default:
      // ignore any other will be considered off
      return false;
 }
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
  pinMode(zone1,OUTPUT);
  pinMode(zone2,OUTPUT);
  pinMode(zone3,OUTPUT);
  pinMode(zone4,OUTPUT);
  
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
          client.print("<input type='hidden' name='zone' value='5'/>");
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


