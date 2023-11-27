/*
 * Definition of a JMRI WiThrottle
 */

#include "WiThrottle.h"
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <ctype.h>
#include <EEPROM.h>
#include <TimeLib.h>
#include <WiFi.h>
#include <Wire.h>


// WiFi communication
extern WiFiClient client;                   // This throttle's WiFi client

// WiThrottle server communication
extern unsigned long lastHeartbeat;         // Timestamp of last heartbeat sent to WiThrottle server


// I2C OLED display
extern Adafruit_SSD1306 display;
extern unsigned char imgBootSequence56x32[];
extern unsigned char imgExlamation16x16[];
extern unsigned char imgJMRI16x16[];
extern unsigned char imgWiFi16x16[];


// Constructor
WiThrottle::WiThrottle(char* name) {
  this->name = name;
}

// Display initialization
void WiThrottle::initDisplay() {
  String bootMessage;                       // Boot sequence message

  #ifdef HL_DISP
    // Start OLED display
    /*
     * SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
     */

    // I want to check if WiThrottle was able to start display
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C)) {
      // Error
      errorHandling("Failed to\ninitialize\nI2C OLED\ndisplay");
    }
    else {
      // Display is active
      display.setRotation(3);
      display.setTextSize(1);
      display.setTextColor(OLED_COLOR_WHITE);
      display.clearDisplay();
      display.display();
    }
  
    // Write boot sequence message to display
    bootMessage = "WiThrottle\n\nAn FSMD\nproject\nV " + String(SW_VERSION) + "\n\n(c) " + String(SW_RELEASE_YEAR) + "\n\n" + String(SW_LICENSE);
    display.setCursor(0, 48);
    display.println(bootMessage);
    display.drawBitmap(4, 4, imgBootSequence56x32, 56, 32, OLED_COLOR_WHITE);
    display.display();
  #endif
}

// EEPROM initialization
void WiThrottle::initEeprom() {
  // I want to check if WiThrottle was able to start display
  if (!EEPROM.begin(EEPROM_SIZE))
  {
    // Error
    errorHandling("Failed to\ninitialize\nEEPROM");
  }
}

// WiFi connection

// Connect to WiFi
void WiThrottle::connectToWiFi(wiFiConfig &wiFiSettings) {
  unsigned int i = 0;                       // Counter variable

  this->wiFiSettings = wiFiSettings;
  #ifdef DEBUG
    Serial.println("Connecting to WiFi '" + String(wiFiSettings.ssid) + "':");
  #endif

  // WiThrottle tries <attempts> times to establish a WiFi connection
  while (WiFi.status() != WL_CONNECTED && i < wiFiSettings.attempts) {
    i++;
    #ifdef DEBUG
      Serial.println("Try # " + String(i));
    #endif
    WiFi.begin(wiFiSettings.ssid, wiFiSettings.pwd);
    WiFi.setHostname(name);
    WiFi.macAddress(macAddress);

    // Establishing a WiFi connection takes time
    delay(3500);
  }

  // I want to check if WiThrottle was able to establish a WiFi connection in defined number of attempts
  if (i >= wiFiSettings.attempts) {
    // Error
    errorHandling("Failed to\nconnect to\nWiFi!");
  }
  else {
    // WiThrottle successfully established a WiFi connection
    #ifdef DEBUG
      Serial.print("Connected to WiFi!\nIP address: ");
      Serial.println(WiFi.localIP());
      Serial.print("MAC address: ");
      for (int i = 0; i <= 5; i++) {
        if (i < 5) {
          Serial.print(macAddress[i], HEX);
          Serial.print(":");
        }
        else {
          Serial.println(macAddress[i], HEX);
        }
      }
    #endif

    #ifdef HL_DISP
      // Wait to keep boot message readable
      delay(2500);

      // Clear display and show WiFi symbol
      display.clearDisplay();
      display.drawBitmap(OLED_WIFI_X, OLED_AREA_1_Y, imgWiFi16x16, 16, 16, OLED_COLOR_WHITE);
      display.display();
    #else
      // Wait to keep LED state readable
      delay(500);

      digitalWrite(LED_STOP, HIGH);
      digitalWrite(LED_FWD, HIGH);
      digitalWrite(LED_REV, HIGH);
    #endif
  }
}

