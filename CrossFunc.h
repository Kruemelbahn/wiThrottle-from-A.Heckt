/*
 * Declaration of cross functional services
 */

#ifndef _CROSS_FUNC_H_
#define _CROSS_FUNC_H_

#include <Arduino.h>
#include <WiFi.h>

// Turn on serial output for debuging
#define DEBUG

// Define hardware layout
//#define HL_DISP                             // If defined hardware uses display setup
//#define ROT_ENCODER                         // If defined hardware uses rotary encoder instead of poti
//#define FCT_WITH_I2C                        // If defined hardware uses pcf8574 instead of direct connection of F-Buttons

// Software
#define SW_RELEASE_YEAR  2020               // Software release year
#define SW_LICENSE   "GPL v3"               // Software licence model
#define SW_VERSION     "0.90"               // Software version


// GPIO mapping

/*
 * GPIO  0 --> ADC2 pin, don't use when using WiFi!
 * GPIO  1 --> TX --> (Output)
 * GPIO  2 --> ADC2 pin, don't use when using WiFi!
 * GPIO  3 --> RX --> must be HIGH during boot --> (Input)
 * GPIO  4 --> ADC2 pin, don't use when using WiFi!
 * GPIO  5 --> Input/Output --> must be HIGH during boot (SCK)
 * GPIO 12 --> ADC2 pin, don't use when using WiFi!
 * GPIO 13 --> ADC2 pin, don't use when using WiFi!
 * GPIO 14 --> ADC2 pin, don't use when using WiFi!
 * GPIO 15 --> Input/Output --> must be LOW during boot
 * GPIO 16 --> Input/Output
 * GPIO 17 --> Input/Output
 * GPIO 18 --> Input/Output (MOSI)
 * GPIO 19 --> Input/Output (MISO)
 * GPIO 21 --> Input/Output
 * GPIO 22 --> SCL
 * GPIO 23 --> SDA
 * GPIO 25 --> ADC2 pin, don't use when using WiFi!
 * GPIO 26 --> ADC2 pin, don't use when using WiFi!
 * GPIO 27 --> ADC2 pin, don't use when using WiFi!
 * GPIO 32 --> Input/Output
 * GPIO 33 --> Input/Output
 * GPIO 34 --> Input, no pull-up
 * GPIO 35 --> Input, no pull-up
 * GPIO 36 --> Input, no pull-up
 * GPIO 39 --> Input, no pull-up
 * 
 * For wake up only so called RTC IO can be used. RTC IO pins are:
 * 0, 2, 4, 12 - 15, 25 - 27, 32 - 39.
 */

// Push buttons
/*
 * Hardware can be build with up to 9 (without I2C display) or up to
 * 3 function buttons (with I2C display) + one shift button (both 
 * versions)
 */
#define BTN_STOP           15               // Emergency stop button
#define BTN_FCT_SH         17               // Shift button
#define BTN_FCT_01         18               // Function button # 1 --> set to 0, if not used in hardware setup
#define BTN_FCT_02         23               // Function button # 2 --> set to 0, if not used in hardware setup
#define BTN_FCT_03         19               // Function button # 3 --> set to 0, if not used in hardware setup
#define BTN_FCT_04         39               // Function button # 4 --> set to 0, if not used in hardware setup
#define BTN_FCT_05         34               // Function button # 5 --> set to 0, if not used in hardware setup
#define BTN_FCT_06         33               // Function button # 6 --> set to 0, if not used in hardware setup
#define BTN_FCT_07          0               // Function button # 7 --> set to 0, if not used in hardware setup
#define BTN_FCT_08          0               // Function button # 8 --> set to 0, if not used in hardware setup
#define BTN_FCT_09          0               // Function button # 9 --> set to 0, if not used in hardware setup

// Direction switch (tri state)
#define DIR_SW             16               // Direction switch

// Rotary potentiometer
#define POT_SIG            36               // Variable output of potentiometer for speed control

// VBatt
#define V_BATT						  0

// LED
#define LED_STOP            5               // Emergency stop LED
#define LED_FWD            21               // Forward LED
#define LED_REV            22               // Reverse LED

