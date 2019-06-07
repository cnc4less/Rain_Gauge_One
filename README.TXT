Features of "Weather_View.ino".  Testing version.

1. RTC with GPS update; used for 15 minute time interval, date-time stamping, and dayofweek; 
every 6th day, "LOG.TXT" file gets renamed, every Saturday "LOG.TXT" to keep file size 
manageable.  Rrenamed in the format "logxxyy" xx being the month and yy being the date; a 
new log.txt is created after every file renaming. 

2. Dynamic web page of current observations for Last update time and date, humidity, dew
point, temperature, heat index, barometric pressure (both inches of Mercury and millibars.) 

3. Server files are listed as web links; clicking link prompts for "Open with/Save as."
"FAVICON.ICO", and "ACCESS.TXT" are listed; however, they are for internal use and are restricted.
"Opened with/Save as," result of clicking link produces "404 Page not found."

4. LOG.TXT file is appended every 15 minutes with the latest update; storing data from Dynamic
web page.

5. DIFFER.TXT stores the difference in Barometric Pressure for the last fifteen minutes. Only
a difference of equal to or greater than .020 inches of Mercury are logged with difference,
date and time.

6. URL file names other than ones defined in the Sketch produce "404 Page not found." Methods   
other than "GET," produce "405" message and exits current request.

7.. Audible alert from Piezo electric buzzer when there is Barometric Pressure difference of
.020 inches of Mercury. I am interested in sudden drop of Barometric Pressure in a 15 minute
interval. Serve weather more likely with a sudden drop. Difference of .020 inches of Mercury
point is set for my observations to log and sound audible alert; not based on any known value
to be associated with serve weather.

8. Optional; two-line LCD Display of Barometric Pressure, in both inches of Mercury and millibars.

9. Tempature, Humidity, Barometric Pressure, and Dew Point have four embedded "ThinkSpeak.com"
graphs on one web page. Graphs are created from Iframes provided by "ThingSpeak.com"

10. ESP8266HTTPClient library is used to POST data to the hosted, Domain website.  Function 
webInterface sends data to hosted, domain website.

11. Free, "000webhost powered by HOSTINGER" is used for "Domain hosted" website.

12. "dataCollector.php" needs to be uploaded to your Hosted web site.

13. Simple FTP file transfer; allows for easy file maintence, preventing too many log log files.
Works with Filezilla client.

14. Built-in OTA update; remote sketch update by web interface.

Two websites,one sketch: "Weather_view.ino"

ESP8266 Server https://bit.ly/2FiuNE5--this has a file browser, selected file 
can be downloaded.

Domain Hosted website:  https://bit.ly/2M5NBs0  --no file browser due to hosted, 
domain server restrictions (free hosting service). Hosted server "sleeps" one hour every 24 
hours.
-----------------------------------------------------------------------------------
Note this is a project is in development; maybe offline or log files may be affected.
Server is online 24/7; except during periods of testing.

Server is a "Wemos WiFi D1 R2" with 32 MiB Flash Memory. Development Board,  and GY-BME280 breakout board, purchased from "Ebay.com" ; both are required for
project. Sensor is located indoors, currently.

Developement board is Arduino friendly; can be programed using the Arduino IDE.
Under $15.00 with shipping, is the project cost for Development Board and one,
GY-BME280 breakout board, sensor for Barometric Pressure, Humidity, and Temperature.
