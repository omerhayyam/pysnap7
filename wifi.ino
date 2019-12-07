/*----------------------------------------------------------------------
 
 * ESP8266 Wifi Joystick connect to PLC S7-300 (CPU314C-2DP + CP343-1) by TUENHIDIY.
 * Thank to Mr.Davide Nardella for his "settimino" Library.
 
----------------------------------------------------------------------*/
/*----------------------------------------------------------------------

 Created  12 Dec 2016
 Modified 10 Mar 2019 for Settimino 2.0.0
 by Davide Nardella
  
----------------------------------------------------------------------*/
#include "Platform.h"
#include "Settimino.h"
#include <Adafruit_NeoPixel.h>

#define RING    D4    // NEOPIXEL RING 16

#define UP      D5    // JOYSTICK UP POSITION
#define DOWN    D6    // JOYSTICK DOWN POSITION
#define RIGHT   D7    // JOYSTICK RIGHT POSITION
#define LEFT    D8    // JOYSTICK LEFT POSITION

#define _RED                Adafruit_NeoPixel::Color(255, 0, 0)           //    RED
#define _GREEN              Adafruit_NeoPixel::Color(0, 255, 0)           //    GREEN
#define _BLUE               Adafruit_NeoPixel::Color(0, 0, 255)           //    BLUE
#define _WHITE              Adafruit_NeoPixel::Color(255, 255, 255)       //    WHITE
#define _BLACK              Adafruit_NeoPixel::Color(0, 0, 0)             //    BLACK
#define _GOLDEN_ROD         Adafruit_NeoPixel::Color(238, 232, 170)       //    GOLDEN ROD
#define _DARK_MAGENTA       Adafruit_NeoPixel::Color(148, 0, 211)         //    DARK MAGENTA
#define _YELLOW_GREEN       Adafruit_NeoPixel::Color(85, 107, 47)         //    YELLOW GREEN

Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, RING, NEO_GRB + NEO_KHZ800);

byte WritetoPLC[64]={0};      // Data Array to be written to PLC
byte ReadfromPLC[64]={0};     // Data Array to be read from PLC

// Uncomment next line to perform small and fast data access
#define DO_IT_SMALL

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x0F, 0x08, 0xE1 };

IPAddress Local(192, 168, 0, 70); // Local Address
IPAddress PLC(192, 168, 0, 71);   // PLC Address

// Following constants are needed if you are connecting via WIFI
// The ssid is the name of my WIFI network (the password obviously is wrong)
char ssid[] = "FTP-Telecom";    // Your network SSID (name)
char pass[] = "12345689";  // Your network password (if any)

IPAddress Gateway(192, 168, 0, 1);
IPAddress Subnet(255, 255, 255, 0);

int DBNum = 1; // This DB must be present in your PLC
byte Buffer[1024];

S7Client Client;

unsigned long Elapsed; // To calc the execution time
//----------------------------------------------------------------------
// Setup : Init Ethernet and Serial port
//----------------------------------------------------------------------
void setup() {

    strip.begin();
    strip.setBrightness(50);
    strip.show(); // Initialize all pixels to 'off'

    pinMode(UP, INPUT);
    pinMode(DOWN, INPUT);
    pinMode(LEFT, INPUT);
    pinMode(RIGHT, INPUT);

    pinMode(RING, OUTPUT);  
    
    // Open serial communications and wait for port to open:
    Serial.begin(115200);
    
#ifdef S7WIFI
//--------------------------------------------- ESP8266 Initialization    
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    WiFi.config(Local, Gateway, Subnet);
    while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
      Error_Indicator();
    }
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.print("Local IP address : ");
    Serial.println(WiFi.localIP());
    Ready_Indicator();
#else
//--------------------------------Wired Ethernet Shield Initialization    
    // Start the Ethernet Library
    EthernetInit(mac, Local);
    // Setup Time, someone said me to leave 2000 because some 
    // rubbish compatible boards are a bit deaf.
    delay(2000); 
    Serial.println("");
    Serial.println("Cable connected");  
    Serial.print("Local IP address : ");
    Serial.println(Ethernet.localIP());
