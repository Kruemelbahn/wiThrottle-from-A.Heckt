/*
 * Free Scale Model Development
 * This is part of the FSMD Project
 *
 * Copyright (c) 2020 Andreas Heckt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public SW_LICENSE as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public SW_LICENSE for more details.
 *
 * You should have received a copy of the GNU General Public SW_LICENSE
 * along with this program. If not, see <http://www.gnu.org/SW_LICENSEs/>.
 */

/*
 * WiTrottle for JMRI
 *
 * Inspired by the Simple Wifi Throttle project by Geoff Bunza and 
 * modifications by Phil Abernathy. as well as by the wiFred project 
 * by Heiko Rosemann
 *
 * Designed to be used with ESP32 board 'SparkFun Thing Plus - ESP32 WROOM'.
 *
 * Ressources
 * [1]: https://www.jmri.org/help/en/package/jmri/jmrit/withrottle/Protocol.shtml
 * [2]: https://model-railroad-hobbyist.com/node/35652
 * [3]: https://newheiko.github.io/wiFred/documentation/html/docu.html
 * [4]: https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
 */

/*
 * modifications by Michael Zimmermann
 */

// original compiliert mit https://github.com/espressif/arduino-esp32@V1.0.6
//     hier compiliert mit https://github.com/espressif/arduino-esp32@V2.0.9 (04.05.2023)
 
#include "CrossFunc.h"
#include "VirtualLoco.h"
#include "WiThrottle.h"
#include <Arduino.h>


// WiFi settings
extern wiFiConfig wiFiSettings;

// WiThrottle server settings
extern hostConfig hostSettings;

// WiThrottle
WiThrottle throttle((char*)"ESP32 WiThrottle");

#ifdef HL_DISP
	#include <Adafruit_GFX.h>
	#include <Adafruit_SSD1306.h>
	
	// I2C OLED display
  extern Adafruit_SSD1306 display;
  extern unsigned char imgFWD7x7[];
  extern unsigned char imgREV7x7[];
#endif

#ifdef ROT_ENCODER
  // Rotary encoder
	#include <AiEsp32RotaryEncoder.h>   // https://github.com/igorantolic/ai-esp32-rotary-encoder
	AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ENC_CLK, ENC_DT, ENC_BTN, ENC_PWR);
#else
	#include <MedianFilter.h>   // https://github.com/daPhoosa/MedianFilter
	MedianFilter notchFiltered(40, 0);          // Filter the potentiometer values for proper notching
#endif

// Virtual loco
VirtualLoco activeLoco;
unsigned long notchTime;                    // Timestamp of last transmisson of reference notch to WiThrottle server

// Function buttons
unsigned int btnFctCount = 0;               // Number of function buttons used in hardware setup
unsigned int btnFctPin[] = { BTN_FCT_01, BTN_FCT_02, BTN_FCT_03, BTN_FCT_04, BTN_FCT_05, BTN_FCT_06, BTN_FCT_07, BTN_FCT_08, BTN_FCT_09, 0 }; // added '0' for safe limitation in "setup()"
                                            // Ordered list of input pins
unsigned int btnFctFn[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
                                            // this will map the pin list to actual functions

// LED for indicating the loco's direction
unsigned int ledDirPin[] = { LED_REV, LED_FWD };
                                            // Ordered list of output pins


// Menu
String Menu_lvl_1[] = { "Loco", "Function", "Layout", "Throttle" };


// Booting sequence
void setup() {
  unsigned int address;                     // DCC address

  // Start serial communication
  Serial.begin(115200);
  #ifdef DEBUG
    Serial.println("--->\nStart\n--");
  #endif

  // Define input and output pins
  pinMode(DIR_SW, INPUT);
  pinMode(BTN_STOP, INPUT_PULLUP);
  while (btnFctPin[btnFctCount] != 0) {
    pinMode(btnFctPin[btnFctCount], INPUT_PULLUP);
    btnFctCount++;
  }
  pinMode(BTN_FCT_SH, INPUT_PULLUP);

  pinMode(LED_STOP, OUTPUT);
  pinMode(LED_FWD, OUTPUT);
  pinMode(LED_REV, OUTPUT);

  // WiThrottle will wakeup by pressing emergency stop button
  /*
   * ATTENTION:
   * gpio_num_t has to be the same GPIO as BTN_STOP
   */
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_15, LOW);

	#ifdef ROT_ENCODER
		pinMode(ENC_CLK, INPUT);
		pinMode(ENC_DT, INPUT);
		pinMode(ENC_BTN, INPUT_PULLUP);

		// WiThrottle initializes hardware
		rotaryEncoder.begin();
		rotaryEncoder.setup([] {rotaryEncoder.readEncoder_ISR();});

	#endif

  #ifdef HL_DISP
    throttle.initDisplay();
  #endif

  throttle.initEeprom();

  // WiThrottle establishes communication
  throttle.connectToWiFi(wiFiSettings);
  throttle.connectToJMRI(hostSettings);
  throttle.turnHeartbeatMonitoringOn();
  throttle.listenToServer();

  // WiThrottle loads default loco
  throttle.assignLoco(activeLoco);

//  throttle.loco[0].select("ID of loco", throttle.lists.roster);

  // I want to try to acquire last active loco
  address = throttle.getLastAddress();
  if (address != 0 && address != 65535)  {
    // Acquire loco with DCC address read from EEPROM
    throttle.loco[0].select(address);  
    throttle.loco[0].acquire();
  }

  throttle.listenToServer();
}

