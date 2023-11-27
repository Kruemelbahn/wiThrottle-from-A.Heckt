/*
 * Declaration of a loco's properties according to NMRA DCC
 */

#ifndef _VIRTUAL_LOCO_H_
#define _VIRTUAL_LOCO_H_

#include "CrossFunc.h"
#include "DccFunction.h"
#include <Arduino.h>
#include <WiFi.h>


// Direction
#define FWD               1                 // Direction forward
#define REV               0                 // Direction reverse
#define IDLE              2                 // Any direction but idle


// Notch
#define ESTOP          -126                 // Notch which is set in case of emergency stop

class VirtualLoco {
  private:
    // Address
    unsigned int address;                   // Address
    String addressType;                     // Address type
    String id;                              // ID (e. g. in the JMRI roster list)

    // Direction
    byte direction;                         // Direction
    String directionTxt[3] = { "REV", "FWD", "IDLE" };
                                            // Direction as a text --> used for debugging

    // Notch
    int notch;                              // Notch
                                            /*
                                             * Valid ranges:
                                             * 0 to 13  (NMRA: optional)
                                             * 0 to 27  (NMRA: mandatory)
                                             * 0 to 126 (NMRA: optional)
                                             * 
                                             * JMRI specialty:
                                             * -126 = emergency stop
                                             */
    byte speedStepMode;                     // Speed step mode
    String speedStepModeTxt[10] = { "<0>", "<1>", "<2>", "<3>", "<4>", "<5>", "<6>", "<7>", "<8>", "<9>" };
                                            // Speed step mode as a text --> used for debugging
                                            /*
                                             * TODO
                                             * What's the meaning of the value sent by WiThrottle server?
                                             * What's the valid range of values?
                                             */

    // Functions
    void initFunctions();                   // Initialize functions

    // Acquire and dispatch
    bool acquired = false;                  // Loco is acquired by WiThrottle

    // WiThrottle server communication
    String cmdPrefix;                       // Prefix to be used in the communication to WiThrottle serverserver

  public:
    // Constructor
    VirtualLoco(void);
    VirtualLoco(unsigned int address);
    VirtualLoco(String id, String &rosterList);

    // Deconstructor
    ~VirtualLoco(void);

    // Initialize Class
    void setPrefix(String cmdPrefix);       // Set Prefix for WiThrottle server communication

    // DCC address
    void select(unsigned int address, bool updateID = true);
                                            // Select loco by DCC address
    void select(String id, String &rosterList);
                                            // Select loco of roster list by ID
    unsigned int getAddress();              // Get DCC address of the loco
    String getAddressType();                // Get address type of the loco
                                            /*
                                             * Values:
                                             * S = short
                                             * L = long
                                             */
    String getDescription();                // Get description of the loco to send debugging information --> used for debugging

    // Direction control
    void setDirection(int direction);       // Set direction of the loco
    unsigned int getDirection();            // Get direction of the loco

    // Notch control
    void setNotch(int notch);               // Set notch of the loco
    int getNotch();                         // Get notch of the loco

    // Functions
    DccFunction function[29];               // Max. number of functions according to NMRA

    // Acquire and dispatch
    void acquire();                         // Acquire loco from WiThrottle server and assign to WiThrottle
    void dispatch();                        // Dispatch loco to WiThrottle server
    bool getAcquired();                     // Get acquisition state of the loco

    // WiThrottle server communication
    void listenToThrottle(String serverInfo);
                                            // Use information sent by WiThrottle server to WiThrottle to update the DCC function's information about state and label
};
#endif
