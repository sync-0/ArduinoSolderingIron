/*
--------------------ARDUINO CONTROLLED SOLDERING IRON--------------------


ORIGINAL CODE BY TECHBUILDER. MODDED BY SYNCHRONOUS 

Features: 
 - Great temperature control without PID.
 - Automatic Sleep mode.
 - Easy, capable and modifiable design.
 - Uses I2C ssd1306 based OLED.

V1.1 IMPROVEMENTS OVER ORIGIONAL CODE:
-> changed the display from 16x2 I2C LCD to 128x32 ssd1306 i2c oled 
   display.
-> added sleep function(default: 5 minutes). iron switches to sleep mode 
   when the set temp is 200+(default) and wakeTime variable times out.
-> fixed an issue with the display refresh rate dropping and program 
   slowing down after ~30 seconds from startup(previousMillis variable
   overflow). Now it slows down after ~50 days
-> general improvements in original code
-> added a Status LED to show iron status(default: on for running, fade for sleeping)

*/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define tempSensor A7     //Temperature sensor input pin
#define knob A6           //Potentiometer input pin
#define moveSensor 2      //Movement sensor input pin
#define iron 3            //Iron output pin
#define programPin 4      //Program pin as input pin 
#define IronLED 5         //Iron power indicator led as output pin
#define statusLED 6       //Status pin for showing modes
#define I2Caddress 0x3C   //I2C address for display. Usually 0x3c
#define displayWidth 128  //width of your I2C display
#define displayHeight 32  //height of your I2C display
Adafruit_SSD1306 display(displayWidth, displayHeight, &Wire, -1);


const int
minTemp = 29,       //Minimum acquired iron tip temp during testing phase (°C)
maxTemp = 450,      //Maximum acquired iron tip temp during testing phase (°C)
minADC  = 267,      //Minimum acquired ADC value during minTemp testing
maxADC  = 701,      //Maximum acquired ADC value during minTemp testing

avgCounts = 5,       //Number of samples to take the average of for displaying
oledInterval = 80,   //OLED refresh rate (milliseconds)

wakeTime = 300,      //how long the iron sits on untouched before going to sleep (in seconds)
sleepTemp = 150,     //Sleep temperature
thresholdTemp = 200; //Temperature above which sleep function turns on

int
maxPWM    = 255,     //Maximum PWM Power
setTempAVG = 0,      //System Variable (for display)
currentTempAVG = 0,  //System Variable (for display)
counter = 0,         //System Variable (for display)
tempRAW = 0,         //System Variable (for display & iron)
knobRAW = 0,         //System Variable (for display & iron)
pwm = 0,             //System Variable (for iron)
brightnessLED = 0,   //System Variable (for sleep mode indicator)
fadeRate = 15;       //System Variable (for sleep mode indicator)

volatile int currentWaketime = wakeTime; //System Variable (for sleep mode)

volatile bool wakeState = 1; //System Variable (for sleep mode)
bool programMode = 0; //System Variable (for programming mode)

unsigned long 
previousMillis = 0, //System Variable (for display)
previousMillis1 = 0;//System Variable (for sleep mode)

float
currentTemp = 0.0,  //System Variable (for iron)
setTemp = 0.0,      //System Variable (for iron)
store = 0.0,        //System Variable (for display)
knobStore = 0.0;    //System Variable (for display)

void wakeReset(){  //the Interrupt service routine, used by other parts of program too. when iron moved, reset the currentwakeTime and wakeState
  currentWaketime = wakeTime;  
  wakeState = 1;
}


void setup(){
  pinMode(tempSensor, INPUT);        //Set Temp Sensor pin as INPUT
  pinMode(knob, INPUT);              //Set Potentiometer Knob as INPUT
  pinMode(iron, OUTPUT);             //Set MOSFET PWM pin as OUTPUT
  pinMode(IronLED, OUTPUT);          //Set IronLED pin as OUTPUT
  pinMode(statusLED, OUTPUT);        //Set Status pin as OUTPUT

  pinMode(moveSensor,INPUT_PULLUP);  //Set iron movement sensor pin as INPUT
  pinMode(programPin, INPUT_PULLUP); //Set programPin as INPUT
  // Serial.begin(9600);         // DEBUG 
  

  attachInterrupt(digitalPinToInterrupt(moveSensor), wakeReset, LOW); //Run ISR when the move sensor pin goes low
  display.begin(SSD1306_SWITCHCAPVCC, I2Caddress);
  display.clearDisplay();
  display.setRotation(250); //Remove this line if your display is flipped upside down

  for(int x = 0; x < (displayWidth+1); x += 2) //Cool startup animation
  {
    display.drawPixel(x, 16, WHITE);
    display.drawPixel(x+1, 16, WHITE);

    display.drawPixel(x, 17, WHITE);
    display.drawPixel(x+1, 17, WHITE);

    display.drawPixel(x, 18, WHITE);
    display.drawPixel(x+1, 18, WHITE);

    display.display();
    delay(1); //for stability 
  }
  
  programMode = !digitalRead(programPin); 
  if(programMode == 1){ // If the program switch has been turned on, i.e. device is in programming mode,
    maxPWM = 0;         // set pwm to 0 so you dont fry your FET
  }

  display.clearDisplay();
  display.display();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  delay(250);
  
}

