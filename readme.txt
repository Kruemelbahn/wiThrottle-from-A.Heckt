# ESP32 WiTrottle for JMRI

Inspired by the Simple Wifi Throttle project by Geoff Bunza and modifications by Phil Abernathy as well as by the wiFred project by Heiko Rosemann.

Designed to be used with ESP32 board 'SparkFun Thing Plus - ESP32 WROOM'.

## Attention

Please put your WiFi credentials (lines 14 and 15) and the IP of your WiThrottle server (line 26) in file <CrossFunc.cpp>!


## Parts

* ebay (tapping screws DIN 7981 2,2 x 6,5 mm): https://www.ebay.de/itm/193210250475
* ebay (tactile switches 6 x 6 x 9,5 mm): https://www.ebay.de/itm/292082752651
* Eremit (LiPo): https://www.eremit.de/p/eremit-3-7v-500mah-lipo-akku-902030-jst-ph-2-0mm
* EXP Tech (ESP32): https://www.exp-tech.de/plattformen/internet-of-things-iot/9339/sparkfun-thing-plus-esp32-wroom
* Reichelt (other electronic components): https://www.reichelt.de/my/1705244


## Usage
* General usage is equivalent to FREMO-Fredi (http://fremodcc.sourceforge.net/diy/fred2/mini_anl_fredi_d.html)
* DCC address is set by serial monitor
* Power on: press red button > 1 second
* Power off: press red button > 5 seconds


## Ressources

1. https://www.jmri.org/help/en/package/jmri/jmrit/withrottle/Protocol.shtml
2. https://model-railroad-hobbyist.com/node/35652
3. https://newheiko.github.io/wiFred/documentation/html/docu.html
4. https://randomnerdtutorials.com/esp32-pinout-reference-gpios/