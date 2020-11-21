#include <FastLED.h>    //Includes library to control LEDs
#define NUM_LEDS 85     //Number of LEDs to be lit up
#define MAXNUM_LEDS 150 //how many LEDs are in the strip
#define DATA_PIN 6      //Green wire from LED strip
CRGB leds[MAXNUM_LEDS]; //Creates array for the LEDs

// different pin connections
int strobePin  = 12;    // Strobe Pin on the MSGEQ7 - Green wire
int resetPin   = 8;    // Reset Pin on the MSGEQ7 - Orange wire
int outPin     = A5;   // Output Pin on the MSGEQ7 - White wire
int level[7];          // An array to hold the values from the 7 frequency bands

// used to calculate the bass
int totalBass;        //Will hold total bass value for computing average
int averageOfBass = 0;    // Will hold the average value of the first two frequencies
float bassToMusic;       // Will hold how large bass is proportional to the music

// used to calculate the highs
int totalHighs;           //Will hold the total high values for computing average
int averageOfHighs;      //Will hold the average value the sixth frequency
float highToMusic;       //Will hold how large the highs are proportional to the music  

//used to calculate the mids
int totalMids;           //Will hold the total high values for computing average
int averageOfMids;      //Will hold the average value the sixth frequency
float midsToMusic;       //Will hold how large the highs are proportional to the music  


int iterationsBeforeAverage = 21;     //How many times to calculate average; 21 seems to be the sweet spot

// used for error checking
int differenceBetweenBass = 0;
float lastAverage = 0.0;
float lastMidAverage = 0.0;
int bassError = 10;
bool highsAreHigher = false;

// used for automatics gain controller / Karn's Algorithm
//float scale = 1.0; //used for the Automatic Gain Controller
float alpha = 0.125;
int currentAverage = 0;
int lastRollingAverage;
int rollingAverageOfFrequencies[7];
int rollingAverageOfMusic = 0;
int totalOfRollingAverages = 0;

// used for base measurements
float minBass = 0;
float minMids = 0;
float minHighs = 0;

void setup() {
  //Start the serial monitor
  Serial.begin (9600);

  // On the data pin, tells code that it's using the Neopixel chipset and initiates the array
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, MAXNUM_LEDS);
 
  //Define pins
  pinMode      (strobePin, OUTPUT);
  pinMode      (resetPin,  OUTPUT);
  pinMode      (outPin,    INPUT);
 
  // Create an initial state for the pins
  digitalWrite (resetPin,  LOW);
  digitalWrite (strobePin, LOW);
  delay        (1);
 
  // Reset the MSGEQ7 as per the datasheet 
  digitalWrite (resetPin,  HIGH);
  delay        (1);
  digitalWrite (resetPin,  LOW);
  digitalWrite (strobePin, HIGH);
  delay        (1);

  //reset all the led lights
  for(int i = 0; i < MAXNUM_LEDS; i++){
    leds[i].setRGB(0, 0, 0);
  }

  //Variables used for finding the minimum average
  int minTotalForBass = 0;
  int counterOfIBass = 0;

  int minTotalForMids = 0;
  int counterOfIMids = 0;

  int minTotalForHighs = 0;
  int counterOfIHighs = 0;

  //get data to calculate mins for each frequency
  for(int j = 0; j < 50; j++){
    for(int i = 0; i < 7; i++){
      //used to cycle through the different frequencies
      digitalWrite       (strobePin, LOW);
      delayMicroseconds  (80);                    // Delay necessary due to timing diagram
      level[i] =         analogRead (outPin);
      digitalWrite       (strobePin, HIGH);
      delayMicroseconds  (80);                    // Delay necessary due to timing diagram  

      //find all the bass numbers and find the average without any music
      if(i == 0 || i == 1){
          minTotalForBass = minTotalForBass + level[i];
          counterOfIBass++;
      }

      //find all the bass numbers and find the average without any music
      if(i == 2 || i == 3){
          minTotalForMids = minTotalForMids + level[i];
          counterOfIMids++;
      }     

      //find all the bass numbers and find the average without any music
      if(i == 4 || i == 5 || i == 6){
         minTotalForHighs = minTotalForHighs + level[i];
         counterOfIHighs++;
      }     
    }
  }

  //calculate the minimums
  float minBass = minTotalForBass/counterOfIBass;
  float minMids = minTotalForMids/counterOfIMids;
  float minHighs = minTotalForHighs/counterOfIHighs;

  //used to calculate the averages and from that average it maps how much that value would be on a scale from 0-255
  minBass = (256./1023.)*minBass;

  minMids = (256./1023.)*minMids;

  minHighs = (256./1023.)*minHighs;

  for(int i = 0; i < 7; i++){
    if(i == 0 || i == 1){
      rollingAverageOfFrequencies[i] = minBass;
    }

    if(i == 2 || i == 3){
      rollingAverageOfFrequencies[i] = minMids;
    }

    if(i == 4 || i == 5 || i == 6){
      rollingAverageOfFrequencies[i] = minHighs;
    }
  }

  for(int i = 0; i < 7; i++){
    rollingAverageOfMusic = rollingAverageOfMusic + rollingAverageOfFrequencies[i];
    //Serial.println(rollingAverageOfFrequencies[i]);
  }
  //Serial.println("Average");
  //Serial.print(rollingAverageOfMusic);
}

