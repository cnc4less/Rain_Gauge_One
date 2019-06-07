///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//                       Version 1.0  "Rain_Gauge_One.ino"  06/7/2019  @ 01:28 EDT Developed by William Lucid
//                       Licensed under GNU GPL v3.0, https://www.gnu.org/licenses/gpl.html
//
//
//                       Arduino.cc topic: "NTP --ESP8266 Time-Synced, Data Logging and Web Server"
//
//                       "NTP_Time-synced_Web_Interface.ino" developed by William Lucid
//
//                       Portion of NTP time code was developed from code provided by schufti  --of ESP8266Community
//
//                       listFiles and readFile functions by martinayotte of ESP8266 Community Forum.  readFile function modified by RichardS of ESP8266 Community Forum for ESP32.
//
//                       https://forum.arduino.cc/index.php?topic=543468.0    //Project discussion at arduino.cc
//
//                       Note:  This Sketch uses the "Adafruit, Espressif ESP32 Development Board - Developer Edition."
//
//                       https://www.adafruit.com/product/3269
//
//                       Time keeping using rtc and GPS time used to update DS3231.
//                       GPS and rain gauge code developed by haroon552 of Arduino.cc Community Forum
//
//                       Previous projects:  https://github.com/tech500
//
//                       Project is Open-Source, requires one BME280 breakout board and "Adafruit, Espressif ESP32 Development Board - Developer Edition."
//
//                       https://bit.ly/2DDV5zV  Project web page  --Servered from ESP8266.
//
//                       https://bit.ly/2M5NBs0  Domain, Hosted web page
//
//
//   External            Note:  Must use esp32 core by ESP32 Community version 1.0.1 from "Arduino IDE, Board Manager"
//
///////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Some lines require editing with your data; such as YourSSID, YourPassword, Your ThinkSpeak ChannelNumber, Your Thinkspeak API Key, Public IP, Forwarded Port,
//  Your filename for restricted access to "ACCESS.TXT."
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ********************************************************************************
// ********************************************************************************
//
//   See invidual library downloads for each library license.
//
//   Following code was developed using the Adafruit CC300 library, "HTTPServer" example.
//   and by merging library examples, adding logic for Sketch flow.
//
// *********************************************************************************
// *********************************************************************************

#include "EEPROM.h"  //Part of version 1.0.1 ESP32 Board Manager install
#include "WiFi.h"   //Part of version 1.0.1 ESP32 Board Manager install
#include "ESPmDNS.h"  //Part of version 1.0.1 ESP32 Board Manager install
#include "WebServer.h"  //Part of version 1.0.1 ESP32 Board Manager install
#include "ESP8266FtpServer.h"  //https://github.com/nailbuster/esp8266FTPServer  -->Needed for ftp transfers
#include "SPIFFS.h" //Part of version 1.0.1 ESP32 Board Manager install
#include "HTTPClient.h"   //Part of version 1.0.1 ESP32 Board Manager install  ----> Used for Domain Web Interace
#include "Update.h"  //1.0.1 ESP32 Board Manager install
#include <DS3232RTC.h>  //https://github.com/JChristensen/DS3232RTC
#include <Timezone.h>    // https://github.com/JChristensen/Timezone
#include <ThingSpeak.h>   //https://github.com/mathworks/thingspeak-arduino . Get it using the Library Manager
#include <TinyGPS++.h> //http://arduiniana.org/libraries/tinygpsplus/ Used for GPS parsing
#include <BME280I2C.h>   //Use the Arduino Library Manager, get BME280 by Tyler Glenn
#include <Wire.h>    //Part of version 1.0.1 ESP32 Board Manager install  -----> Used for I2C protocol
//#include <LiquidCrystal_I2C.h>   //https://github.com/esp8266/Basic/tree/master/libraries/LiquidCrystal Not used anymore

// Replace with your network details
const char* host = "esp32";
const char* ssid = "yourssid";
const char* password = "networkpassword";
WiFiClient client;
////////////////////////// FTP /////////////////////////
FtpServer ftpSrv;
/////////////////////////////////////////////////////////

static const uint32_t GPSBaud = 9600;                   // Ublox GPS default Baud Rate is 9600

const double Home_LAT = 88.888888;                      // Your Home Latitude --edit with your data
const double Home_LNG = 88.888888;                      // Your Home Longitude --edit with your data
const char* WiFi_hostname = "DS3231-NTP";

HardwareSerial uart(1);  //change to uart(2) <--GPIO pins 16 and 17

TinyGPSPlus gps;

DS3232RTC RTC;

int DOW, MONTH, DATE, YEAR, HOUR, MINUTE, SECOND;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

int lc = 0;
time_t tnow = 0;

//year, month, date, hour, min, sec and week-day(starts from 0 and goes to 6)
//writing any non-existent time-data may interfere with normal operation of the RTC.
//Take care of week-day also.
//DateTime dt(2011, 11, 10, 15, 18, 0, 5);

// US Eastern Time Zone (New York, Detroit)
TimeChangeRule myDST = {"EDT", Second, Sun, Mar, 2, -240};    //Daylight time = UTC - 4 hours
TimeChangeRule mySTD = {"EST", First, Sun, Nov, 2, -300};     //Standard time = UTC - 5 hours
Timezone myTZ(myDST, mySTD);

// If TimeChangeRules are already stored in EEPROM, comment out the three
// lines above and uncomment the line below.
//Timezone myTZ(100);       //assumes rules stored at EEPROM address 100

TimeChangeRule *tcr;        //pointer to the time change rule, use to get TZ abbrev

WebServer httpServer(85);
//HTTPUpdate httpUpdater;

///////////////////////  OTA  ////////////////////////////
WebServer otaServer(80);

/*
 * Login page
 */

const char* loginIndex =
     "<form name='loginForm'>"
     "<table width='20%' bgcolor='A09F9F' align='center'>"
     "<tr>"
     "<td colspan=2>"
     "<center><font size=4><b>ESP32 Login Page</b></font></center>"
     "<br>"
     "</td>"
     "<br>"
     "<br>"
     "</tr>"
     "<td>Username:</td>"
     "<td><input type='text' size=25 name='userid'><br></td>"
     "</tr>"
     "<br>"
     "<br>"
     "<tr>"
     "<td>Password:</td>"
     "<td><input type='Password' size=25 name='pwd'><br></td>"  
     "<br>"
     "</tr>"
     "<tr>"
     "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
     "</tr>"
     "</table>"
     "</form>"
     "<script>"
     "function check(form)"
     "{"
     "if(form.userid.value=='admin' && form.pwd.value=='admin')"  //Default userid and passwrd value is 'admin'
     "<br>"
     "{"
     "window.open('/serverIndex')"
     "}"
     "else"
     "{"
     " alert('Error Password or Username')/*displays error message*/"
     "}"
     "}"
     "</script>";

/*
 * Server Index Page
 */

const char* serverIndex =
     "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
     "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
     "<input type='file' name='update'>"
     "<input type='submit' value='Update'>"
     "</form>"
     "<div id='prg'>progress: 0%</div>"
     "<script>"
     "$('form').submit(function(e){"
     "e.preventDefault();"
     "var form = $('#upload_form')[0];"
     "var data = new FormData(form);"
     " $.ajax({"
     "url: '/update',"
     "type: 'POST',"
     "data: data,"
     "contentType: false,"
     "processData:false,"
     "xhr: function() {"
     "var xhr = new window.XMLHttpRequest();"
     "xhr.upload.addEventListener('progress', function(evt) {"
     "if (evt.lengthComputable) {"
     "var per = evt.loaded / evt.total;"
     "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
     "}"
     "}, false);"
     "return xhr;"
     "},"
     "success:function(d, s) {"
     "console.log('success!')"
     "},"
     "error: function (a, b, c) {"
     "}"
     "});"
     "});"
     "</script>";

/////////////////////////  End OTA  ///////////////////////

char message[29];

String dtStamp;

String lastUpdate;

unsigned long delayTime;