void loop(){


  ////--------GATHER SENSOR DATA--------////

  knobRAW = analogRead(knob);  //Get analog value of Potentiometer
  setTemp = map(knobRAW, 0, 1023, minTemp, maxTemp);  //Scale pot analog value into temp unit

  tempRAW = analogRead(tempSensor);  //Get analog value of temp sensor
  currentTemp = map(tempRAW, minADC, maxADC, minTemp, maxTemp);  //Scale raw analog temp values as actual temp units

  ////----------SLEEP TIME CHECKER----------////

  unsigned long currentMillis1 = millis();//acquire the current runtime of the program
  if(setTemp < thresholdTemp){  //if set temperature is below 200C (thresholdTemp)
    wakeReset();  //reset the wake timer 
  }
  else if(currentMillis1 - previousMillis1 >= 1000 ){  //else if 1 second has passed and setTemp is above 250C
    previousMillis1 = currentMillis1;  //set previous runtime to current runtime 
    currentWaketime -= 1;  //and subtract 1 second from current waketime

    if(currentWaketime <= 0 && programMode == 0){  //if the current waketime has decreased to zero or less and program mode is off
      wakeState = 0;  //set wakeState to zero. i.e. go to sleep
      
    }
  }


  ////--------SOLDERING IRON POWER CONTROL--------////
  if(wakeState == 0){     //if wakeState=0, i.e. iron in sleep mode
    setTemp = sleepTemp;  //set the setTemp to sleepTemp 
  }

  if(knobRAW < 20){  //Turn off iron when knob is at its low (IRON OFF)
    digitalWrite(IronLED, LOW);
    pwm = 0;
  }
  else if(currentTemp <= setTemp && !programMode ){  //Turn on iron when iron temp is lower than preset temp and iron isnt in program mode 
    digitalWrite(IronLED, HIGH);
    pwm = maxPWM;
  }
  else{  //Turn off iron when iron temp is higher than preset temp
    digitalWrite(IronLED, LOW);
    pwm = 0;
  }
  analogWrite(iron, pwm);  //Apply the acquired PWM value from one of the three cases above


  //--------GET AVERAGE OF CURRENT TEMP AND KNOB SET TEMP--------//

  if(counter < avgCounts){  //Sum up temp and knob data samples
    store = store + currentTemp;
    knobStore = knobStore + setTemp;
    counter++;
  }
  else{
    currentTempAVG = (store / avgCounts) - 1;  //Get temp mean (average)
    setTempAVG = (knobStore / avgCounts);  //Get knob - set temp mean (average)
    knobStore = 0;                         //Reset storage variable
    store = 0;                             //Reset storage variable
    counter = 0;                           //Reset storage variable

  }


  //--------DISPLAY DATA--------//

  unsigned long currentMillis = millis(); //Use and acquire millis function instead of using delay
  if (currentMillis - previousMillis >= oledInterval){ //Display will only display new data every n milliseconds intervals
    previousMillis = currentMillis;  //set previous runtime to current runtime 
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);

    if (programMode == 1){
      display.setTextSize(1);
      display.println("PROGRAMMING...");
    }

    else if(knobRAW < 20){ //if the knob is near 0, display that the iron is off

      display.println("IRON OFF ");
    }

    else if(wakeState == 0){  //else if iron is in sleep mode, display sleeping and sleep temp;
      display.setTextSize(1);
      display.println("SLEEPING");
      display.setCursor(0, 9);
      display.print("SET:");
      display.print(setTempAVG, 1);
      display.write(247);          // Degree symbol (might differ for you)
      display.println("C ");     

    }

    //if not, then display the temp set by the knob
    else{
      
      display.setCursor(0,0);      //set the cursor coordinate
      display.print("SET: ");
      display.print(setTempAVG, 1);
      display.write(247);          // Degree symbol
      display.println("C ");
    }
    
    // if the current temp is almost room temperature, display that the tip is cool
    if(currentTemp < minTemp + 10){
      display.setCursor(0,18);
      display.println("TIP COOL ");
    }
    //if not, then display the current iron temp 
    else{
      display.setCursor(0,18);        //set the cursor coordinate
      display.print("TMP: ");
      display.print(currentTempAVG, 1);
      display.write(247);             // Degree symbol
      display.println("C ");
    }
    display.display();   //update the display

    if(wakeState == 1){   //if iron is awake
      digitalWrite(statusLED, HIGH);  //turn on status LED
    }

    else{
      analogWrite(statusLED, brightnessLED);
      brightnessLED += fadeRate;

      if(brightnessLED <= 0 || brightnessLED >= 255){
        fadeRate = -fadeRate;
      }
    }
  }
}