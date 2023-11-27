/*
 * Declaration of a DCC function
 */

#ifndef _DCC_FUNCTION_H_
#define _DCC_FUNCTION_H_

#include "CrossFunc.h"
#include <Arduino.h>
#include <WiFi.h>


// State of DCC functions
#define OFF                 0               // Off
#define ON                  1               // On

class DccFunction {
  private:
    // Function
    byte fn;                                // No. of the function
                                            /*
                                             * 0 ... 28 
                                             */
    byte state;                             // State of the function
                                            /*
                                             * OFF, ON 
                                             */
    String stateTxt[2] = { "OFF", "ON" };   // State as a text --> used for debugging
    String label;                           // Name of the function

    // WiThrottle server communication
    String cmdPrefix;                       // Prefix to be used in the communication to WiThrottle server

  public:
    // Constructor
    DccFunction(void);

    // Initialize Class
    void setFn(byte fn);                    // Set function number
    void setPrefix(String cmdPrefix);       // Set Prefix for WiThrottle server communication

    // Change state of the function
    void toggle();                          // Change state
    void on();                              // Change state to On
    void off();                             // Change state to Off

    // WiThrottle server communication
    void listenToLoco(String serverInfo);   // Use information sent by WiThrottle server to WiThrottle to update the DCC function's information about state and label
};
#endif
