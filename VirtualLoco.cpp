/*
 * Definition of a loco's properties according to NMRA DCC
 */

#include "VirtualLoco.h"
#include <EEPROM.h>

#ifdef HL_DISP
  #include <Adafruit_SSD1306.h>
  #include <Adafruit_GFX.h>

  // I2C OLED display
  extern Adafruit_SSD1306 display;
  extern unsigned char imgOne16x16[];
  extern unsigned char imgOneInverted16x16[];
#endif


// Constructor
VirtualLoco::VirtualLoco(void) {
  address = 0;
  id = "# 0";
}

VirtualLoco::VirtualLoco(unsigned int address) {
  select(address);
}

VirtualLoco::VirtualLoco(String id, String &rosterList) {
  select(id, rosterList);
}


// Destructor
VirtualLoco::~VirtualLoco(void) {
  #ifdef HL_DISP
    // hide symbol for 1 loco
    display.drawBitmap(0, 0, imgOne16x16, 16, 16, OLED_COLOR_BLACK);
    display.display();
  #endif

  #ifdef DEBUG
    Serial.println("Loco with DCC address " + String(address) + " has beed rejected.");
  #endif
}


// Initialize Class

// Set Prefix for WiThrottle server communication
void VirtualLoco::setPrefix(String cmdPrefix) {
  this->cmdPrefix = cmdPrefix;
}


// DCC address

// Select loco by DCC address
void VirtualLoco::select(unsigned int address, bool updateID) {
  // I want to check if loco has already been acquired
  if (!acquired) {
    this->address = address;
    addressType = "S";                      // Default

    // I want to check if DCC address is a long one
    if (address > 127) {
      addressType = "L";
    }

    // I want to check if ID has to be updated
    if (updateID) {
      id = "# " + String(address);
    }

    initFunctions();

    #ifdef HL_DISP
      // show symbol for 1 loco
      display.drawBitmap(0, 0, imgOne16x16, 16, 16, OLED_COLOR_WHITE);
      display.display();
    #endif

    #ifdef DEBUG
      Serial.println("Select loco by DCC address " + String(address) + ".");
    #endif
  }
  else {
    /*
     * TODO
     */
  }
}

// Select loco by ID
void VirtualLoco::select(String id, String &rosterList) {
  char* tmpRosterList = (char*)"";          // Roster list
  String tmpRosterItem = (char*)"";         // Roster item
  const char* rosterItemDelim = "]\\[";     // Delimiter betweed each roster item
  const char* rosterItemInfoDelim = "}|{";  // Delimiter betweed each roster item
  String tmpID = "";                        // ID of roster item
  unsigned int tmpAddress = 0;              // DCC address of roster item

  // I want to check if loco has already been acquired
  if (!acquired) {
    // Split different entries of roster list
    tmpRosterList = strtok(const_cast<char*>(rosterList.c_str()), rosterItemDelim);
    while (tmpRosterList != NULL) {
      tmpRosterItem = String(tmpRosterList);
      tmpID = tmpRosterItem.substring(0, tmpRosterItem.indexOf(rosterItemInfoDelim));

      tmpRosterItem = tmpRosterItem.substring(tmpRosterItem.indexOf(rosterItemInfoDelim) + 3, tmpRosterItem.length());
      tmpAddress = tmpRosterItem.substring(0, tmpRosterItem.indexOf(rosterItemInfoDelim)).toInt();

//      tmpRosterItem = tmpRosterItem.substring(tmpRosterItem.indexOf(rosterItemInfoDelim) + 3, tmpRosterItem.length());
//      tmpAddressType = tmpRosterItem;

      // I want to check if ID matches
      if (id == tmpID) {
        this->id = id;
        select(tmpAddress, false);
        break;
      }

      tmpRosterList = strtok(NULL, rosterItemDelim);
    }
  }
  else {
    /*
     * TODO
     */
  }
}

// Get address of loco
unsigned int VirtualLoco::getAddress() {
  return address;
}

// Get address type of the loco
String VirtualLoco::getAddressType() {
  return addressType;
}

// Get description of the loco to send debugging information
String VirtualLoco::getDescription() {
  String tmpDescription = "";

  if (address != 0) {
    tmpDescription = "DCC address: " + String(address);
    if (id != "") {
      tmpDescription = "ID: '" + id + "'; " + tmpDescription;
    }
    tmpDescription = "(" + tmpDescription + ")";
  }
  return tmpDescription;
}


// Direction control

// Set direction of the loco
void VirtualLoco::setDirection(int direction) {
  // I want to check if setting the direction is possible
  if (acquired && direction != IDLE) {
    this->direction = direction;

    #ifdef DEBUG
      Serial.println("Set direction of loco " + getDescription() + " to " + directionTxt[direction] + ".");
    #endif

    sendCmd("M0A" + addressType + String(address) + "<;>R" + String(direction));
  }
}

// Get direction of the loco
unsigned int VirtualLoco::getDirection() {
  return direction;
}


// Notch control

// Set notch of the loco
void VirtualLoco::setNotch(int notch) {
  if (acquired) {
    if (notch == ESTOP) {
      #ifdef DEBUG
        Serial.println("Emergency stop loco " + getDescription() + ".");
      #endif

      this->notch = notch;
      sendCmd("M0A" + addressType + String(address) + "<;>X");
    }
    else if ((this->notch != ESTOP && notch != this->notch) || (this->notch == ESTOP && notch == 0)) {
      /*
       * Compares reference value of DCC notch with actually
       * set DCC notch; if different actual value is updated
       * to reference value
       *
       * change notch only if loco has not been stopped for 
       * emergency
       *
       * if loco has been stopped for emergency notch has to
       * be set to 0 first
       */
      this->notch = notch;

      #ifdef DEBUG
        Serial.println("Set notch of loco " + getDescription() + " to " + String(notch) + ".");
      #endif

      sendCmd("M0A" + addressType + String(address) + "<;>V" + String(notch));
    }
  }
}

