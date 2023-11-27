/*
 * Definition of a DCC function
 */

#include "DccFunction.h"


// Constructor
DccFunction::DccFunction() {
  fn = 0;
  state = OFF;
  label = "";
}


// Initialize Class

// Set function number
void DccFunction::setFn(byte fn) {
  this->fn = fn;
  label = String("F" + fn);
}

// Set Prefix for WiThrottle server communication
void DccFunction::setPrefix(String cmdPrefix) {
  this->cmdPrefix = cmdPrefix;
}


// State of the function

// Change state
void DccFunction::toggle() {
  #ifdef DEBUG
    Serial.println("Toggle function F" + String(fn) + ".");
  #endif

  state = !state;
  sendCmd(cmdPrefix + "<;>F" + state + fn);	// ZIM: ON changed to state???
}

// Change state to On
void DccFunction::on() {
  #ifdef DEBUG
    Serial.print("Turn function F" + String(fn) + " on --> ");
  #endif

  // I want to check if state has to be changed
  if (state == OFF) {
    toggle();
  }
}

// Change state to Off
void DccFunction::off() {
  #ifdef DEBUG
    Serial.print("Turn function F" + String(fn) + " off --> ");
  #endif

  // I want to check if state has to be changed
  if (state == ON) {
    toggle();
  }
}


// WiThrottle server communication

// Use information sent by WiThrottle server to WiThrottle to update the DCC function's information about state and label
void DccFunction::listenToLoco(String serverInfo) {
  // I want to check if information is a function state information
  if (serverInfo.charAt(0) == 'F') {
    // I want to check if function state information belongs to this function
    if (serverInfo.substring(2, serverInfo.length()).toInt() == fn) {
      // State information
      state = serverInfo.substring(1, 2).toInt();

      #ifdef DEBUG
        Serial.print("DCC Function F" + String(fn));
        if (label != "") Serial.print(" '" + label + "'");
        Serial.println(" is " + stateTxt[state] + ".");
      #endif
    }
  }
  else if (serverInfo.startsWith("]\\[")) {
    // Function label information
    for(byte i = 0; i <= fn; i++) {
      serverInfo = serverInfo.substring(3, serverInfo.length());
      label = serverInfo.substring(0, serverInfo.indexOf("]\\["));
      serverInfo.replace(label, "");
    }

    #ifdef DEBUG
      Serial.println("DCC Function F" + String(fn) + " is named '" + label + "'.");
    #endif
  }
  else {
    // Unknown command
    #ifdef DEBUG
      Serial.println("Class Dccfunction: Unknown command '" + serverInfo + "'.");
    #endif
  }
}
