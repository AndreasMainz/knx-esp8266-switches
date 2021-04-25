# knx-esp8266-switches
Simple 4xtoggle Switch with Short and Long actuation 
based on the Konnekting Knx Library:
https://www.konnekting.de/
and MicroBcu2 here: https://gitlab.com/knx-makerstuff/knx_microbcu2

HW setup:  swaped Serial -> Serial2  on D7(GPIO13)=RX   GPIO15(D8)=TX 19200 baud -> standard for this lib

Taster1 attached to gpio5    // D1

Taster2 attached to gpio12   // D6

Taster3 attached to gpio14   // D5

Taster4 attached to gpio4    // D2

Each Taster is in "UM" Mode, if the last value ws "0" next activation will send "1" and visa versa
The groupadress will be send out with release edge of switch.
The time between rising and falling edge is measured.
If the two events happen inbetween 600ms a short activation is detected, also sending on the ga's for short activation
If the two events differ more than 600ms a long activation is detected, also sending on the ga's for long activation

The switch is listening to Input GA's in order to register external commands at the own Send GA's.
Internal state will be corrected to value of external GA's.

A Pcb is also available, alsready under test, coming soon.