// Disconnect from WiFi
void WiThrottle::disconnectFromWiFi() {
  WiFi.disconnect();

  #ifdef HL_DISP
    // Hide WiFi symbol
    display.drawBitmap(OLED_WIFI_X, OLED_AREA_1_Y, imgWiFi16x16, 16, 16, OLED_COLOR_BLACK);
    display.display();
  #else
    digitalWrite(LED_STOP, HIGH);
    digitalWrite(LED_FWD, HIGH);
    digitalWrite(LED_REV, HIGH);
  #endif

  // Wait to keep display or led statereadable
  delay(500);

  #ifdef DEBUG
    Serial.println("Disconnected from WiFi!");
  #endif
}

// Check if WiFi connection is still alive
void WiThrottle::checkConnectionToWiFi() {
  // I want to check if WiFi connection broke
  if (WiFi.status() != WL_CONNECTED) {
    errorHandling("WiFi\nconnection\nbroke!");
  }
}


// WiThrottle server connection

// Connect to WiThrottle server
void WiThrottle::connectToJMRI(hostConfig &hostSettings) {
  unsigned int attempt = 0;                 // Counter variable
  String jmriCmd = "";                      // Text containing commands that is sent from throttle to WiThrottle server

  this->hostSettings = hostSettings;

  #ifdef DEBUG
    Serial.println("Connecting to WiThrottle server " + String(hostSettings.ip));
  #endif

  // WiThrottle tries <attempt> times to establish a WiThrottle server connection
  while (client.connected() != true && attempt < hostSettings.attempts) {
    attempt++;
    #ifdef DEBUG
      Serial.println("Try # " + String(attempt));
    #endif

    // I want to check if connection has been established
    if (client.connect(hostSettings.ip, hostSettings.port)) {
      #ifdef DEBUG
        Serial.println("Connected to WiThrottle server!");
      #endif

      // Read initial message from WiThrottle server
      listenToServer();

      // Send hardware information to WiThrottle server, use Mac address
      #ifdef DEBUG
        Serial.println("Send hardware information to server.");
      #endif
      jmriCmd = "HU" + String(macAddress[0], HEX) + String(macAddress[1], HEX) + String(macAddress[2], HEX) + String(macAddress[3], HEX) + String(macAddress[4], HEX) + String(macAddress[5], HEX);
      jmriCmd.toUpperCase();
      sendCmd(jmriCmd);

      // Publish name of throttle to WiThrottle server
      #ifdef DEBUG
        Serial.println("Publish name of throttle.");
      #endif
      jmriCmd = "N";
      jmriCmd.concat(name);
      sendCmd(jmriCmd);

      #ifdef HL_DISP
        // Clear other areas on display
        display.fillRect(OLED_AREA_2_X, OLED_AREA_2_Y, OLED_AREA_2_W, OLED_AREA_2_H, OLED_COLOR_BLACK);
        display.fillRect(OLED_AREA_3_X, OLED_AREA_3_Y, OLED_AREA_3_W, OLED_AREA_3_H, OLED_COLOR_BLACK);
        display.display();

        // Show JMRI symbol
        display.drawBitmap(OLED_JMRI_X, OLED_AREA_1_Y, imgJMRI16x16, 16, 16, OLED_COLOR_WHITE);
        display.display();
      #else
        digitalWrite(LED_STOP, LOW);
        digitalWrite(LED_FWD, HIGH);
        digitalWrite(LED_REV, HIGH);
      #endif
    }
  }

  // I want to check if WiThrottle was able to establish a connection to WiThrottle server in defined number of attempts
  if (attempt >= hostSettings.attempts) {
    // Error
    errorHandling("Failed to\nconnect to\nWiThrottle\nserver!");
  }
}