float temp(NAN), hum(NAN), pres(NAN), pressure, millibars, fahrenheit, RHx, T, heat_index, dew, dew_point, atm;

float HeatIndex, DewPoint, temperature, humidity, TempUnit_Fahrenheit;

int count = 0;

int error = 0;

BME280I2C bme;    // Default : forced mode, standby time = 1000 ms
// Oversampling = pressure ×1, temperature ×1, humidity ×1, filter off,

bool AcquisitionDone = false;

int started;   //Used to tell if Server has started

//use I2Cscanner to find LCD display address, in this case 3F   //https://github.com/todbot/arduino-i2c-scanner/
//LiquidCrystal_I2C lcd(0x3F,16,2);  // set the LCD address to 0x3F for a 16 chars and 2 line display

//#define sonalert 9  // pin for Piezo buzzer

#define online D5  //pin for online LED indicator

float currentPressure;  //Present pressure reading used to find pressure change difference.
float pastPressure;  //Previous pressure reading used to find pressure change difference.
float milliBars;   //Barometric pressure in millibars
float difference;   //change in barometric pressure drop; greater than .020 inches of mercury.

//long int id = 1;  //Increments record number

char *filename;
char str[] = {0};

String fileRead;

char MyBuffer[13];

String publicIP = "xxx.xxx.xxx.xxx";   //in-place of xxx.xxx.xxx.xxx put your Public IP address inside quotes

#define LISTEN_PORT           yyyy // in-place of yyyy put your listening port number
// The HTTP protocol uses port 80 by default.

#define MAX_ACTION            10      // Maximum length of the HTTP action that can be parsed.

#define MAX_PATH              64      // Maximum length of the HTTP request path that can be parsed.
// There isn't much memory available so keep this short!

#define BUFFER_SIZE           MAX_ACTION + MAX_PATH + 20  // Size of buffer for incoming request data.
// Since only the first line is parsed this
// needs to be as large as the maximum action
// and path plus a little for whitespace and
// HTTP version.

#define TIMEOUT_MS           500 // Amount of time in milliseconds to wait for     /////////default 500/////
// an incoming request to finish.  Don't set this
// too high or your server could be slow to respond.

uint8_t buffer[BUFFER_SIZE + 1];
int bufindex = 0;
char action[MAX_ACTION + 1];
char path[MAX_PATH + 1];

//////////////////////////
// Web Server on port LISTEN_PORT
/////////////////////////
WiFiServer server(LISTEN_PORT);

/*
  This is the ThingSpeak channel number for the MathwWorks weather station
  https://thingspeak.com/channels/YourChannelNumber.  It senses a number of things and puts them in the eight
  field of the channel:

  Field 1 - Temperature (Degrees C )
  Field 2 - Humidity (%RH)
  Field 3 - Barometric Pressure (hpa)
  Field 4 - Rain Last 5 Minutes  (mm)
*/

//edit ThingSpeak.com data here...
unsigned long myChannelNumber = 123456;
const char * myWriteAPIKey = "E12345";

/* You only need to format SPIFFS the first time you run a
   test or else use the SPIFFS plugin to create a partition
   https://github.com/me-no-dev/arduino-esp32fs-plugin */

//Hardware pin definitions
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// digital I/O pins

#define online 19

float gpslng = 0.0, gpslat = 0.0, gpsalt = 0.0;
int gpssat = 0;

//Variables used for GPS
//unsigned long age;
int gps_year, gps_month, gps_day;
int seconds_new, minutes_new, hours_new, gps_year_new, gps_month_new, gps_day_new;

//Calibrate rain bucket here
//Rectangle raingauge from Sparkfun.com weather sensors
//float rain_bucket_mm = 0.011*25.4;//Each dump is 0.011" of water
//DAVISNET Rain Collector 2
//float rain_bucket_mm = 0.01*25.4;//Each dump is 0.01" of water

// volatiles are subject to modification by IRQs
//volatile unsigned long raintime, rainlast, raininterval, rain, Rainindtime, Rainindlast;  // For Rain
//int addr=0;

#define eeprom_size 512

String eepromstring = "0.00";

//for loop
//int i; 

unsigned long lastSecond,last5Minutes;
float lastPulseCount;
int currentPulseCount;
float rain5min;
float rainFall;
float rainHour;
float rainDay;
float daysRain;

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//Interrupt routines (these are called by the hardware interrupts, not by the main code)
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
#define FIVEMINUTES (300*1000L) 
#define REEDPIN 32
#define REEDINTERRUPT 0 

//int count;

volatile int pulseCount_ISR = 0;

portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR reedSwitch_ISR()
{
    static unsigned long lastReedSwitchTime;
    // debounce for a quarter second = max. 4 counts per second
    if (labs(millis()-lastReedSwitchTime)>250) 
    {
        portENTER_CRITICAL_ISR(&mux);
        pulseCount_ISR++;
        
        lastReedSwitchTime=millis();
        portEXIT_CRITICAL_ISR(&mux);
    }

}     

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup(void)
{


     while (!Serial);

     Serial.begin(115200);
     uart.begin(9600, SERIAL_8N1, 17, 16);

     delay(3000); // wait for console opening

     Wire.begin(22,21);

     setSyncProvider(RTC.get);   // the function to get the time from the RTC
  if(timeStatus()!= timeSet)
     Serial.println("Unable to sync with the RTC");
  else
        Serial.println("RTC has set the system time");
   
     while (WiFi.status() != WL_CONNECTED)
     {
          delay(1000 * 5);

          wifi_Start();
     }

     pinMode(4, INPUT_PULLUP); //Set input (SDA) pull-up resistor on GPIO_0 // Change this *! if you don't use a Wemos

  pinMode(REEDPIN,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(REEDPIN),reedSwitch_ISR, FALLING); 
     
     // initialize EEPROM with predefined size
     EEPROM.begin(eeprom_size);
    
    //RESET EEPROM CONTENT - ONLY EXECUTE ONE TIME - AFTER COMMENT

/*

     Uncomment to 'clear'.eeprom values.

     Serial.println("CLEAR ");
     eepromClear();
     Serial.println("SET ");
     eepromSet("rain5min", "0.00");
     eepromSet("rainDay", "0.00");
     eepromSet("rainHour", "0.00");
     Serial.println("LIST ");
     Serial.println(eepromList());
*/

     //END - RESET EEPROM CONTENT - ONLY EXECUTE ONE TIME - AFTER COMMENT
     //eepromClear();
  
     //GET STORED RAINCOUNT IN EEPROM
     Serial.println("");
     Serial.println("GET EEPROM --Setup");
     eepromstring=eepromGet("rainDay");
     rainDay=eepromstring.toFloat();
     Serial.print("RAINDAY VALUE FROM EEPROM: ");
     Serial.println(rainDay);

     eepromstring=eepromGet("rainHour");
     rainHour=eepromstring.toFloat();
     Serial.print("RAINHOUR VALUE FROM EEPROM: ");
     Serial.println(rainHour);

     eepromstring=eepromGet("rain5min");
     rain5min=eepromstring.toFloat();
     Serial.print("rain5min VALUE FROM EEPROM: ");
     Serial.println(rain5min);
     //END - GET STORED RAINCOUNT IN EEPROM

     //  pinMode(online, OUTPUT); //Set pinMode to OUTPUT for online LED



     while(!bme.begin())
     {
          Serial.println("Could not find BME280 sensor!");
          delayTime = 1000;
     }

     // bme.chipID(); // Deprecated. See chipModel().
     switch(bme.chipModel())
     {
     case BME280::ChipModel_BME280:
          Serial.println("Found BME280 sensor! Success.");
          break;
     case BME280::ChipModel_BMP280:
          Serial.println("Found BMP280 sensor! No Humidity available.");
          break;
     default:
          Serial.println("Found UNKNOWN sensor! Error!");
     }

     if(!SPIFFS.begin(true))
     {
          Serial.println("SPIFFS Mount Failed");
          return;
     }

////////////////////////// FTP /////////////////////////////////
//////FTP Setup, ensure SPIFFS is started before ftp;  /////////
#ifdef ESP32       //esp32 we send true to format spiffs if cannot mount
     if (SPIFFS.begin(true))
     {
#elif defined ESP8266
     if (SPIFFS.begin())
     {
#endif
          Serial.println("SPIFFS opened!");
          ftpSrv.begin("admin","admin");    //username, password for ftp.  set ports in ESP8266FtpServer.h  (default 21, 50009 for PASV)
     }
////////////////////////////////////////////////////////////////

     delayTime = 1000;

     //SPIFFS.format();
     //SPIFFS.remove("/SERVER.TXT");
     //SPIFFS.remove("/WIFI.TXT");
     //SPIFFS.remove("/LOG421.TXT");
     //SPIFFS.rename("/LOG715 TXT", "/LOG715.TXT");

     //lcdDisplay();      //   LCD 1602 Display function --used for inital display

     ThingSpeak.begin(client);

     //WiFi.disconnect();  //For testing wifiStart function in setup and listen functions.

     //minutes_5m    = MINUTE % 5;

     //lastMSecond = millis();

}

// How big our line buffer should be. 100 is plenty!
#define BUFFER 100

void loop()
{

  // each second read and reset pulseCount_ISR
  if (millis()-lastSecond>=1000) 
  {
    lastSecond+=1000;
    portENTER_CRITICAL(&mux);
    currentPulseCount+=pulseCount_ISR; // add to current counter  
    pulseCount_ISR=0; // reset ISR counter
    rainFall = currentPulseCount * .047; //Amout of rain in one bucket dump.
    portEXIT_CRITICAL(&mux);
      
    Serial.print("Rainfall:  " + (String)rainFall);Serial.print('\t');Serial.print("Dumps during 5 Minutes:  " + (String)currentPulseCount);

    Serial.println();
  }

    // each 5 minutes save data to another counter
    if (millis()-last5Minutes>=FIVEMINUTES) 
    {
        float daysRain;
    
        rain5min = rainFall;
        rainHour = rainHour + rainFall;  //accumulaing 5 minute rainfall for 1 hour then reset -->rainHour Rainfall 
        rainDay = rainDay + rainFall;  //Only 1 hour
        daysRain = daysRain + rainDay  ;  //accumulaing every hour rainfall for 1 day then reset -->day total
        //Serial.println("5 Minute Rainfall:  " + (String)rain5min);
        //Serial.println("Hourly rainfall rate:  " + (String)rainHour);
        //Serial.println("60 Minute Accumulating:  " + (String)rainHour);
        //Serial.println("Day Rainfall:  " + (String)rainDay);
        last5Minutes+=FIVEMINUTES; // remember the time
        lastPulseCount+=currentPulseCount; // add to last period Counter
        currentPulseCount=0;; // reset counter for current period
    
         
    }
    
    yield();

     otaServer.handleClient();
     delay(1);

     while (WiFi.status() != WL_CONNECTED)
     {
          delay(1000 * 5);

          wifi_Start();
     }

     getDate_Time();

     for(int x = 1; x < 5000; x++)
     {
          ftpSrv.handleFTP();
     }

     // This sketch displays information every time a new sentence is correctly encoded.
     while (uart.available() > 0)
          if (gps.encode(uart.read()))
               //displayInfo();
               //delay(3000);;



               if (millis() > 5000 && gps.charsProcessed() < 10)
               {
                    Serial.println(F("No GPS detected: check wiring."));
                    while(true);
               }


     if (started == 1)
     {

          RTC_UPDATE();

          // Open a "log.txt" for appended writing
          File log = SPIFFS.open("/SERVER.TXT", FILE_APPEND);

          if (!log)
          {
               Serial.println("file 'SERVER.TXT' open failed");
          }

          log.print("Started Server:  ");
          log.println(dtStamp);
          log.close();

          started = 0;   //only time started = 1 is when Server starts in setup

     }

     //Resets every 5 Minutes and 50 Seconds.
     if ((MINUTE % 5 == 0) && (SECOND == 0))  //Reset rain5min variable every 5 MINUTES 11 SECOND.  //SECONDS offset needed to not interfere with 15 minute routing
     {
          
		delayTime = 20000;
		
		int min;
		
		min++;
		
		Serial.println("");
          Serial.println("Five Minute routine");
          Serial.println(dtStamp);

          //seteeprom();

          getWeatherData();
          lastUpdate = dtStamp;   //store dtstamp for use on dynamic web page
          updateDifference();  //Get Barometric Pressure difference
          
          delayTime = 2000;

          //STORE RAINCOUNT IN EEPROM
          Serial.println("SET EEPROM rainHour");
          eepromstring = String(rainHour,2);
          eepromSet("rainHour", eepromstring);
          //END - STORE RAINCOUNT IN EEPROM

          //STORE RAINCOUNT IN EEPROM
          Serial.println("SET EEPROM rainDay");
          eepromstring = String(rainDay,2);
          eepromSet("rainDay", eepromstring);
          //END - STORE RAINCOUNT IN EEPROM
		
		      //STORE RAINCOUNT IN EEPROM
          Serial.println("SET EEPROM rain5min");
          eepromstring = String(rain5min,2);
          eepromSet("rain5min", eepromstring);
          //END - STORE RAINCOUNT IN EEPROM

          rainFall = 0;
		
		if(min == 3)  //do fifthteen minute routine
		{
			Serial.println("");
			Serial.println("Fifthteen minute routine");
			Serial.println(dtStamp);

			delay(2000);

			
			logtoSD();   //Output to SPIFFS  --Log to SPIFFS on 15 minute interval.
			delayTime = 200;  //Be sure there is enough SPIFFS write time
			//webInterface();  //Interface for Hosted Domain Website.
			delayTime = 200;
			
			pressure = pastPressure;
			delayTime = 100;
			speak();
			
			RTC_UPDATE();
			
			min = 0;
		}
     }
      
	if((MINUTE % 4 == 0) && (SECOND == 10))  //One hour; clear rainHour.
	{
		rainFall = 0;
		rainHour = 0;
	}

     if((HOUR == 23) && (MINUTE == 59) && (SECOND == 59))  //24 Hour; clear ALL.
     {
          fileStore();
          
          rain5min = 0;
          rainFall = 0;
          rainHour = 0;
          rainDay = 0;
          daysRain = 0;
         
         
     }

     
  
  for(int x = 1; x<210000; x++);
     {
          listen();
     }

}
////////////////
void accessLog()
{

     String ip1String = "10.0.0.146";   //Host ip address
     String ip2String = client.remoteIP().toString();   //client remote IP address

     //Open a "access.txt" for appended writing.   Client access ip address logged.
     File logFile = SPIFFS.open("/ACCESS.TXT", FILE_APPEND);

     if (!logFile)
     {
          Serial.println("File 'ACCESS.TXT'failed to open");
     }

     if ((ip1String == ip2String) || (ip2String == "0.0.0.0"))
     {

          //Serial.println("IP Address match");
          logFile.close();

     }
     else
     {
          //Serial.println("IP address that do not match ->log client ip address");

          logFile.print("Accessed:  ");
          logFile.print(dtStamp);
          logFile.print(" -- Client IP:  ");
          logFile.print(client.remoteIP());
          logFile.print(" -- ");
          logFile.print("Action:  ");
          logFile.print(action);
          logFile.print(" -- ");
          logFile.print("Path:  ");
          logFile.print(path);

          //Serial.println("Error = " + (String)error);


          if ((error == 1) || (error == 2))
          {

               if(error == 1)
               {

                    Serial.println("Error 404");
                    logFile.print("  Error 404");
                    error = 0;

               }

               if(error == 2)
               {

                    Serial.println("Error 405");
                    logFile.print("  Error 405");

               }

          }

          error = 0;

          logFile.println("");
          logFile.close();

     }

}


////////////////////////////////
void beep(unsigned char delayms)
{

     // wait for a delayms ms
//     digitalWrite(sonalert, HIGH);
//     delayTime = 3000;
//     digitalWrite(sonalert, LOW);

}

//---------------------------------------------------------EEPROM----------------------------------------------

void eepromSet(String name, String value)
{
     Serial.println("eepromSet");

     String list=eepromDelete(name);
     String nameValue="&" + name + "=" + value;
     //Serial.println(list);
     //Serial.println(nameValue);
     list+=nameValue;
     for (int i = 0; i < list.length(); ++i)
     {
          EEPROM.write(i,list.charAt(i));
     }
     EEPROM.commit();
     Serial.print(name);
     Serial.print(":");
     Serial.println(value);
}


String eepromDelete(String name)
{
     Serial.println("eepromDelete");

     int nameOfValue;
     String currentName="";
     String currentValue="";
     int foundIt=0;
     char letter;
     String newList="";
     for (int i = 0; i < 512; ++i)
     {
          letter= char(EEPROM.read(i));
          if (letter=='\n')
          {
               if (foundIt==1)
               {
               }
               else if (newList.length()>0)
               {
                    newList+="=" + currentValue;
               }
               break;
          }
          else if (letter=='&')
          {
               nameOfValue=0;
               currentName="";
               if (foundIt==1)
               {
                    foundIt=0;
               }
               else if (newList.length()>0)
               {
                    newList+="=" + currentValue;
               }

          }
          else if (letter=='=')
          {
               if (currentName==name)
               {
                    foundIt=1;
               }
               else
               {
                    foundIt=0;
                    newList+="&" + currentName;
               }
               nameOfValue=1;
               currentValue="";
          }
          else
          {
               if (nameOfValue==0)
               {
                    currentName+=letter;
               }
               else
               {
                    currentValue+=letter;
               }
          }
     }

     for (int i = 0; i < 512; ++i)
     {
          EEPROM.write(i,'\n');
     }
     EEPROM.commit();
     for (int i = 0; i < newList.length(); ++i)
     {
          EEPROM.write(i,newList.charAt(i));
     }
     EEPROM.commit();
     Serial.println(name);
     Serial.println(newList);
     return newList;
}
void eepromClear()
{
     Serial.println("eepromClear");
     for (int i = 0; i < 512; ++i)
     {
          EEPROM.write(i,'\n');
     }
}
String eepromList()
{
     Serial.println("eepromList");
     char letter;
     String list="";
     for (int i = 0; i < 512; ++i)
     {
          letter= char(EEPROM.read(i));
          if (letter=='\n')
          {
               break;
          }
          else
          {
               list+=letter;
          }
     }
     Serial.println(list);
     return list;
}
String eepromGet(String name)
{
     Serial.println("eepromGet");

     int nameOfValue;
     String currentName="";
     String currentValue="";
     int foundIt=0;
     String value="";
     char letter;
     for (int i = 0; i < 512; ++i)
     {
          letter= char(EEPROM.read(i));
          if (letter=='\n')
          {
               if (foundIt==1)
               {
                    value=currentValue;
               }
               break;
          }
          else if (letter=='&')
          {
               nameOfValue=0;
               currentName="";
               if (foundIt==1)
               {
                    value=currentValue;
                    break;
               }
          }
          else if (letter=='=')
          {
               if (currentName==name)
               {
                    foundIt=1;
               }
               else
               {
               }
               nameOfValue=1;
               currentValue="";
          }
          else
          {
               if (nameOfValue==0)
               {
                    currentName+=letter;
               }
               else
               {
                    currentValue+=letter;
               }
          }
     }
     Serial.print(name);
     Serial.print(":");
     Serial.println(value);
     return value;
}

//////////
void end()
{

     // Wait a short period to make sure the response had time to send before
     // the connection is closed .

     delayTime = 1000;

     //Client flush buffers
     client.flush();
     // Close the connection when done.
     client.stop();

     //digitalWrite(online, LOW);   //turn-off online LED indicator

     getDate_Time();

     Serial.println("Client closed:  " + dtStamp);

     delayTime = 10;   //Delay for changing too quickly to new browser tab.

     loop();

}

////////////////
void fileStore()   //If 6th day of week, rename "log.txt" to ("log" + month + day + ".txt") and create new, empty "log.txt"
{


     String logname;
     logname = "/LOG";
     logname += DATE; ////logname += Clock.getMonth(Century);
     logname += MONTH;   ///logname += Clock.getDate();
     logname += YEAR;
     logname += ".TXT";
     // Open file for appended writing
     File log = SPIFFS.open(logname.c_str(), "a");

     // Open file for appended writing
     //File log = SPIFFS.open("/LOG.TXT", "a");

     if (!log)
     {
          Serial.println("file open failed");
     }

     // rename the file "LOG.TXT"
     /*
        String logname;
        logname = "/LOG";
        logname += MONTH; ////logname += Clock.getMonth(Century);
        logname += DATE;   ///logname += Clock.getDate();
        logname += ".TXT";
        SPIFFS.rename("/LOG.TXT", logname.c_str());
        log.close();

        //For troubleshooting
        //Serial.println(logname.c_str());
     */
}

void FTP_UPLOAD()
{
     boolean upload;  ////made thisup...
     upload = true;
     if (doFTP(upload))
          Serial.println(F("FTP OK"));
     else Serial.println(F("FTP FAIL"));
     //Neither of these print statements are displayed inSerial Monitor ////////////////////////////////////////////////////////////////////////////

     delayTime = 1000;  // give time to the WiFi handling in the background
}



#define FTPWRITE

boolean debug = true;

unsigned long startTime = millis();

// change to your server
IPAddress ftserver( 10,0,0,47 );

WiFiClient dclient;

char outBuf[128];
char outCount;

// change fileName to your file (8.3 format!)
////String fileName = logname;
//String  fpath = logname;

// SPIFFS file handle
File fh;


//format bytes
String formatBytes(size_t bytes)
{
     if (bytes < 1024)
     {
          return String(bytes) + "B";
     }
     else if (bytes < (1024 * 1024))
     {
          return String(bytes / 1024.0) + "KB";
     }
     else if (bytes < (1024 * 1024 * 1024))
     {
          return String(bytes / 1024.0 / 1024.0) + "MB";
     }
     else
     {
          return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
     }
}



//----------------- FTP fail
void efail()
{
     byte thisByte = 0;

     client.println(F("QUIT"));

     while (!client.available()) delayTime = 1;

     while (client.available())
     {
          thisByte = client.read();
          Serial.write(thisByte);
     }

     client.stop();
     Serial.println(F("Command disconnected"));
     //fh.close();
     Serial.println(F("SD closed"));
}
// efail

//-------------- FTP receive
byte eRcv()
{
     byte respCode;
     byte thisByte;

     while (!client.available()) delayTime = 1;

     respCode = client.peek();

     outCount = 0;

     while (client.available())
     {
          thisByte = client.read();
          Serial.write(thisByte);

          if (outCount < 127)
          {
               outBuf[outCount] = thisByte;
               outCount++;
               outBuf[outCount] = 0;
          }
     }

     if (respCode >= '4')
     {
          efail();
          return 0;
     }
     return 1;
}  // eRcv()

//--------------- FTP handling
byte doFTP(boolean upload)
{


     String logname;
     logname = "/LOG";
     logname += DATE; ////logname += Clock.getMonth(Century);
     logname += MONTH;   ///logname += Clock.getDate();
     logname += YEAR;
     logname += ".TXT";



     if (upload)
     {
          fh = SPIFFS.open(logname.c_str(), "a+");
     }
// else
// {
     //  SPIFFS.remove(path);
     //  fh = SPIFFS.open(path, "w");

     listen();
// }

     if (!fh)
     {
          Serial.println(F("SPIFFS open fail"));
          return 0;
     }

     if (upload)
     {
          if (!fh.seek((uint32_t)0, SeekSet))
          {
               Serial.println(F("Rewind fail"));
               //fh.close();
               return 0;
          }
     }

     if (debug) Serial.println(F("SPIFFS opened"));

     if (client.connect(ftserver, 21))    // 21 = FTP server
     {
          Serial.println(F("Command connected"));
     }
     else
     {
          //fh.close();
          Serial.println(F("Command connection failed"));
          return 0;
     }

     if (!eRcv()) return 0;
     if (debug) Serial.println("Send USER");
     client.println(F("USER admin"));

     if (!eRcv()) return 0;
     if (debug) Serial.println("Send PASSWORD");
     client.println(F("PASS admin"));

     if (!eRcv()) return 0;
     if (debug) Serial.println("Send SYST");
     client.println(F("SYST"));

     if (!eRcv()) return 0;
     if (debug) Serial.println("Send Type I");
     client.println(F("Type I"));

     if (!eRcv()) return 0;
     if (debug) Serial.println("Send PASV");
     client.println(F("PASV"));

     if (!eRcv()) return 0;

     char *tStr = strtok(outBuf, "(,");
     int array_pasv[6];
     for ( int i = 0; i < 6; i++)
     {
          tStr = strtok(NULL, "(,");
          array_pasv[i] = atoi(tStr);
          if (tStr == NULL)
          {
               Serial.println(F("Bad PASV Answer"));
          }
     }
     unsigned int hiPort, loPort;
     hiPort = array_pasv[4] << 8;
     loPort = array_pasv[5] & 255;

     if (debug) Serial.print(F("Data port: "));
     hiPort = hiPort | loPort;
     if (debug) Serial.println(hiPort);

     if (dclient.connect(ftserver, hiPort))
     {
          Serial.println(F("Data connected"));
     }
     else
     {
          Serial.println(F("Data connection failed"));
          client.stop();
          //fh.close();
          return 0;
     }

     if (upload)
     {
          if (debug) Serial.println("Send STOR filename");
          client.print(F("STOR "));
          client.println(logname.c_str());
     }
     else
     {
          if (debug) Serial.println("Send RETR filename");
          client.print(F("RETR "));
          client.println(logname.c_str());
     }

     if (!eRcv())
     {
          dclient.stop();
          return 0;
     }

     if (upload)
     {
          if (debug) Serial.println(F("Writing"));
          // for faster upload increase buffer size to 1460
//#define bufSizeFTP 64
#define bufSizeFTP 1460
          uint8_t clientBuf[bufSizeFTP];
          //unsigned int clientCount = 0;
          size_t clientCount = 0;

          while (fh.available())
          {
               clientBuf[clientCount] = fh.read();
               clientCount++;
               if (clientCount > (bufSizeFTP - 1))
               {
                    dclient.write((const uint8_t *) &clientBuf[0], bufSizeFTP);
                    clientCount = 0;
                    delayTime = 1;
               }
          }
          if (clientCount > 0) dclient.write((const uint8_t *) &clientBuf[0], clientCount);

     }
     else
     {
          while (dclient.connected())
          {
               while (dclient.available())
               {
                    char c = dclient.read();
                    fh.write(c);
                    if (debug) Serial.write(c);
               }
          }
     }

     dclient.stop();
     Serial.println(F("Data disconnected"));

     if (!eRcv()) return 0;

     client.println(F("QUIT"));

     if (!eRcv()) return 0;

     client.stop();
     Serial.println(F("Command disconnected"));

     //fh.close();
     //if (debug) Serial.println(F("SPIFS closed"));
     return 1;

     listen();  //Not sure where this should go.  do FTP needs to return to listen();   /////////////////////////////////////////////////////////////////////////////////////
}  // doFTP()


//void readSPIFFS() {
//  fh = SPIFFS.open(fileName, "r");

//  if (!fh) {
//    Serial.println(F("SPIFFS open fail"));
//    return;
// }

//  while (fh.available()) {
//   Serial.write(fh.read());
// }

//  fh.close();
//}  // readSPIFFS()


//////////////////////////////////
//rtc and unixtime
//////////////////////////////////

void getDate_Time()
{
  time_t utc = now();
  time_t local = myTZ.toLocal(utc, &tcr);
   
  HOUR = hour(local);          // returns the hour for the given time  
  MINUTE = minute(local);        // returns the minute for the given time  
  SECOND = second(local);        // returns the second for the given time  
  DATE = day(local);           // the day for the given time  
  DOW = weekday(local);       // day of the week for the given time  
  MONTH = month(local);         // the month for the given time  
  YEAR = year(local);          // the year for the given time  

  printDateTime(local, tcr -> abbrev);
  
}

// format and print a time_t value, with a time zone appended.
void printDateTime(time_t t, const char *tz)
{
    char buf[32];
    char m[4];    // temporary storage for month string (DateStrings.cpp uses shared buffer)
    strcpy(m, monthShortStr(month(t)));
    sprintf(buf, "%.2d:%.2d:%.2d %s %.2d %s %d %s",
        hour(t), minute(t), second(t), dayShortStr(weekday(t)), day(t), m, year(t), tz);
    dtStamp = buf;
}

void RTC_UPDATE()
{

     struct tm  timeinfo;
     unsigned long int unixtime;

     if((!gps.date.isValid()) || (!gps.time.isValid()))
     {
          getDate_Time();
          Serial.println(dtStamp);
          return;
     }

     Serial.print(F("GPS: "));
     if (gps.date.isValid())
     {

          timeinfo.tm_year =  gps.date.year() - 1900;
          timeinfo.tm_mon = gps.date.month() - 1;
          timeinfo.tm_mday =  gps.date.day();
     }
     else
     {
          Serial.print(F("Date INVALID"));
          exit;
     }

     Serial.print(F(" "));
     if (gps.time.isValid())
     {

          timeinfo.tm_hour =  gps.time.hour();
          timeinfo.tm_min =  gps.time.minute();
          timeinfo.tm_sec = gps.time.second();

          unixtime = mktime(&timeinfo);
          Serial.println("");
          printf("unixtime = %u\n", unixtime);

          setTime(unixtime);   // the function to get the time from the RTC
          if(timeStatus()!= timeSet)
             Serial.println("Unable to sync with the RTC");
          else
             Serial.println("RTC has set the system time");;
      
             Serial.println("RTC updated");
             Serial.println("");

     }
     else
     {
          Serial.print(F(" Time INVALID"));
          exit;
     }

     Serial.println();

}


/////////////////////
void getWeatherData()
{

     BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
     BME280::PresUnit presUnit(BME280::PresUnit_hPa);

     bme.read(pres, temp, hum, tempUnit, presUnit);

     pressure = pres + 30.602;  //Correction for relative pressure  ////////////////  Relative pressure in millibard = Absoute pressure + (elevation(in meters)/ 8.3)
     //Pabsolute pressure is the reading from the Bosch BME280 in millibars.
}

/////////////
void listen()   // Listen for client connection
{

     if(WiFi.status() != WL_CONNECTED)
     {
          wifi_Start();

          delay(1000 * 10);   //wait 10 seconds before writing

          //Open a "WIFI.TXT" for appended writing.   Client access ip address logged.
          File logFile = SPIFFS.open("/WIFI.TXT", "a");

          if (!logFile)
          {
               Serial.println("File failed to open");
          }
          else
          {
               logFile.print("Connected WiFi:  ");
               logFile.println(dtStamp);
          }


     }

     // Check if a client has connected
     client = server.available();

     if(client)
     {

          while(client.connected())
          {

               // Process this request until it completes or times out.
               // Note that this is explicitly limited to handling one request at a time!

               // Clear the incoming data buffer and point to the beginning of it.
               bufindex = 0;
               memset(&buffer, 0, sizeof(buffer));

               // Clear action and path strings.
               memset(&action, 0, sizeof(action));
               memset(&path,   0, sizeof(path));

               // Set a timeout for reading all the incoming data.
               unsigned long endtime = millis() + TIMEOUT_MS;

               // Read all the incoming data until it can be parsed or the timeout expires.
               bool parsed = false;

               while (!parsed && (millis() < endtime) && (bufindex < BUFFER_SIZE))
               {

                    if (client.available())
                    {

                         buffer[bufindex++] = client.read();

                    }

                    parsed = parseRequest(buffer, bufindex, action, path);

               }

               if (parsed)
               {

                    //ESP.wdtDisable();

                    error = 0;

                    // Check the action to see if it was a GET request.
                    if (strcmp(action, "GET") == 0)
                    {

                         digitalWrite(online, HIGH);  //turn on online LED indicator

                         String ip1String = "10.0.0.146";   //Host ip address
                         String ip2String = client.remoteIP().toString();   //client remote IP address

                         Serial.print("\n");
                         Serial.println("Client connected:  " + dtStamp);
                         Serial.print("Client IP:  ");
                         Serial.println(ip2String);
                         Serial.println(F("Processing request"));
                         Serial.print(F("Action: "));
                         Serial.println(action);
                         Serial.print(F("Path: "));
                         Serial.println(path);

                         accessLog();

                         // Check the action to see if it was a GET request.
                         if (strncmp(path, "/favicon.ico", 12) == 0)  // Respond with the path that was accessed.
                         {
                              filename = "/FAVICON.ICO";
                              strcpy(MyBuffer, filename);

                              // send a standard http response header
                              client.println("HTTP/1.1 200 OK");
                              client.println("Content-Type: image/x-icon");
                              client.println();

                              error = 0;

                              readFile();

                         }
                         // Check the action to see if it was a GET request.
                         if ((strcmp(path, "/Weather") == 0) || (strcmp(path, "/") == 0))   // Respond with the path that was accessed.
                         {

                              getWeatherData();

                              if(!isnan(dew))
                              {

                                   gpslat = gps.location.lat();
                                   gpslng = gps.location.lng();
                                   gpsalt = gps.altitude.feet();

                                   // First send the success response code.
                                   client.println("HTTP/1.1 200 OK");
                                   client.println("Content-Type: html");
                                   client.println("Connnection: close");
                                   client.println("Server: Robotdyn WiFi D1 R2");
                                   // Send an empty line to signal start of body.
                                   client.println("");
                                   // Now send the response data.
                                   // output dynamic webpage
                                   client.println("<!DOCTYPE HTML>");
                                   client.println("<html>");
                                   client.println("<head><title>Weather Observations</title>");
                                   client.println("<body>");
                                   // add a meta refresh tag, so the browser pulls again every 15 seconds:
                                   //client.println("<meta http-equiv=\"refresh\" content=\"15\">");
                                   client.println("<h2>Rain Station<br>");
                                   client.println("with GPS and RTC</h2></head><br>");

                                   if(lastUpdate != NULL)
                                   {
                                        client.println("Last Update:  ");
                                        client.println(lastUpdate);
                                        client.println("   <br>");
                                   }

                                   client.println("Latitude : " );
                                   client.print(gpslat,5);
                                   client.print(" <br>");
                                   client.println("Longitude : " );
                                   client.print(gpslng,5);
                                   client.print(" <br>");
                                   client.println("Elevation");
                                   client.print(gpsalt,0);
                                   client.print(" Feet.<br>");
                                   client.println("Temperature:  ");
                                   client.print(temp, 1);
                                   client.print(" C.<br>");
                                   client.println("Humidity:  ");
                                   client.print(hum, 1);
                                   client.print(" %<br>");
                                   //client.println("Dew point:  ");
                                   //client.print(DewPoint, 1);
                                   //client.print(" F. <br>");
                                   //client.println("Heat Index:  ");
                                   //client.print(HeatIndex, 1);
                                   //client.print(" F. <br>");
                                   client.println("Barometric Pressure:  ");
                                   client.print(pressure, 1);   //Inches of Mercury
                                   client.print(" hpa.<br>");
                                   client.println("Rain Day :  ");
                                   client.print(rainDay, 2);   //Inches of Mercury
                                   client.print(" Day/mm <br>");
                                   client.println("Rain Hour:  ");
                                   client.println(rainHour, 2);   //Convert hPA to millbars
                                   client.println(" Hour/mm <br>");
                                   client.println("Rain5min :  ");
                                   client.print(rainFall, 2);
                                   client.print(" 5min/mm <br>");
                                   //client.println("Elevation:  824 Feet<br>");
                                   client.println("<h2>Weather Observations</h2>");
                                   client.println("<h3>" + dtStamp + "    </h3><br>");
                                   client.println("<br>");
                                   client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/SdBrowse >File Browser</a><br>");
                                   client.println("<br>");
                                   client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/Graphs >Graphed Weather Observations</a><br>");
                                   client.println("<br>");
                                   client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/README.TXT download>Server:  README</a><br>");
                                   client.println("<br>");
                                   //Show IP Adress on client screen
                                   client.print("Client IP: ");
                                   client.print(client.remoteIP().toString().c_str());
                                   client.println("</body>");
                                   client.println("</html>");

                                   end();

                              }

                         }
                         // Check the action to see if it was a GET request.
                         else if (strcmp(path, "/SdBrowse") == 0)   // Respond with the path that was accessed.
                         {

                              // send a standard http response header
                              client.println("HTTP/1.1 200 OK");
                              client.println("Content-Type: text/html");
                              client.println();
                              client.println("<!DOCTYPE HTML>");
                              client.println("<html>");
                              client.println("<body>");
                              client.println("<head><title>SDBrowse</title><head />");
                              // print all the files, use a helper to keep it clean
                              client.println("<h2>Collected Observations</h2>");


                              //////////////// Code to listFiles from martinayotte of the "ESP8266 Community Forum" ///////////////
                              //////////////////// Modified for ESP32 //////////////////

                              String str = String("<!DOCTYPE HTML><html><head></head>");

                              if (!SPIFFS.begin(true))
                              {
                                   Serial.println("An Error has occurred while mounting SPIFFS");
                                   return;
                              }

                              File root = SPIFFS.open("/");

                              File file = root.openNextFile();

                              while (file)
                              {


                                   str += "<a href=\"";
                                   str += file.name();
                                   str += "\">";
                                   str += file.name();
                                   str += "</a>";
                                   str += "    ";
                                   str += file.size();
                                   str += "<br>\r\n";
                                   //str += "</body></html>\r\n";

                                   file = root.openNextFile();
                              }

                              client.print(str);

                              ////////////////// End code by martinayotte //////////////////////////////////////////////////////
                              client.println("<br><br>");
                              client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/Weather    >Current Observations</a><br>");
                              client.println("</body>");
                              client.println("</html>");

                              end();

                         }
                         // Check the action to see if it was a GET request.
                         else if (strcmp(path, "/Graphs") == 0)   // Respond with the path that was accessed.
                         {

                              //delayTime =1000;

                              // First send the success response code.
                              client.println("HTTP/1.1 200 OK");
                              client.println("Content-Type: html");
                              client.println("Connnection: close");
                              client.println("Server: Robotdyn D1 R2");
                              // Send an empty line to signal start of body.
                              client.println("");
                              // Now send the response data.
                              // output dynamic webpage
                              client.println("<!DOCTYPE HTML>");
                              client.println("<html>");
                              client.println("<body>");
                              client.println("<head><title>Graphed Weather Observations</title></head>");
                              // add a meta refresh tag, so the browser pulls again every 15 seconds:
                              //client.println("<meta http-equiv=\"refresh\" content=\"15\">");
                              client.println("<br>");
                              client.println("<h2>Graphed Weather Observations</h2><br>");
                              client.println("<p>");
                              client.println("<frameset rows='30%,70%' cols='33%,34%'>");
                              client.println("<iframe width='450' height='260' 'border: 1px solid #cccccc;' src='https://thingspeak.com/channels/290421/charts/1?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&timescale=15&title=Temperature&type=line&xaxis=Date&yaxis=Degrees'></iframe>");
                              client.println("<iframe width='450' height='260' 'border: 1px solid #cccccc;' src='https://thingspeak.com/channels/290421/charts/2?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&timescale=15&title=Humidity&type=line&xaxis=Date&yaxis=Humidity'></iframe>");
                              client.println("<br>");
                              client.println("<br>");
                              delay(200);
                              //client.println("</frameset>");
                              //client.println("<frameset rows='30%,70%' cols='33%,34%'>");
                              client.println("<iframe width='450' height='260' 'border: 1px solid #cccccc;' src='https://thingspeak.com/channels/290421/charts/3?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&timescale=15&title=Barometric+Pressure&type=line&xaxis=Date&yaxis=Pressure'></iframe>");
                              client.println("<iframe width='450' height='260' 'border: 1px solid #cccccc;' src='https://thingspeak.com/channels/290421/charts/4?bgcolor=%23ffffff&color=%23d62020&dynamic=true&results=60&timescale=15&title=Dew+Point&type=line'></iframe>");
                              client.println("</frameset>");
                              client.println("<br><br>");
                              client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/Weather    >Current Observations</a><br>");
                              client.println("<br>");
                              client.println("</p>");
                              client.println("</body>");
                              client.println("</html>");

                              end();

                         }


                         else if ((strncmp(path, "/LOG", 4) == 0) ||  (strcmp(path, "/WIFI.TXT") == 0) || (strcmp(path, "/DIFFER.TXT") == 0) || (strcmp(path, "/SERVER.TXT") == 0) || (strcmp(path, "/README.TXT") == 0))   // Respond with the path that was accessed.
                         {

                              char *filename;
                              char name;
                              strcpy( MyBuffer, path );
                              filename = &MyBuffer[1];

                              if ((strncmp(path, "/FAVICON.ICO", 12) == 0) || (strncmp(path, "/SYSTEM~1", 9) == 0) || (strncmp(path, "/ACCESS", 7) == 0))
                              {

                                   client.println("HTTP/1.1 404 Not Found");
                                   client.println("Content-Type: text/html");
                                   client.println();
                                   client.println("<h2>404</h2>");
                                   delay(250);
                                   client.println("<h2>File Not Found!</h2>");
                                   client.println("<br><br>");
                                   client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/SdBrowse    >Return to File Browser</a><br>");

                                   Serial.println("Http 404 issued");

                                   error = 1;

                                   end();
                              }
                              else
                              {

                                   client.println("HTTP/1.1 200 OK");
                                   client.println("Content-Type: text/plain");
                                   client.println("Content-Disposition: attachment");
                                   client.println("Content-Length:");
                                   client.println("Connnection: close");
                                   client.println();

                                   readFile();

                                   end();
                              }

                         }
                         // Check the action to see if it was a GET request.
                         else if (strncmp(path, "/zzzzzzzz.zzz", 7) == 0)  //replace "/zzzzzzz" with your choice.  Makes "ACCESS.TXT" a restricted file.  Use this for private access to remote IP file.
                         {
                              //Restricted file:  "ACCESS.TXT."  Attempted access from "Server Files:" results in
                              //404 File not Found!

                              char *filename = "/ACCESS.TXT";
                              strcpy(MyBuffer, filename);

                              // send a standard http response header
                              client.println("HTTP/1.1 200 OK");
                              client.println("Content-Type: text/plain");
                              client.println("Content-Disposition: attachment");
                              //client.println("Content-Length:");
                              client.println();

                              readFile();

                              end();

                         }
                         else
                         {

                              delay(1000);

                              // everything else is a 404
                              client.println("HTTP 404 Not Found");
                              client.println("Content-Type: text/html");
                              client.println();
                              client.println("<h2>404</h2>");
                              delay(250);
                              client.println("<h2>File Not Found!</h2>");
                              client.println("<br><br>");
                              client.println("<a href=http://" + publicIP + ":" + LISTEN_PORT + "/SdBrowse    >Return to File Browser</a><br>");

                              Serial.println("Http 404 issued");

                              error = 1;

                              end();

                         }

                    }
                    else
                    {
                         // Unsupported action, respond with an HTTP 405 method not allowed error.
                         Serial.print(action);
                         client.println("HTTP Method Not Allowed");
                         client.println("");
                         Serial.println("");
                         Serial.println("Http 405 issued");
                         Serial.println("");

                         digitalWrite(online, HIGH);   //turn-on online LED indicator

                         error = 2;

                         end();

                    }

               }

          }

     }  //if client

}


//********************************************************************
//////////////////////////////////////////////////////////////////////
// Return true if the buffer contains an HTTP request.  Also returns the request
// path and action strings if the request was parsed.  This does not attempt to
// parse any HTTP headers because there really isn't enough memory to process
// them all.
// HTTP request looks like:
//  [method] [path] [version] \r\n
//  Header_key_1: Header_value_1 \r\n
//  ...
//  Header_key_n: Header_value_n \r\n
//  \r\n
bool parseRequest(uint8_t* buf, int bufSize, char* action, char* path)
{
     // Check if the request ends with \r\n to signal end of first line.
     if (bufSize < 2)
          return false;

     if (buf[bufSize - 2] == '\r' && buf[bufSize - 1] == '\n')
     {
          parseFirstLine((char*)buf, action, path);
          return true;
     }
     return false;
}


///////////////////////////////////////////////////////////////////
// Parse the action and path from the first line of an HTTP request.
void parseFirstLine(char* line, char* action, char* path)
{
     // Parse first word up to whitespace as action.
     char* lineaction = strtok(line, " ");

     if (lineaction != NULL)

          strncpy(action, lineaction, MAX_ACTION);
     // Parse second word up to whitespace as path.
     char* linepath = strtok(NULL, " ");

     if (linepath != NULL)

          strncpy(path, linepath, MAX_PATH);
}

void logtoSD()   //Output to SPIFFS every fifthteen minutes
{


     getDate_Time();

     gpslat = gps.location.lat();
     gpslng = gps.location.lng();
     gpsalt = gps.altitude.feet();

     String logname;
     logname = "/LOG";
     logname += DATE; ////logname += Clock.getMonth(Century);
     logname += MONTH;   ///logname += Clock.getDate();
     logname += YEAR;
     logname += ".TXT";

     // Open a "log.txt" for appended writing
     File log = SPIFFS.open(logname.c_str(), FILE_APPEND);

     if (!log)
     {
          Serial.println("file 'LOG.TXT' open failed");
     }

     //log.print(id);
     //log.print(" , ");
     log.print("Latitude : ");
     log.print(gpslat, 5);
     log.print(" , ");
     log.print("Longitude : ");
     log.print(gpslng, 5);
     log.print(" , ");
     log.print(lastUpdate);
     log.print(" , ");


     log.print("Humidity:  ");
     log.print(hum, 1);
     log.print(" % , ");


     log.print("Temperature:  ");
     log.print(temp, 1);
     log.print(" C. , ");



     log.print("Pressure:  ");
     log.print(pressure, 1);
     log.print(" hpa. ");
     log.print(" , ");

     log.print(" RainDay ");
     log.print(rainDay, 1);
     log.print(" ,");

     log.print(" RainHour ");
     log.print(rainHour, 1);
     log.print(" , ");

     log.print(" Rain5min ");
     log.print(rain5min, 1);
     log.print(" mm. ");
     log.print(" , ");

     log.print("Elevation:  ");
     log.print(gpsalt, 0);
     log.print(" feet. ");
     log.println();
     //Increment Record ID number
     //id++;
     Serial.print("\n");
     //Serial.println("Data written to 'LOG.TXT'  " + dtStamp);
     Serial.println("Data written to  " + logname + dtStamp);
     Serial.println("\n");

     log.close();

}

///////////////
void readFile()
{

//     digitalWrite(online, HIGH);   //turn-on online LED indicator

     String filename = (const char *)&MyBuffer;

     Serial.print("File:  ");
     Serial.println(filename);


     File webFile = SPIFFS.open(filename);

     if (!webFile)
     {
          Serial.println("File" + filename + "failed to open");
          Serial.println("\n");
     }
     else
     {
          char buf[1024];
          int len;
          while  (len = webFile.read((uint8_t *)buf, 1024))
          {
               client.write((const char*)buf, len);
          }

          //delay(500);

          webFile.flush();

          webFile.close();

     }

     error = 0;

     delayTime = 1000;

     MyBuffer[0] = '\0';

//     digitalWrite(online, LOW);   //turn-off online LED indicator

     listen();

}

void seteeprom()
{
  
  eepromstring = String(rainDay,2);
  eepromSet("rainDay", eepromstring);

  rain5min = 0;

  eepromstring = String(rainHour,2);
  eepromSet("rainHour", eepromstring);

  eepromstring = String(rain5min,2);
  eepromSet("rain5min", eepromstring);

  
  //END - STORE RAINCOUNT IN EEPROM

}
////////////
void speak()
{

     char t_buffered1[14];
     dtostrf(temp, 7, 1, t_buffered1);

     char t_buffered2[14];
     dtostrf(hum, 7, 1, t_buffered2);

     char t_buffered3[14];
     dtostrf(pressure, 7, 1, t_buffered3);

     char t_buffered4[14];
     dtostrf(rain5min, 7, 1, t_buffered4);

     // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
     // pieces of information in a channel.  Here, we write to field 1.
     // Write to ThingSpeak. There are up to 8 fields in a channel, allowing you to store up to 8 different
     // pieces of information in a channel.  Here, we write to field 1.
     ThingSpeak.setField(1, t_buffered1);  //Temperature
     ThingSpeak.setField(2, t_buffered2);  //Humidity
     ThingSpeak.setField(3, t_buffered3);  //Barometric Pressure
     ThingSpeak.setField(4, t_buffered4);  //Dew Point F.

     // Write the fields that you've set all at once.
     //ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

     getDate_Time();

     Serial.println("");
     Serial.println("Sent data to Thingspeak.com  " + dtStamp + "\n");

     listen();

}



////////////////////////
float updateDifference()  //Pressure difference for fifthteen minute interval
{

     //Function to find difference in Barometric Pressure
     //First loop pass pastPressure and currentPressure are equal resulting in an incorrect difference result.
     //Future loop passes difference results are correct

     Serial.println("currentPressure:  " + (String)currentPressure);

     difference = currentPressure - pastPressure;  //This will be pressure from this pass thru loop, pressure1 will be new pressure reading next loop pass
     if (difference == currentPressure)
     {
          difference = 0;
     }

     return (difference); //Barometric pressure change in inches of Mecury

}

///////////////////
void webInterface()
{

     gpslat = gps.location.lat();
     gpslng = gps.location.lng();
     gpsalt = gps.altitude.feet();

     char glat[12]; // Buffer big enough for 9-character float
     dtostrf(gpslat, 6, 5, glat); // Leave room for too large numbers!

     char glng[12]; // Buffer big enough for 9-character float
     dtostrf(gpslng, 6, 5, glng); // Leave room for too large numbers!

     char cel[8];// Buffer big enough for 9-character float
     dtostrf(temp, 6, 1, cel); // Leave room for too large numbers!

     char humidity[8]; // Buffer big enough for 9-character float
     dtostrf(hum, 6, 1, humidity); // Leave room for too large numbers!

     char cpressure[10]; // Buffer big enough for 9-character float
     dtostrf(pressure, 6, 3, cpressure); // Leave room for too large numbers!

     char rain5[10]; // Buffer big enough for 9-character float
     dtostrf(rain5min, 6, 3, rain5); // Leave room for too large numbers!

     char rain60[10]; // Buffer big enough for 9-character float
     dtostrf(rainHour, 6, 3, rain60); // Leave room for too large numbers!

     char rain24[10]; // Buffer big enough for 9-character float
     dtostrf(rainDay, 6, 3, rain24); // Leave room for too large numbers!

     char alt[8]; // Buffer big enough for 9-character float
     dtostrf(gpsalt, 6, 1, alt); // Leave room for too large numbers!

     String data = "&last="                  +  (String)lastUpdate

                   + "&glat="                +  glat

                   + "&glng="                +  glng

                   + "&cel="                 +  cel

                   + "&humidity="            +  humidity

                   + "&cpressure="           +  cpressure

                   + "&rain5="               +  rain5

                   + "&rain60="              +  rain60

                   + "&rain24="              +  rain24

                   + "&alt="                 +  alt;




     if(WiFi.status()== WL_CONNECTED)
     {
          //Check WiFi connection status

          HTTPClient http;    //Declare object of class HTTPClient

          http.begin("http://YourDomainServer/dataCollector.php");      //Specify request destination
          http.addHeader("Content-Type", "application/x-www-form-urlencoded");  //Specify content-type header

          int httpCode = http.POST(data);   //Send the request
          String payload = http.getString();                  //Get the response payload

          if(httpCode == 200)
          {
               Serial.print("");
               Serial.print("HttpCode:  ");
               Serial.print(httpCode);   //Print HTTP return code
               Serial.print("  Data echoed back from Hosted website  " );
               Serial.println("");
               Serial.println(payload);    //Print payload response

               http.end();  //Close HTTPClient http
          }
          else
          {
               Serial.print("");
               Serial.print("HttpCode:  ");
               Serial.print(httpCode);   //Print HTTP return code
               Serial.print("  Domain website data update failed.  ");
               Serial.println("");

               http.end();   //Close HTTPClient http
          }

          listen();

     }
     else
     {

          Serial.println("Error in WiFi connection");

          listen();

     }


}

void wifi_Start()
{

     WiFi.disconnect();

     //delay(1000 * 10);

     WiFi.mode(WIFI_STA);

     //wifi_set_sleep_type(NONE_SLEEP_T);

     Serial.println();
     Serial.print("MAC: ");
     Serial.println(WiFi.macAddress());

     // We start by connecting to a WiFi network
     Serial.print("Connecting to ");
     Serial.println(ssid);

     WiFi.begin(ssid, password);



     //setting the addresses
     IPAddress ip(10,0,0,200);
     IPAddress gateway(10,0,0,1);
     IPAddress subnet(255, 255, 255, 0);
     IPAddress dns(75,75,76,76);

     WiFi.config(ip, gateway, subnet, dns);

     WiFi.waitForConnectResult();

     Serial.printf("Connection result: %d\n", WiFi.waitForConnectResult());

     ///////////////////////////////////  OTA  ///////////////////////////////
     /*use mdns for host name resolution*/
     if (!MDNS.begin(host))   //http://esp32.local
     {
          Serial.println("Error setting up MDNS responder!");
          while (1)
          {
               delay(1000);
          }
     }
     Serial.println("mDNS responder started");
     /*return index page which is stored in serverIndex */
     otaServer.on("/", HTTP_GET, []()
     {
          otaServer.sendHeader("Connection", "close");
          otaServer.send(200, "text/html", loginIndex);
     });
     otaServer.on("/serverIndex", HTTP_GET, []()
     {
          otaServer.sendHeader("Connection", "close");
          otaServer.send(200, "text/html", serverIndex);
     });
     /*handling uploading firmware file */
     otaServer.on("/update", HTTP_POST, []()
     {
          otaServer.sendHeader("Connection", "close");
          otaServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
          ESP.restart();
     }, []()
     {
          HTTPUpload& upload = otaServer.upload();
          if (upload.status == UPLOAD_FILE_START)
          {
               Serial.printf("Update: %s\n", upload.filename.c_str());
               if (!Update.begin(UPDATE_SIZE_UNKNOWN))   //start with max available size
               {
                    Update.printError(Serial);
               }
          }
          else if (upload.status == UPLOAD_FILE_WRITE)
          {
               /* flashing firmware to ESP*/
               if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
               {
                    Update.printError(Serial);
               }
          }
          else if (upload.status == UPLOAD_FILE_END)
          {
               if (Update.end(true))   //true to set the size to the current progress
               {
                    Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
               }
               else
               {
                    Update.printError(Serial);
               }
          }
     });

     otaServer.begin();

////////////////////////////////  End OTA  ///////////////////////////////////////

     // Starting the web server
     server.begin();
     Serial.println("Web server running. Waiting for the ESP IP...");

     delayTime = 500;

     started = 1;   //Server started

     // Printing the ESP IP address
     Serial.print("Server IP:  ");
     Serial.println(WiFi.localIP());
     Serial.print("Port:  ");
     Serial.print(LISTEN_PORT);
     Serial.println("\n");





}