// Get notch of the loco
int VirtualLoco::getNotch() {
  return this->notch;
}


// DCC functions

// Initialize DCC functions
void VirtualLoco::initFunctions() {
  for(byte fn = 0; fn <= 28; fn++) {
    function[fn].setFn(fn);
    function[fn].setPrefix(cmdPrefix + "A" + getAddressType() + getAddress());
  }
}


// Acquire and dispatch loco

// Acquire loco from WiThrottle server and assign to WiThrottle!
void VirtualLoco::acquire() {
  bool useID = false;                       // Use ID of loco to acuire

  // I want to check if acquisition is possible
  if (!acquired && address != 0) {
    #ifdef DEBUG
      Serial.println("Acquire loco " + getDescription() + ".");
    #endif

    // I want to check if loco has to be acquired by ID
    if (id.length() > 2) {
      if (id.substring(0, 2) != "# ") {
        useID = true;
      }
    }

    if (useID) {
      sendCmd("M0+" + addressType + String(address) + "<;>E" + id);
    }
    else {
      sendCmd("M0+" + addressType + String(address) + "<;>" + addressType + String(address));
    }

    #ifdef HL_DISP
      // show symbol for 1 loco
      display.drawBitmap(0, 0, imgOne16x16, 16, 16, OLED_COLOR_BLACK);
      display.drawBitmap(0, 0, imgOneInverted16x16, 16, 16, OLED_COLOR_WHITE);
      display.display();
    #endif
  }
}

// Dispatch loco to WiThrottle server!
void VirtualLoco::dispatch() {
  // I want to check if dispatch is possible
  if (acquired) {
    #ifdef DEBUG
      Serial.println("Dispatch loco " + getDescription() + ".");
    #endif

    sendCmd("M0-" + addressType + String(address) + "<;>r");
    select(0);

    #ifdef HL_DISP
      // hide symbol for 1 loco
      display.drawBitmap(0, 0, imgOneInverted16x16, 16, 16, OLED_COLOR_BLACK);
      display.drawBitmap(0, 0, imgOne16x16, 16, 16, OLED_COLOR_WHITE);
      display.display();
    #endif
  }
}

// Get acquisition state of the loco
bool VirtualLoco::getAcquired() {
  return acquired;
}


// WiThrottle server communication

// Use information sent by WiThrottle server to WiThrottle to update the DCC function's information about state and label
void VirtualLoco::listenToThrottle(String serverInfo) {
  char cmdKey;

  // I want to check if information belongs to this loco
  if (serverInfo.indexOf(getAddressType() + String(getAddress()) + "<;>") >= 0) {
    // Information belongs to this loco
    cmdKey = serverInfo.charAt(0);
    serverInfo = serverInfo.substring(serverInfo.indexOf("<;>") + 3, serverInfo.length());

    switch(cmdKey) {
      case '+':
        // Add a locomotive to the throttle
        acquired = true;
        #ifdef DEBUG
          Serial.println("Loco " + getDescription() + " has been acquired.");
        #endif
        break;

      case '-':
        // Remove a locomotive from the throttle
        acquired = false;
        #ifdef DEBUG
          Serial.println("Loco " + getDescription() + " has been dispatched.");
        #endif
        break;

      case 'A':
        // Action. The following characters provide more details
        cmdKey = serverInfo.charAt(0);
        serverInfo = serverInfo.substring(1, serverInfo.length());

        switch(cmdKey) {
          case 'F':
            // Function state information
            for(byte fn = 0; fn <= 28; fn++) {
              function[fn].listenToLoco(cmdKey + serverInfo);
            }
            break;

          case 'R':
            // Direction information
            direction = serverInfo.toInt();
            #ifdef DEBUG
              Serial.println("Direction of loco " + getDescription() + " is " + directionTxt[direction] + ".");
            #endif
            break;

          case 's':
            // Speed step information
            speedStepMode = serverInfo.toInt();
            #ifdef DEBUG
              Serial.println("Speed step mode of loco " + getDescription() + " is " + speedStepModeTxt[speedStepMode] + ".");
            #endif
            break;

          case 'V':
            // Notch information
            notch = serverInfo.toInt();
            if (serverInfo.toInt() == ESTOP) {
              #ifdef DEBUG
                Serial.println("Loco " + getDescription() + " has been stopped for emergency (Notch: " + String(notch) + ").");
              #endif
            }
            else {
              #ifdef DEBUG
                Serial.println("Notch of loco " + getDescription() + " is " + String(notch) + ".");
              #endif
            }
            break;

          default:
            // Unknown command
            #ifdef DEBUG
              Serial.println("Class VirtualLoco: Unknown command " + getAddressType() + String(address) + "<;>" + serverInfo);
            #endif
            /*
             * TODO
             */
            break;
        }
        break;

      case 'L':
        // Information according the loco's functions
        for(byte fn = 0; fn <= 28; fn++) {
          function[fn].listenToLoco(serverInfo);
        }
        break;

      case 'S':
        // Request steal locomotive
        #ifdef DEBUG
          Serial.println("Request steal locomotive: " + serverInfo);
        #endif
        /*
         * TODO
         */
        break;

      default:
        // Unknown command
        #ifdef DEBUG
          Serial.println("Class VirtualLoco: Unknown command '" + serverInfo + "'.");
        #endif
        break;
    }
  }
}