// Disconnect from WiThrottle server
void WiThrottle::disconnectFromJMRI() {
  // Disconnect from WiThrottle server
  #ifdef DEBUG
    Serial.println("Disconnect from WiThrottle server!");
  #endif

  sendCmd("Q");
  client.stop();

  #ifdef HL_DISP
    // Hide JMRI symbol
    display.drawBitmap(OLED_JMRI_X, OLED_AREA_1_Y, imgJMRI16x16, 16, 16, OLED_COLOR_BLACK);
    display.display();
  #else
    digitalWrite(LED_STOP, LOW);
    digitalWrite(LED_FWD, HIGH);
    digitalWrite(LED_REV, HIGH);
  #endif

  // Wait to keep display or led statereadable
  delay(500);
}

// Check if WiThrottle server connection is still alive
void WiThrottle::checkConnectionToJMRI() {
  // I want to check the WiThrottle server connection state
  if (!client.connected()) {
    // Error
    errorHandling("Connection\nto \nWiThrottle\nserver\nbroke!");
  }
}

// Listen to WiThrottle server
void WiThrottle::listenToServer() {
  char* cmdLine;                            // Line with commands
  String serverInfo;                        // Complete set of commands sent by WiThrottle
  String cmdItem;                           // Command item
  const char* delim = "\r\n\r\n";           // Delimiter betweed lines with commands

  serverInfo = readCmd();

  if (serverInfo != "") {
    // Split different commands
    cmdLine = strtok(const_cast<char*>(serverInfo.c_str()), delim);
    while (cmdLine != NULL) {
      cmdItem = String(cmdLine);

      // I want to check the type of information
      if (cmdItem.startsWith("M0")) {
        // MultiThrottle information
        cmdItem = cmdItem.substring(2, cmdItem.length());
        loco[0].listenToThrottle(cmdItem);
      }
      else if (cmdItem.startsWith("*")) {
        // Heartbeat information
        cmdItem = cmdItem.substring(1, cmdItem.length());
        hostSettings.heartbeat = cmdItem.toInt();

        #ifdef DEBUG
          Serial.println("Heartbeat is expected after " + String(hostSettings.heartbeat) + " seconds.");
        #endif
      }
      else if (cmdItem.startsWith("PFT")) {
        // Fastclock information
        cmdItem.replace("PFT", "");

        // Unix Timestamp of fast clock supplied by WiThrottle server
        fastClockSettings.timeStamp = (cmdItem.substring(0, cmdItem.indexOf("<;>")).toInt());

        // Sync with millis
        fastClockSettings.timeStampMillis = millis();

        // Fast time ratio
        if (cmdItem.indexOf("<;>") >= 0) {
          // Sometimes ratio is not sent by WiThrottle server
          fastClockSettings.ratio = cmdItem.substring(cmdItem.indexOf("<;>") + 3, cmdItem.length()).toDouble();
        }

        #ifdef DEBUG
          Serial.printf("Fast clock timestamp is %02d:%02d:%02d, fast time runs with a ratio of %2.1f.\n", hour(fastClockSettings.timeStamp), minute(fastClockSettings.timeStamp), second(fastClockSettings.timeStamp), fastClockSettings.ratio);
        #endif
      }
      else if (cmdItem.startsWith("PPA")) {
        // Track power information
        cmdItem.replace("PPA", "");
        trackPower = cmdItem.toInt();

        #ifdef DEBUG
          Serial.print("Track power of DCC system ");
          if (trackPower == 1) {
            Serial.println("is on.");
          }
          else if (trackPower == 0) {
            Serial.println("is off.");
          }
          else {
            Serial.println("has an unknown state.");
          }
        #endif
      }
      else if (cmdItem.startsWith("PR")) {
        // Route list
        cmdItem = cmdItem.substring(cmdItem.indexOf(']\[') + 1, cmdItem.length());
        lists.route = cmdItem;

        #ifdef DEBUG
          Serial.print("Route list received: ");
          if (lists.route == "") {
            Serial.println("no entries.");
          }
          else {
            Serial.println(lists.route);
          }
        #endif
      }
      else if (cmdItem.startsWith("PT")) {
        // Turnout list
        cmdItem = cmdItem.substring(cmdItem.indexOf(']\[') + 1, cmdItem.length());
        lists.turnout = cmdItem;

        #ifdef DEBUG
          Serial.print("Turnout list received: ");
          if (lists.turnout == "") {
            Serial.println("no entries.");
          }
          else {
            Serial.println(lists.turnout);
          }
        #endif
      }
      else if (cmdItem.startsWith("PW")) {
        // JMRI web port
        cmdItem.replace("PW", "");

        #ifdef DEBUG
          Serial.println("Used web port is " + cmdItem + ".");
        #endif
      }
      else if (cmdItem.startsWith("RC")) {
        // Consist list
        cmdItem.replace("RCC0", "");
        cmdItem = cmdItem.substring(cmdItem.indexOf(']\[') + 1, cmdItem.length());
        lists.consist = cmdItem;

        #ifdef DEBUG
          Serial.print("Consist list received: ");
          if (lists.consist == "") {
            Serial.println("no entries.");
          }
          else {
            Serial.println(lists.consist);
          }
        #endif
      }
      else if (cmdItem.startsWith("RL")) {
        // Roster list
        cmdItem.replace("RL0", "");
        cmdItem = cmdItem.substring(cmdItem.indexOf(']\[') + 1, cmdItem.length());
        lists.roster = cmdItem;

        #ifdef DEBUG
          Serial.print("Roster list received: ");
          if (lists.roster == "") {
            Serial.println("no entries.");
          }
          else {
            Serial.println(lists.roster);
          }
        #endif
      }
      else if (cmdItem.startsWith("VN")) {
        // Protocol version
        cmdItem.replace("VN", "");
        hostSettings.protocolVersion = cmdItem;

        #ifdef DEBUG
          Serial.println("Protocol version is " + cmdItem + ".");
        #endif
      }
      else {
        // Unknown command
        #ifdef DEBUG
          Serial.println("Class WiThrottle: Unknown command '" + serverInfo + "'.");
        #endif
      }

      cmdLine = strtok(NULL, delim);
    }
  }
}


