# ArduinoSolderingIron
ORIGINAL CODE BY TECHBUILDER. MODDED BY SYNCHRONOUS 
This project is inspired by techbuilder's take on a hakko type soldering iron control circuit.
The code was taken from techbuilder's video but it has been immensely modified and improved. 

## Features: 
 - Great temperature control without PID.
 - Automatic Sleep mode.
 - Easy, capable and modifiable design.
 - Uses I2C ssd1306 based OLED.

### Modifiable Parameters: 
 - minTemp (minimum temp to be acquired by the iron)
 - maxTemp (max temp that can be aquired by the iron)
 - minADC  (minimum acquired ADC during testing, can be found using math or by experimentation)
 - maxADC  (maximum acquired ADC during testing, can be found using math or by experimentation)
 - oledInterval (display refresh rate)
 - wakeTime (seconds, how long the iron waits before going to sleep)
 - sleepTemp (temperature that iron holds during sleep)
 - thresholdTemp (temperature above which the sleep function activates)
 - maxPWM (set this lower if the supply you use is not capable of 3 amps or heavy spikes)

### What you need to do to get the project working: 
You can watch https://www.youtube.com/watch?v=gd2W-boIRPo&t=10s&pp=ygULdGVjaGJ1aWxkZXI%3D and follow
the general testing and calculations part to get the values you need to put in the code. 

Breadboard the circuit and upload the code but
!!!!DONT KEEP THE POTENTIOMETER ABOVE 0 WHEN UPLOADING CODE IF IRON POWER IS NOT CONNECTED.!!!!

Or you will end up burning the mosfet controlling the iron, usually for uploading new code either removing the
fet is recommended or pull down pin D4 TO GROUND (connect to ground) before connecting the circuit to the laptop
when the circuit is off. this will set maxPWM to 0. after uploading, disconnect D4 from ground and repower the circuit ENSURING THE IRON POWER SUPPLY IS CONNECTED. 
Alternatively, You can keep the iron supply on before connecting to the PC, that way you dont have to pull D4 to ground. 

### Common errors and troubleshooting: 
#### Set temp not changing or increasing with temperature?
Check if the middle pin of the pot is connected to the proper arduino pin (A6 default).
If it still doesn't, your potentiometer might be dead. 

#### Iron heating time too long?
Use a higher voltage power supply < 24v.
See if the Solder connections on your PCB are thick enough. Next, Check if the iron connctor heats up, if it does, 
Clean up the connectors, the performance increase depends on how filthy your connectors were. 

#### Display orientation messed up? 
Try removing the display.setRoatation(250) line. (in void setup)

#### Display not working? 
Check the datasheet for your OLED, the code works only with ssd1306 I2C, if you have another display (like ssh1106)
you must modify the code and use a different library.


## V1.1 IMPROVEMENTS OVER ORIGIONAL CODE:
Changed the display from 16x2 I2C LCD to 128x32 ssd1306 i2c oled 
display.

Added sleep function(default: 5 minutes). iron switches to sleep mode 
when the set temp is 200+(default) and wakeTime variable times out.

Fixed an issue with the display refresh rate dropping and program 
slowing down after ~30 seconds from startup(previousMillis variable
overflow). Now it slows down after ~50 days.

General improvements in original code.

Added a Status LED to show iron status(default: on for running, fade for sleeping).