void loop() {
  /*
   * Code is executed repetitively.
   *
   * Order of calling the subs is by descencing importance.
   */
  throttle.checkConnectionToWiFi();
  throttle.checkConnectionToJMRI();
  btnStopLoop();
	#ifndef ROT_ENCODER
		directionLoop();	// direction from switch
	#endif
  ledLoop();
  btnFnLoop();
	#ifndef ROT_ENCODER
		speedLoop();	// speed from Potentiometer
	#endif
	throttle.sendHeartbeat();
  throttle.listenToServer();
	#ifdef ROT_ENCODER
		encoderLoop();	// speed from rotary encoder
	#endif
  #ifdef HL_DISP
    throttle.fastClockUpdate();
  #endif
}

// Checks if emergency stop button has been pressed
void btnStopLoop() {
  /*
   * If shift button is pressed together with emergencystop button, 
   * loco will be despatched. Version without display will turn off
   * after dispatch.
   * 
   * If emergency button only is pressed, speed is set to 0 (notch -126) 
   * and the potentiometer is set out of order.
   * 
   * Pressing button longer than 5 seconds will turnoff WiThrottle.
   * 
   * WiThrottle can be switched on again by pressing the button again.
   */

  unsigned long startTime;                  // Start time emergency stop button has been pressed

  // I want to check if emergency stop button has been pressed together with shift button
  if (digitalRead(BTN_FCT_SH) == LOW && digitalRead(BTN_STOP) == LOW) {
    // Loco will be dispatchedturn off
    throttle.loco[0].dispatch();
    throttle.setLastAddress(0);

    // Loop while emergency button is pressed to avoid chatter effect
    while (digitalRead(BTN_STOP) == LOW) {
    }
  }
  else if (digitalRead(BTN_STOP) == LOW && throttle.loco[0].getNotch() != ESTOP) {
    // Loco will be stopped for emergency
    throttle.loco[0].setNotch((throttle.loco[0].getNotch() >= 0) * ESTOP);
    startTime = millis();

    // Loop while emergency button is pressed to avoid chatter effect
    while (digitalRead(BTN_STOP) == LOW) {
      // I want to check the time the emergency stop button is beeing pressed
      if ((millis() - startTime) >= 5000) {
        // WiThrottle will be turned off
        throttle.shutdown();
      }
    }
  }
}

// Checks if a function button is pressed
void btnFnLoop() {
  /*
   * Function buttons are read in combination with the shift button.
   * 
   * As long as a function button is pressed function is only 
   * toggled once.
   */

  // I want to check if a function button is pressed
  for(int i = 0; i < btnFctCount; i++) {
    if (digitalRead(btnFctPin[i]) == LOW) {
      throttle.loco[0].function[i + (btnFctCount * (digitalRead(BTN_FCT_SH) == LOW))].toggle();

      // Loop while function button is pressed to avoid chatter effect
      while (digitalRead(btnFctPin[i]) == LOW) {
      }
    }
  }
}

#ifdef ROT_ENCODER
// Checks if encoder is used
void encoderLoop() {
  /* 
   * TODO
   */
  
  int encoderDelta;                         // Encoder delta value
  int encoderValue;                         // Encoder absolute value

  // Define min and max value
  rotaryEncoder.setBoundaries(0, 10, false);

  // I want to check if encoder button is pressed
  if (rotaryEncoder.currentButtonState() == BUT_RELEASED) {
    Serial.println("Rotary knob");
  }

  encoderDelta = rotaryEncoder.encoderChanged();
  if (encoderDelta > 0) {
    Serial.println("+");
  }
  if (encoderDelta < 0) {
    Serial.println("-");
  }

  // I want to check if encoder position has been changed
  if (encoderDelta != 0) {    
    encoderValue = rotaryEncoder.readEncoder();
    Serial.println("Encoder value: " + String(encoderValue));
  } 
}
#endif

