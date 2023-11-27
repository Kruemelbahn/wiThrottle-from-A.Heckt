/*
 * Declaration of a JMRI WiThrottle
 */

#ifndef _WI_THROTTLE_H_
#define _WI_THROTTLE_H_

#include "CrossFunc.h"
#include "VirtualLoco.h"
#include <Arduino.h>
#include <WiFi.h>


// LED
#define LED_DELAY         100               // Delay for flashing LED, e. g. in case of an error

// Track power of DCC system
#define POWER_ON            1               // Track power of DCC system is on
#define POWER_OFF           0               // Track power of DCC system is off
#define POWER_UNKNOWN       2               // Track power of DCC system is in unknown state

// Locos
#define LOCO_MAX            2               // Maximum number of locos to be handled by WiThrottle


// Structures

// Lists supplied to WiThrottle by WiThrottle server
typedef struct {
  String roster = "";                     // Roster list
  String turnout = "";                    // Turnout list --> not used by now
  String route = "";                      // Route list --> not used by now
  String consist = "";                    // Consist list --> not used by now
} jmriLists;

// Fastclock
typedef struct {
  unsigned long timeStamp = 0;              // Start time of fast clock as Unix Timestamp supplied by WiThrottle server
  double ratio;                             // Fast time ratio
  unsigned long timeStampMillis = 0;        // Millis since <timeStamp> has been updated
} fastClockConfig;


class WiThrottle {
  private:
    // This WiThrottle
    char* name;                             // Name of throttle

    // WiFi communication
    wiFiConfig wiFiSettings;                // WiFi settings
    byte macAddress[6];                     // MAC address

    // WiThrottle server communication
    hostConfig hostSettings;                // WiThrottle server settings
    const String cmdPrefix = "M0";          // Prefix to be sent for Multithrottle commands

    // Layout control
    byte trackPower = POWER_UNKNOWN;        // Track power of DCC system
    fastClockConfig fastClockSettings;      // Fast clock settings

    // Fast clock
    unsigned long timeStampFC;              // Last timestamp for fast clock

  public:
    // Constructor
    WiThrottle(char* name);

    // Display initialization
    void initDisplay();

    // EEPROM initialization
    void initEeprom();

    // WiFi connection
    void connectToWiFi(wiFiConfig &wiFiSettings);
                                            // Connect to WiFi
    void disconnectFromWiFi();              // Disconnect from WiFi
    void checkConnectionToWiFi();           // Check if WiFi connection is still alive

    // WiThrottle server connection
    void connectToJMRI(hostConfig &host);   // Connect to WiThrottle server
    void disconnectFromJMRI();              // Disconnect from WiThrottle server
    void checkConnectionToJMRI();           // Check if WiThrottle server connection is still alive
    void listenToServer();                  // Listen to WiThrottle server
    jmriLists lists;                        // Lists supplied to WiThrottle by WiThrottle server

    // WiThrottle control
    void shutdown();                        // Put WiThrottle into sleep mode

    // Loco control
    void assignLoco(VirtualLoco loco);      // Assign loco to WiThrottle
    void retractLoco();                     // Retract loco from WiThrottle
    bool checkActiveLoco();                 // Check if a loco is selected
    VirtualLoco loco[LOCO_MAX];             // Locos to be controlled by WiThrottle; maximum is 3 locos
    unsigned int getLastAddress();          // Read last active DCC address from EEPROM
    void setLastAddress(unsigned int address);
                                            // Write last active DCC address to EEPROM
    unsigned int getAddressByMenu();        // Get a DCC address by menu
    unsigned int getAddressBySerial();      // Get a DCC address by serial monitor

    // Layout control
    void switchDCCPowerOn();                // Switch track power of DCC system on
    void switchDCCPowerOff();               // Switch track power of DCC system off

    // Fast clock
    void fastClockUpdate();                 // Updates fast clock

    // Heartbeat
    void sendHeartbeat();                   // Send heartbeat to WiThrottle server
    void turnHeartbeatMonitoringOn();       // Turn heartbeat monitoring on
    void turnHeartbeatMonitoringOff();      // Turn heartbeat monitoring off
    
    // Error handling
    void errorHandling(String errorMsg);
};
#endif