void loop() {

  //checks to see if lastaverage was used, if so then use bool to compare new value to later
  bool wasLastAverageHigh;
  if(lastAverage > 18.0){
    wasLastAverageHigh = true;
  }
  else{
    wasLastAverageHigh = false;
  }

  //Checks to see how large the difference is between Mid levels; used to prevent inaccurate readings
  bool isDifferenceOfMidHigh = false;
  float differenceOfMid = lastMidAverage - midsToMusic;
  if(differenceOfMid >= 5.0 || differenceOfMid <= -5.0){
    isDifferenceOfMidHigh = true;
  }

  //Values used later to compare new values to 
  lastMidAverage = midsToMusic;
  lastAverage = bassToMusic; 

  //Used for error checking highs
  highsAreHigher = false;

  //initialize new values for Bass, mids and highs
  totalBass = 0;
  totalHighs = 0;
  totalMids = 0;
  currentAverage = 0;
  lastRollingAverage = currentAverage;
  
  // Cycle through each frequency band by pulsing the strobe.
  // Outer for loop - how many iterations before adding to the different levels
  // Inner for loop - adds value of the bass 
  for(int j = 0; j < iterationsBeforeAverage; j++){
    for (int i = 0; i < 7; i++) {
      //used to cycle through the different frequencies
      digitalWrite       (strobePin, LOW);
      delayMicroseconds  (80);                    // Delay necessary due to timing diagram
      level[i] =         analogRead (outPin);
      digitalWrite       (strobePin, HIGH);
      delayMicroseconds  (80);                    // Delay necessary due to timing diagram  

      //totalBass is the amount of bass during the iterations
      if(i == 0 || i == 1){
        totalBass = totalBass + level[i];
      }

      //totalHighs is the amount of highs during the iterations
      if(i == 4 || i == 5 || i == 6){
        totalHighs = totalHighs + level[i];
      }

      //totalMids is the amount of mids during the iterations
      if(i == 2 || i == 3){
        totalMids = totalMids + level[i];
      }      

      //used for calculcating scale
      rollingAverageOfFrequencies[i] = alpha * level[i] + (1 - alpha) * rollingAverageOfFrequencies[i];
      //following used for debugging
      //Serial.print       (rollingAverageOfFrequencies[i]);
      //Serial.print       ("   ");
    }
      //Serial.println ();
  
   }

  
   totalOfRollingAverages = 0;
   for(int i = 0; i < 7; i++){
    totalOfRollingAverages = totalOfRollingAverages + rollingAverageOfFrequencies[i];
   }
   
  float averageOfRollingAverages = totalOfRollingAverages/7;
  rollingAverageOfMusic = alpha * averageOfRollingAverages + (1 - alpha) * rollingAverageOfMusic;
  //Serial.println(rollingAverageOfMusic);
  float scalar = (1023 - rollingAverageOfMusic)/rollingAverageOfMusic;
  Serial.println(scalar);

  //sometimes values are negative due to inaccurate reading from chip, but the absolute value of that number is still accurate
  if(totalBass < 0){
    totalBass = totalBass * -1;
  }
  if(totalHighs < 0){
    totalHighs = totalHighs * -1;
  }
  if(totalMids < 0){
    totalMids = totalMids * -1;
  }
  if(currentAverage < 0){
    currentAverage = currentAverage * -1;
  }  

  //used to calculate the averages and from that average it maps how much that value would be on a scale from 0-255
  averageOfBass = totalBass/(2*iterationsBeforeAverage);  
  bassToMusic = (256./1023.)*averageOfBass;

  averageOfHighs = totalHighs/(3*iterationsBeforeAverage);
  highToMusic = (256./1023.)*averageOfHighs;

  averageOfMids = totalMids/(2*iterationsBeforeAverage);
  midsToMusic = (256./1023.)*averageOfMids;

  

  //used to determine if mid lights should display mids or highs
  if(highToMusic + 30 > midsToMusic){
    highsAreHigher = true;
  }
  else{
    highsAreHigher = false;
  }

  

  //used to calculate what the LEDs should look like 
  for(int i = 0; i < NUM_LEDS; i++){

    //Serial.println(bassToMusic);

      //sets first third of led strip to green - lows
      if(i < NUM_LEDS/3 - 2){
        leds[i].setRGB(20, 220, 70);
        if(i > NUM_LEDS/3){
           leds[i].setRGB(0, 0, 0);
        }
        if(bassToMusic > minBass && lastAverage > 28.5){
          leds[i].fadeLightBy(215-bassToMusic);
         }  
         else{
          leds[i].fadeLightBy(190);
        }    
      }

      //sets second third of led strip to blue or yellow - mids/highs
      if(i < (NUM_LEDS * 2 /3) + 2 && i >= (NUM_LEDS/3) - 2){
          leds[i].setRGB(230, 0, 30);
          //maybe use the above bool isDifferenceOfMidHigh to detect a muffled sound
          if(midsToMusic > minMids || (lastMidAverage > 18.0 && wasLastAverageHigh == true)){
             leds[i].fadeLightBy(200-midsToMusic); 
             //Serial.print("Here 1");
          }
          else{
            if(lastMidAverage < 28.0){
              leds[i].fadeLightBy(200);
              //Serial.print("Here 2");
            }  
          }  
          if(highsAreHigher == true && highToMusic >= 45.0 && midsToMusic < minMids){    
             leds[i].fadeLightBy(200-highToMusic);
             //Serial.print("Here 3");   
          }
       }

       //Serial.println();
    
      //sets last third of led strip to green - lows
      if(i < (NUM_LEDS) && i >= (NUM_LEDS * 2/3) + 2){
         leds[i].setRGB(20, 220, 70);
         if(i > NUM_LEDS){
           leds[i].setRGB(0, 0, 0);
         }
         if(bassToMusic > minBass && lastAverage > 28.5){
          leds[i].fadeLightBy(215-bassToMusic);
         }  
         else{
          leds[i].fadeLightBy(190);
        }    
      }   
    }   

    //displays the LEDs
 FastLED.show();
}