#endif   
}
//----------------------------------------------------------------------
// Connects to the PLC
//----------------------------------------------------------------------
bool Connect()
{
    int Result=Client.ConnectTo(PLC, 
                                  0,  // Rack (see the doc.)
                                  2); // Slot (see the doc.)
    Serial.print("Connecting to "); Serial.println(PLC);  
    if (Result==0) 
    {
      Serial.print("Connected ! PDU Length = "); Serial.println(Client.GetPDULength());
      Ready_Indicator();
    }
    else
      Serial.println("Connection error");
      Error_Indicator();
    return Result==0;
}
//----------------------------------------------------------------------
// Dumps a buffer (a very rough routine)
//----------------------------------------------------------------------
void Dump(void *Buffer, int Length)
{
  int i, cnt=0;
  pbyte buf;
  
  if (Buffer!=NULL)
    buf = pbyte(Buffer);
  else  
    buf = pbyte(&PDU.DATA[0]);

  Serial.print("[ Dumping "); Serial.print(Length);
  Serial.println(" bytes ]===========================");
  for (i=0; i<Length; i++)
  {
    cnt++;
    if (buf[i]<0x10)
    Serial.print("0");
    Serial.print(buf[i], HEX);
    Serial.print(" ");
    if (cnt==16)
    {
      cnt=0;
      Serial.println();
    }
  }  
  Serial.println("===============================================");
}
//----------------------------------------------------------------------
// Prints the Error number
//----------------------------------------------------------------------
void CheckError(int ErrNo)
{
  Serial.print("Error No. 0x");
  Serial.println(ErrNo, HEX);
  
  // Checks if it's a Severe Error => we need to disconnect
  if (ErrNo & 0x00FF)
  {
    Serial.println("SEVERE ERROR, disconnecting.");
    Client.Disconnect(); 
  }
}
//----------------------------------------------------------------------
// Profiling routines
//----------------------------------------------------------------------
void MarkTime()
{
  Elapsed=millis();
}
//----------------------------------------------------------------------
void ShowTime()
{
  // Calcs the time
  Elapsed=millis()-Elapsed;
  Serial.print("Job time (ms) : ");
  Serial.println(Elapsed);   
}
//----------------------------------------------------------------------
// Main Loop
//----------------------------------------------------------------------
void loop() 
{
  int Size, Result;
  void *Target;

//----------------------------------------------------------------------
// Read Joystick
//---------------------------------------------------------------------- 
   if (digitalRead(UP) == 1)
  {
      WritetoPLC[0] = 1;      // UP position is stored in WritetoPLC Array and written to PLC at DB1.DBX0.0
   }

  else if (digitalRead(DOWN) == 1)
  {
      WritetoPLC[0] = 2;      // DOWN position is stored in WritetoPLC Array and written to PLC at DB1.DBX0.1 
   }

  else if (digitalRead(LEFT) == 1)
  {
      WritetoPLC[0] = 4;      // LEFT position is stored in WritetoPLC Array and written to PLC at DB1.DBX0.2 
   }
  else if (digitalRead(RIGHT) == 1)
  {
      WritetoPLC[0] = 8;      // RIGHT position is stored in WritetoPLC Array and written to PLC at DB1.DBX0.3
   }   
  else
  {
      WritetoPLC[0] = 0;      // Joystick ZERO position   
  }
    
    Serial.print("Write data prepare: "); Serial.println(WritetoPLC[0]);
    //delay(1000);   
   
//----------------------------------------------------------------------
    
#ifdef DO_IT_SMALL
  Size=1;
  Target = NULL; // Uses the internal Buffer (PDU.DATA[])
#else
  Size=1024;
  Target = &Buffer; // Uses a larger buffer
#endif
  
  // Connection
  while (!Client.Connected)
  {
    if (!Connect())
      delay(500);
      Ready_Indicator();
  }

  // Get the current tick
  MarkTime();
  
  // Write commands from WIFI JOYSTICK to "DB1 - Byte 0" of PLC
  // And PLC will carry out the commands as follows
  // UP      -    DB1.DBX0.0  - CONTROL RELAY 0 AT PLC OUTPUT Q124.0
  // DOWN    -    DB1.DBX0.1  - CONTROL RELAY 1 AT PLC OUTPUT Q124.1
  // RIGHT   -    DB1.DBX0.2  - CONTROL RELAY 2 AT PLC OUTPUT Q124.2
  // LEFT    -    DB1.DBX0.3  - CONTROL RELAY 3 AT PLC OUTPUT Q124.3

  Client.WriteArea(S7AreaDB, // We are requesting DB access
                         DBNum,    // DB Number
                         0,        // Start from byte N.0
                         Size,     // We need "Size" bytes
                         &WritetoPLC);  // Put them into our target (Buffer or PDU)
  
  Serial.print("Reading "); Serial.print(Size); Serial.print(" bytes from DB"); Serial.println(DBNum);
  
  // Read the feedback signals from "DB1 - Byte 1" of PLC
  // The feedback signals are Normal Open (NO) aux. contact from RELAYS and they are connected to PLC INPUTS as follows
  // FEEDBACK UP POSTION      - PLC INPUT I124.0  -    DB1.DBX1.0
  // FEEDBACK DOWN POSTION    - PLC INPUT I124.1  -    DB1.DBX1.1
  // FEEDBACK RIGHT POSTION   - PLC INPUT I124.2  -    DB1.DBX1.2
  // FEEDBACK LEFT POSTION    - PLC INPUT I124.3  -    DB1.DBX1.3
  
  Result=Client.ReadArea(S7AreaDB, // We are requesting DB access
                         DBNum,    // DB Number
                         1,        // Start from byte N.1
                         Size,     // We need "Size" bytes
                         &ReadfromPLC);  // Put them into our target (Buffer or PDU)
  
  if (Result==0)
  {
    ShowTime();
    Dump(&ReadfromPLC, Size);
  
  // LED Indicator from feedback signals
  // It means when we command from WIFI JOYSTICK to PLC, then after processing PLC will feedback the status of this command by LED.
    
  // FEEDBACK UP POSTION  
    if (bitRead(ReadfromPLC[0],0))
    {
      for(int i=0; i<4; i++) 
      { 
        strip.setPixelColor(i, _RED);         
        strip.show();                          
      }
    }
   // FEEDBACK DOWN POSTION 
    else if (bitRead(ReadfromPLC[0],1))
    {
      for(int i=8; i<12; i++)
      { 
        strip.setPixelColor(i, _GREEN);         
        strip.show();                                     
      }
    }
    // FEEDBACK RIGHT POSTION
    else if (bitRead(ReadfromPLC[0],2))
    {
      for(int i=12; i<strip.numPixels(); i++)      
      { 
        strip.setPixelColor(i, _BLUE);         
        strip.show();                             
      }
    }
    // FEEDBACK LEFT POSTION
    else if (bitRead(ReadfromPLC[0],3))
    {
      for(int i=4; i<8; i++) 
      { 
        strip.setPixelColor(i, _WHITE);         
        strip.show();                            
        
      }
    }
    // JOYSTICK ZERO POSTION
    else
    {
      for(int i=0; i<strip.numPixels(); i++) 
      { 
        strip.setPixelColor(i, _BLACK);         
        strip.show();                                
      }
    }
  }
  else
    CheckError(Result);   
    //delay(100);  
}

// WIFI JOYSTICK FAULT INDICATION
void Error_Indicator() 
{
  for(int i=0; i<strip.numPixels(); i++) 
  {
    strip.setPixelColor(i, _DARK_MAGENTA);         
    strip.show();                          
  }
}
// WIFI JOYSTICK READY INDICATION
void Ready_Indicator() 
{
  for(int i=0; i<strip.numPixels(); i++) 
  { // For each pixel in strip...
    strip.setPixelColor(i, _YELLOW_GREEN);         
    strip.show();                          
  }
}