// Checks if a loco is active
void ledLoop() {
  /*
   * Controls LED for direction and emergency stop; checks, wheather 
   * a new loco has to be acquiredand turns heartbeat on and off.
   *
   * TODO:
   * IDLE-state --> both LED FWD and REV on
   */

  bool ledState = LOW;                      // Status of direction LED
  unsigned int direction;                   // Actual direction of loco
  unsigned int address;                     // DCC address of loco

  // I want to check if a loco is acquired
  if (throttle.loco[0].getAcquired()) {
    // A loco is acquired
    direction = throttle.loco[0].getDirection();
    
    // I want to check if the loco has already been stopped for emergency
    if (throttle.loco[0].getNotch() == ESTOP) {
      // Loco has already been stopped for emergency
      
      // Loop while reference notch > 0
      while (analogRead(POT_SIG) > 0) {
        // Inverse LED state
        ledState = !ledState;
        digitalWrite(ledDirPin[direction], ledState);
        #ifdef HL_DISP
          /*
           * TODO
           */
        #endif
  
        // Wait
        delay(LED_DELAY);
      }
    }
    else if (digitalRead(ledDirPin[direction] == LOW)) {
      // Indicate the loco's direction
      digitalWrite(LED_STOP, LOW);
      digitalWrite(ledDirPin[direction], HIGH);
      digitalWrite(ledDirPin[!direction], LOW);
      #ifdef HL_DISP
        display.drawBitmap(OLED_FWD_X, OLED_AREA_2_Y, imgFWD7x7, 7, 7, direction == FWD);
        display.drawBitmap(OLED_REV_X, OLED_AREA_2_Y, imgREV7x7, 7, 7, direction == REV);
        display.display();
      #endif
    }
  }
  else {
    // No loco is acquired
    digitalWrite(LED_STOP, HIGH);
    digitalWrite(LED_FWD, LOW);
    digitalWrite(LED_REV, LOW);
    #ifdef HL_DISP
      display.drawBitmap(OLED_FWD_X, 20, imgFWD7x7, 7, 7, OLED_COLOR_BLACK);
      display.drawBitmap(OLED_REV_X, 20, imgREV7x7, 7, 7, OLED_COLOR_BLACK);
      display.display();
    #endif

    // I want to try to acquire a new loco
    address = throttle.getAddressBySerial();
    if (address != 0) {
      // Try to acquire loco with DCC address was received from serial monitor, 
      throttle.loco[0].select(address);
      throttle.loco[0].acquire();
      throttle.setLastAddress(address);
    }
  }
}
  
// Checks if direction of loco needs to change
void directionLoop() {
  /*
   * Reads reference direction from direction switch and compares 
   * actual direction with reference direction; ifactual direction 
   * is different from reference direction it is checked wheather 
   * actual DCC notch is above 0. If so loco will be stopped 
   * immediately, but direction will not change unless potentiometer 
   * has been set to 0.
   */

  int directionReference;                   // Reference direction

  directionReference = !digitalRead(DIR_SW);

  // I want to check if direction of loco needs to be changed
  if (directionReference != throttle.loco[0].getDirection()) {
    // The direction of the loco is different from the reference direction
    switch(directionReference) {
      case IDLE:
        /*
         * TODO:
         * Define idle behaviour
         *
         * Idea:
         * Use a tri state switch with 2 x 10 kOhm resistors: VCC to
         * signal and signal to GND
         *
         * Code:
         * directionReference = map(analogRead(DIR_SW), 0, 4095, 0, 2);
         * default:
         * directionReference = directionReference / 2;
         *
         * throttle.loco[0].setDirection(directionReference);
         */
        break;

      default:        
        // I want to check if loco has to be stopped first
        if (throttle.loco[0].getNotch() > 0) {
          // Notch > 0, loco will be stopped
          throttle.loco[0].setNotch(ESTOP);
        }
        else {
          // Notch = 0, direction of loco is changed
          throttle.loco[0].setDirection(directionReference);
        }

        break;
    }
  }
}

#ifndef ROT_ENCODER
// Checks if speed of loco needs to change
void speedLoop() {
  /*
   * To avoid unnecessary server communication the average value is 
   * flattened by a median filter.
   */

  unsigned int potentiometerSignal;         // Signal read from potentiometer; value ranges from 0 to 4095 (12 bit resolution)
  unsigned int notch;                       // Reference speed translated into DCC notches
  unsigned int boundaryArea;                // Size of boundary area; unit: DCC notches
  const unsigned int notchTimeout = 250;    // Time before notch information is send next time to WiThrottle server

  potentiometerSignal = analogRead(POT_SIG);
  
  // I want to make min and max area less sensitive depending on the DCC speed step mode
  switch(notchRange) {
    case 13:
      // DCC 14 speed step mode
      boundaryArea = 1;
      break;

    case 27:
      // DCC 28 speed step mode
      boundaryArea = 3;
      break;

    case 126:
      // DCC 128 speed step mode
      boundaryArea = 10;
      break;
  }
  notch = map(potentiometerSignal, 0, 4095, 0, notchRange + 2 * boundaryArea);
  notch = min(max(notch, boundaryArea), notchRange + boundaryArea) - boundaryArea;

  // I want to filter notch values
  notch = notchFiltered.in(notch);

  // I want to check if direction switch is in IDLE position
  if (throttle.loco[0].getDirection() == IDLE) {
    // Loco in idle mode means notch = 0
    notch = 0;
  }

  // I want to check if notch has to be sent
  if (millis() > notchTime + notchTimeout) {
    // <notchTimeout> senconds passed since notch has been sent last time
    notchTime = millis();
    throttle.loco[0].setNotch(notch);
  }
}
#endif
