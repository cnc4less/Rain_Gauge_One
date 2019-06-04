//Rain_Gauge_One.ino

//Original code by jurs od Arduino.cc forum for use with "tipping bucket rain gauge.
//jurs code modified by William Lucid, Github.com/tech500.


#define FIVEMINUTES (30*1000L) // 1 minutes are 600000 milliseconds

#define REEDPIN 32
#define REEDINTERRUPT 0 

int count;
int hourCount;

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

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println("Total\tCurrent");

  pinMode(REEDPIN,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(REEDPIN),reedSwitch_ISR, FALLING);  // <----- changed to this line
}

unsigned long lastSecond,last5Minutes;
float lastPulseCount;
int currentPulseCount;
float rain5min;
float rainHour;
float rainDay;

void loop() 
{
  
  float rainFall;
  
  // each second read and reset pulseCount_ISR
  if (millis()-lastSecond>=1000) 
  {
      lastSecond+=1000;
      portENTER_CRITICAL(&mux);
      currentPulseCount+=pulseCount_ISR; // add to current counter  <--changed to this line
      pulseCount_ISR=0; // reset ISR counter
      rainFall = currentPulseCount * .047;
      portEXIT_CRITICAL(&mux);
            
      Serial.print("Rainfall:  " + (String)rainFall);Serial.print('\t');Serial.print("Dumps during 5 Minutes:  " + (String)currentPulseCount);
  
      Serial.println();
  }

    // each 5 minutes save data to another counter
    if (millis()-last5Minutes>=FIVEMINUTES) 
    {
  
      count++;
      
      float daysRain;
  
      rain5min = rainFall;
      rainHour = rainHour + rainFall;  //accumulaing 5 minute rainfall for 1 hour then reset -->rainHour Rainfall 
      rainDay = rainDay + rainFall;  //Only 1 hour
      daysRain = daysRain + rainDay  ;  //accumulaing every hour rainfall for 1 day then reset -->day total
      //Serial.println("5 Minute Rainfall:  " + (String)rain5min);
      Serial.println("Hourly rainfall rate:  " + (String)rainHour);
      Serial.println("60 Minute Accumulating:  " + (String)rainHour);
      Serial.println("Day Rainfall:  " + (String)rainDay);
      last5Minutes+=FIVEMINUTES; // remember the time
      lastPulseCount+=currentPulseCount; // add to last period Counter
      currentPulseCount=0;; // reset counter for current period
  
      Serial.println("count:  " + (String)count);
  
           if(count == 3) //End of 1 hour  (for testing); otherwise, count = 20 for real 1 hour.)
           {
               rainFall = 0;
               rainHour = 0;
           }
           
           if (count == 6)  //End of 1 day  (for testing); otherwise, count = 24 for real 1 hour.)
           {
              rainFall = 0;
              rainHour = 0;
              rainDay = 0;
              daysRain = 0;
              count = 0;		   
            
          }
    }
}