// WiThrottle control

// Put WiThrottle into sleep mode
void WiThrottle::shutdown() {
  String shutdownMessage;                   // Shutdown sequence message

  loco[0].dispatch();
  retractLoco();
  turnHeartbeatMonitoringOff();
  disconnectFromJMRI();
  disconnectFromWiFi();

  #ifdef HL_DISP
    display.clearDisplay();
    display.display();

    // Write shutdown sequence message to display
    shutdownMessage = "WiThrottle\nis going\nto sleep!";
    display.setCursor(0, 48);
    display.println(shutdownMessage);
    display.drawBitmap(4, 4, imgBootSequence56x32, 56, 32, OLED_COLOR_WHITE);
    display.display();

    // Wait to keep shutdown message readable
    delay(2500);

    display.clearDisplay();
    display.display();
  #endif

  #ifdef DEBUG
    Serial.println("WiThrottle is going to sleep!\n--\nEnd\n<---");
    delay(500);
  #endif

  Serial.end();
  esp_deep_sleep_start();
}


// Loco handling

// Assign loco to WiThrottle
void WiThrottle::assignLoco(VirtualLoco loco) {
  this->loco[0] = loco;
  this->loco[0].setPrefix(cmdPrefix);
}

// Retract loco from WiThrottle
void WiThrottle::retractLoco() {
  /*
   * TODO
   */
}

// Check if a loco is selected
bool WiThrottle::checkActiveLoco() {
  bool tmpCheckActiveLoco = false;

  for (int i = 0; i <= LOCO_MAX; i++) {
    if (loco[i].getAddress() != 0) {
      tmpCheckActiveLoco = true;
      break;
    }
  }
  return tmpCheckActiveLoco;
}

