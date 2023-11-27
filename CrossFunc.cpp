/*
 * Definition of cross functional services
 */

#include "CrossFunc.h"
#include "Symbols.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi

// WiFi settings
wiFiConfig wiFiSettings {
  "SSID", 
  "Passphrase", 
  4
};

WiFiClient client;                      // This throttle's WiFi client


// WiThrottle server communication

// WiThrottle server settings
hostConfig hostSettings {
  "0.0.0.0",
  12090,
  2,
  "",
  0
};

unsigned long lastHeartbeat;            // Timestamp of last heartbeat sent to WiThrottle server

// Send command to WiThrottle server
void sendCmd(String command, bool waitAfterCommand) {
  client.println(command);
  lastHeartbeat = millis();

  #ifdef DEBUG
    Serial.println("-->: " + command);
  #endif

  if (waitAfterCommand) {
    delay(JMRI_DELAY);
  }
}

// Read command from WiThrottle server
String readCmd() {
  String command;                       // Command string sent by WiThrottle server
  String tmpCommand;                    // Modified command string used for debugging

  if (client.available() > 0) {
    command = client.readString();
    client.flush();

    #ifdef DEBUG
      if (command != "\r\n\r\n" && command != "") {
        tmpCommand = command.substring(0, command.length() - 4);
        tmpCommand.replace("\r\n\r\n", "\r\n\<--: ");
        Serial.println("<--: " + tmpCommand);
      }
    #endif
  }

  return command; 
}


// I2C OLED display
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