// Rotary encoder
#define ENC_CLK            34               // Clock line from rotary encoder CLK
#define ENC_DT             39               // Data line from rotary encoder DT
#define ENC_BTN            15               // Switch from rotary encoder SW
#define ENC_PWR            -1               // VCC pin # (-1 = powered separately)

// I2C OLED display
#define OLED_SDA           23               // Data line for I2C OLED display SDA
#define OLED_SCL           22               // Clock line for I2C OLED display SCL
#define OLED_RESET         -1               // Reset pin # (-1 = sharing Arduino reset pin)

// I2C OLED display
#define OLED_I2C         0x3c               // I2C address
                                            /*
                                             * Display:
                                             * ┌────────────────────────┐
                                             * │No. of locos, JMRI, WiFi│ --> OLED_AREA_1
                                             * ├────────────────────────┤
                                             * │Direction and fast clock│ --> OLED_AREA_2
                                             * ├────────────────────────┤
                                             * │Other information       │ --> OLED_AREA_3
                                             * │                        │
                                             * │                        │
                                             * │                        │
                                             * │                        │
                                             * │                        │
                                             * │                        │
                                             * │                        │
                                             * └────────────────────────┘
                                             */
#define OLED_HEIGHT        64               // I2C OLED display height in pixels
#define OLED_WIDTH        128               // I2C display width in pixels
#define OLED_COLOR_BLACK  LOW               // Pixel is dark
#define OLED_COLOR_WHITE HIGH               // Pixel is bright
#define OLED_AREA_1_X       0               // X position of top left corner
#define OLED_AREA_1_Y       0               // Y position of top left corner
#define OLED_AREA_1_W      64               // Width
#define OLED_AREA_1_H      16               // Heigt
#define OLED_AREA_2_X       0               // X position of top left corner
#define OLED_AREA_2_Y      19               // Y position of top left corner
#define OLED_AREA_2_W      64               // Width
#define OLED_AREA_2_H       7               // Heigt
#define OLED_AREA_3_X       0               // X position of top left corner
#define OLED_AREA_3_Y      28               // Y position of top left corner
#define OLED_AREA_3_W      64               // Width
#define OLED_AREA_3_H     112               // Heigt
#define OLED_JMRI_X        24               // X position of the JMRI symbol
#define OLED_WIFI_X        48               // X position of the WiFi symbol
#define OLED_FWD_X         57               // X position of the FWD symbol
#define OLED_REV_X          0               // X position of the REV symbol


// Other constants
#define LIFO_SIZE          50               // Size of LIFO array used for smoothing speed DCC notch reference read from potentiometer
#define EEPROM_SIZE        64               // EEPROM config
#define notchRange        126               // Range of DCC notches
                                            /*
                                             * Valid ranges:
                                             * 0 to 13  (NMRA: optional)
                                             * 0 to 27  (NMRA: mandatory)
                                             * 0 to 126 (NMRA: optional)
                                             */


// WiFi communication

// WiFi settings
typedef struct {
  char* ssid;                               // WiFi SSID
  char* pwd;                                // WiFi passphrase
  unsigned int attempts;                    // Attempts to try to connect to WiFi
                                            // Error is thrown by code, if connection is not established after <attempts> attempts
} wiFiConfig;


// WiThrottle server communication

#define JMRI_DELAY        350               // Delay for a stable server communication

// WiThrottle server settings
typedef struct {
  char* ip;                                 // IP address of WiThrottle server
  unsigned int port;                        // Port used by WiThrottle server
                                            // Default port: 12090 according to WiThrottle server settings
  unsigned int attempts;                    // Attempts to try to connect to WiThrottle server
                                            // Error is thrown by code, if connection is not established after <attempts> attempts
  String protocolVersion;                   // Version number of the WiThrottle protocol used by WiThrottle server
  unsigned int heartbeat;                   // Withrottle server expecteds heartbeat after <heartbeat> seconds
} hostConfig;

void sendCmd(String command, bool waitAfterCommand = true);
                                            // Send command to WiThrottle server
String readCmd();                           // Read command from WiThrottle server

#endif