// Read last active DCC address from EEPROM
unsigned int WiThrottle::getLastAddress() {
  byte byte_1 = EEPROM.read(1);         // First byte of DCC address
  byte byte_2 = EEPROM.read(2);         // Second byte of DCC address

  return int(byte_1 << 8) + int(byte_2);
}

// Write last active DCC address to EEPROM
void WiThrottle::setLastAddress(unsigned int address) {
  byte byte_1 = EEPROM.read(1);         // First byte of DCC address
  byte byte_2 = EEPROM.read(2);         // Second byte of DCC address
  unsigned int addressLast = int(byte_1 << 8) + int(byte_2);
  // DCC address

  if (address != addressLast) {
    // Address to save is different from last stored address
    byte_1 = byte(address >> 8);
    byte_2 = byte(address & 0x00FF);

    #ifdef DEBUG
      Serial.println("Write last DCC address " + String(address) + " to EEPROM.");
    #endif

    EEPROM.write(1, byte_1);
    EEPROM.write(2, byte_2);
    EEPROM.commit();
  }
}

// Get a DCC address by menu
unsigned int WiThrottle::getAddressByMenu() {
  /*
   * TODO
   */
//  unsigned int x = 0;
//  unsigned int y;
//
//  y = OLED_AREA_3_Y;
//  display.setTextColor(OLED_COLOR_WHITE);
//  display.setCursor(x, y);
//  display.print("Address?");
//
//  y = y + 10;
//  display.setTextColor(OLED_COLOR_BLACK, OLED_COLOR_WHITE);
//  display.setCursor(x, y);
//  display.print("0");
//  display.setTextColor(OLED_COLOR_WHITE, OLED_COLOR_BLACK);
//  display.print("0");
//  display.print("0");
//  display.print("0");
//  display.display();
}

// Get a DCC address by serial monitor
unsigned int WiThrottle::getAddressBySerial() {
  unsigned int address;                     // DCC address of loco
  String addressInput;                      // Input read from serial monitor
  bool isValidAddress = true;               // True if input is a valid DCC address
  unsigned long startTime;                  // Start time emergency stop button has been pressed

  // Turn heartbeat monitoring off while waiting for input
  turnHeartbeatMonitoringOff();  
  Serial.println("Please enter DCC address.");

  // Wait until input received
  while (Serial.available() == 0) {
    startTime = millis();
    while (digitalRead(BTN_STOP) == LOW) {
      // I want to check the time the emergency stop button is beeing pressed
      if ((millis() - startTime) >= 5000) {
        // WiThrottle will be turned off
        shutdown();
      }
    }
  }

  // Received input by serial monitor    
  while (Serial.available() > 0) {
    addressInput = Serial.readString();

    // Remove line feed
    addressInput = addressInput.substring(0, addressInput.length() - 1);

    // Compare length of input with allowed length
    isValidAddress = (addressInput.length() >= 1 && addressInput.length() <= 5);

    // Proceed if length of input is okay
    if (isValidAddress) {
      // I want to check if each digtit of the input is numeric
      for(int i = 0; i < addressInput.length(); i++) {
        isValidAddress = (isDigit(addressInput.charAt(i)) && isValidAddress);
      }

      // Proceed if input is numeric
      if (isValidAddress) {
        // I want to check if input is in extended DCC address range (1 to 10239)
        address = addressInput.toInt();
        isValidAddress = (address > 0 && address <= 10239);
      }
    }
  }

  // Error handling in case input is not a valid DCC address
  if (!isValidAddress) {
    address = 0;
    Serial.println(addressInput + " is not a valid DCC address.\nValid DCC addresses are from 1 to 10239.");
  }

  // Turn heartbeat monitoring on
  turnHeartbeatMonitoringOn();
  
  return address;
}


// Layout control

// Switch track power of DCC system on
void WiThrottle::switchDCCPowerOn() {
  #ifdef DEBUG
    Serial.println("Switch track power of DCC system on.");
  #endif

  sendCmd("PPA1");
}

// Switch track power of DCC system off
void WiThrottle::switchDCCPowerOff() {
  #ifdef DEBUG
    Serial.println("Switch Track power of DCC system off.");
  #endif

  sendCmd("PPA0");
}


// Fast clock

// Updates fast clock
void WiThrottle::fastClockUpdate() {
  unsigned long timeStampAct;               // Actual timestamp for fast clock
  unsigned int x = (OLED_HEIGHT - (5 * 6)) / 2 + 2;
                                            // x-position of fast clock on display

  #ifdef HL_DISP
    timeStampAct = fastClockSettings.timeStamp + (long)((millis() - fastClockSettings.timeStampMillis) * fastClockSettings.ratio / 1000);
    if (hour(timeStampAct) != hour(timeStampFC) || minute(timeStampAct) != minute(timeStampFC)) {
      display.setTextColor(OLED_COLOR_BLACK);
      display.setCursor(x, OLED_AREA_2_Y);
      display.printf("%02d:%02d", hour(timeStampFC), minute(timeStampFC));
      display.display();

      timeStampFC = timeStampAct;

      display.setTextColor(OLED_COLOR_WHITE);
      display.setCursor(x, OLED_AREA_2_Y);
      display.printf("%02d:%02d", hour(timeStampFC), minute(timeStampFC));
      display.display();
    }
  #endif
}


// Heartbeat

// Send heartbeat to WiThrottle server
void WiThrottle::sendHeartbeat() {
  double secondsSinceLastHeartbeat;         // Seconds since last heartbeat has been sent

  secondsSinceLastHeartbeat = double(millis() - lastHeartbeat) / 1000;
  
  // I want to check if timeout is near
  if (secondsSinceLastHeartbeat > (hostSettings.heartbeat - 2)) {
    #ifdef DEBUG
      Serial.printf("Send Heartbeat after %2.1f seconds of inactivity.\n", secondsSinceLastHeartbeat);
    #endif
    sendCmd("*");
  }
}

// Turn heartbeat monitoring on
void WiThrottle::turnHeartbeatMonitoringOn() {
  #ifdef DEBUG
    Serial.println("Turn heartbeat monitoring on.");
  #endif

  sendCmd("*+", false);
}

// Turn heartbeat monitoring off
void WiThrottle::turnHeartbeatMonitoringOff() {
  #ifdef DEBUG
    Serial.println("Turn heartbeat monitoring off.");
  #endif

  sendCmd("*-", false);
}


// Error handling
void WiThrottle::errorHandling(String errorMsg) {
  /*
   * Line breaks are necessary for a proper readable message
   * on the OLED display and have to be part of the text set
   * by <errorMsg>
   * 
   * Throttle has to be reset after an error occurs
   */

  unsigned long startTime = millis();       // Time when errorHandling begins
  unsigned int imgColor = OLED_COLOR_BLACK; // Color of symbol
  bool ledState = LOW;                      // Status of emergency stop LED

  #ifdef DEBUG
    Serial.println("\r\n(!)\r\n\r\n" + errorMsg + "\r\n");
  #endif

  #ifdef HL_DISP
    // Write error message on display
    display.clearDisplay();
    display.setCursor(0, 30);
    display.println(errorMsg);
    display.display();
  #endif

  // Flash LED and show error symbol on display
  do {
    // Don't proceed, loop for 5 seconds and then turn off WiThrottle
    #ifdef HL_DISP
      // Inverse color of symbol
      imgColor = !imgColor;
      display.drawBitmap(0, 0, imgExlamation16x16, 16, 16, imgColor);
      display.display();
    #endif

    // Inverse LED state
    ledState = !ledState;
    digitalWrite(LED_STOP, ledState);

    // Wait
    delay(LED_DELAY);
  } while ((millis() - startTime) <= 5000);

  #ifdef HL_DISP
    display.clearDisplay();
    display.display();
  #endif

  shutdown();
}
